// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "connection/iconnection_service.h"
#include "daemon/daemon_connection_service.h"
#include "daemon/daemon_transport.h"
#include "daemon/node_api_client.h"
#include "daemon/node_api_codec.h"
#include "settings/isettings_store.h"
#include "wire_mux_fixture.h"
#include "ws_mux_fixture.h"

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

using daemonapp::daemon::DaemonConnectionService;
using daemonapp::daemon::DaemonTransport;
using daemonapp::daemon::NodeApiClient;
using daemonapp::daemon::NodeApiCodec;
using daemonapp::test::MuxServerCore;
using daemonapp::test::WireMuxServer;
using daemonapp::test::WsMuxServer;

namespace {

// ApiResponse::Ok is the externally-tagged unit variant -> the bare CBOR text string "Ok".
QByteArray okResponse() {
    return QByteArrayLiteral("\x62Ok");
}

// A healthy Health response: {"Health": {"all_ok": true, "services": []}} - what the connection
// service needs to resolve "ready".
QByteArray healthResponse() {
    return {"\xA1\x66Health\xA2\x66"
            "all_ok\xF5\x68services\x80",
            27};
}

// A memory-only ISettingsStore so the token-persistence path runs without touching the user's
// real QSettings.
class MemorySettingsStore : public settings::ISettingsStore {
public:
    using settings::ISettingsStore::ISettingsStore;
    [[nodiscard]] QVariant value(const QString& key, const QVariant& fallback = {}) const override {
        return m_values.value(key, fallback);
    }
    void setValue(const QString& key, const QVariant& value) override {
        m_values.insert(key, value);
        emit changed(key);
        emit changedAny();
    }

private:
    QHash<QString, QVariant> m_values;
};

// The client-visible outcome of one scripted mux session (a Call round-trip plus a pushed
// Open/Item/End stream), collected identically over any carrier for the parity comparison.
struct MuxRunResult {
    QByteArray reply;
    QList<QByteArray> items;
    qsizetype endCount = 0;
    bool endError = true;
};

// Drive the shared scenario against `fake` over whatever carrier `configureTarget` points the
// transport at. Returns false on any timeout (asserted by the caller); fills `out` on success.
template <typename ConfigureTarget>
[[nodiscard]] bool driveMuxScenario(MuxServerCore& fake, ConfigureTarget configureTarget,
                                    MuxRunResult* out) {
    fake.setReplyPayload(okResponse());

    DaemonTransport transport;
    configureTarget(transport);
    NodeApiClient client(&transport);

    QSignalSpy responses(&client, &NodeApiClient::responseReady);
    QSignalSpy items(&client, &NodeApiClient::streamItem);
    QSignalSpy ends(&client, &NodeApiClient::streamEnded);

    client.sendRequest(NodeApiCodec::encodeHealthRequest(), QStringLiteral("c1"));
    if (!QTest::qWaitFor([&] { return responses.count() >= 1; }, 3000)) {
        return false;
    }
    out->reply = responses.at(0).at(1).toByteArray();

    const quint64 id =
        client.openStream(NodeApiCodec::encodeSubscribeRequest(QStringLiteral("s1"), 0, 64));
    if (!QTest::qWaitFor([&] { return fake.lastOpenId() == id; }, 3000)) {
        return false;
    }
    fake.pushItem(okResponse());
    fake.pushItem(healthResponse());
    if (!QTest::qWaitFor([&] { return items.count() >= 2; }, 3000)) {
        return false;
    }
    for (int i = 0; i < items.count(); ++i) {
        out->items.append(items.at(i).at(1).toByteArray());
    }
    fake.endStream(false);
    if (!QTest::qWaitFor([&] { return ends.count() >= 1; }, 3000)) {
        return false;
    }
    out->endCount = ends.count();
    out->endError = ends.at(0).at(1).toBool();
    return true;
}

} // namespace

// The WebSocket daemon transport (the browser/wasm carrier, exercised natively): the `daemon-mux`
// subprotocol handshake, message-per-frame Call/stream round-trips, SASL over WS, behavioral
// parity with the Unix stream carrier, and the connection service's `remote-ws` mode.
class TestWsTransport : public QObject {
    Q_OBJECT

private slots:
    // Connect + Hello over WS: the client handshakes through the WebSocket carrier, negotiating
    // the pinned `daemon-mux` subprotocol, and surfaces the daemon's advertised api version.
    void helloHandshakeOverWs() {
        WsMuxServer fake;
        fake.setReplyPayload(okResponse());
        QVERIFY2(fake.start(), "fixture must listen");

        DaemonTransport transport;
        transport.setWsTarget(fake.url());
        NodeApiClient client(&transport);

        QSignalSpy handshakes(&client, &NodeApiClient::handshakeReady);
        client.sendRequest(NodeApiCodec::encodeHealthRequest(), QStringLiteral("c1"));

        QTRY_COMPARE_WITH_TIMEOUT(handshakes.count(), 1, 3000);
        QVERIFY(fake.helloCount() >= 1);
        QCOMPARE(fake.lastSubprotocol(), QStringLiteral("daemon-mux"));
        QCOMPARE(client.daemonApiVersion(), NodeApiCodec::kDaemonApiVersion);
        QVERIFY(client.streamingAvailable());
        QVERIFY(transport.isConnected());
    }

    // A one-shot Call round-trips over WS: one binary message per frame in each direction, the
    // Reply correlated to the caller's id with the inner ApiResponse bytes unchanged.
    void oneShotCallRoundTripsOverWs() {
        WsMuxServer fake;
        fake.setReplyPayload(okResponse());
        QVERIFY2(fake.start(), "fixture must listen");

        DaemonTransport transport;
        transport.setWsTarget(fake.url());
        NodeApiClient client(&transport);

        QSignalSpy responses(&client, &NodeApiClient::responseReady);
        client.sendRequest(NodeApiCodec::encodeHealthRequest(), QStringLiteral("c1"));

        QTRY_COMPARE_WITH_TIMEOUT(responses.count(), 1, 3000);
        QCOMPARE(responses.at(0).at(0).toString(), QStringLiteral("c1"));
        QCOMPARE(responses.at(0).at(1).toByteArray(), okResponse());
        QCOMPARE(fake.requestCount(), 1);
    }

    // SASL over WS: a SCRAM-SHA-256 node authenticates through the WebSocket carrier, the queued
    // Call stays gated until AuthOk, and the issued session token surfaces.
    void scramAuthRoundTripOverWs() {
        WsMuxServer fake;
        fake.setReplyPayload(okResponse());
        fake.setAuthMechanisms({QStringLiteral("SCRAM-SHA-256")});
        fake.setCredentials("user", "pencil");
        QVERIFY2(fake.start(), "fixture must listen");

        DaemonTransport transport;
        transport.setWsTarget(fake.url());
        NodeApiClient client(&transport);
        NodeApiClient::AuthCredentials creds;
        creds.username = QStringLiteral("user");
        creds.password = QStringLiteral("pencil");
        client.setAuthCredentials(creds);

        QSignalSpy authed(&client, &NodeApiClient::authenticated);
        QSignalSpy tokens(&client, &NodeApiClient::tokenIssued);
        QSignalSpy responses(&client, &NodeApiClient::responseReady);

        client.sendRequest(NodeApiCodec::encodeHealthRequest(), QStringLiteral("c1"));

        QTRY_COMPARE_WITH_TIMEOUT(authed.count(), 1, 3000);
        QTRY_COMPARE_WITH_TIMEOUT(responses.count(), 1, 3000);
        QCOMPARE(fake.callsBeforeAuthOk(), 0); // outbox gate held until AuthOk
        QCOMPARE(fake.authOkCount(), 1);
        QCOMPARE(tokens.count(), 1);
        QCOMPARE(tokens.at(0).at(0).toString(), QStringLiteral("session-token-xyz"));
        QVERIFY(client.isAuthenticated());
        QCOMPARE(client.principal().username, QStringLiteral("user"));
    }

    // WS-vs-Unix parity: the identical scripted session (Call round-trip + Open/Item/Item/End)
    // driven over both carriers yields the identical client-visible outcome - same reply bytes,
    // same item bytes in order, same clean close - and the fixtures observe the same traffic.
    void wsAndUnixCarriersBehaveIdentically() {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        const QString sock = tmp.filePath(QStringLiteral("parity.sock"));

        WireMuxServer unixFake;
        QVERIFY2(unixFake.start(sock), "unix fixture must listen");
        MuxRunResult unixRun;
        QVERIFY2(driveMuxScenario(
                     unixFake, [&sock](DaemonTransport& t) { t.setSocketPath(sock); }, &unixRun),
                 "unix scenario timed out");

        WsMuxServer wsFake;
        QVERIFY2(wsFake.start(), "ws fixture must listen");
        MuxRunResult wsRun;
        QVERIFY2(
            driveMuxScenario(
                wsFake, [&wsFake](DaemonTransport& t) { t.setWsTarget(wsFake.url()); }, &wsRun),
            "ws scenario timed out");

        QCOMPARE(wsRun.reply, unixRun.reply);
        QCOMPARE(wsRun.items, unixRun.items);
        QCOMPARE(wsRun.endCount, unixRun.endCount);
        QCOMPARE(wsRun.endError, unixRun.endError);
        QCOMPARE(wsFake.helloCount(), unixFake.helloCount());
        QCOMPARE(wsFake.requestCount(), unixFake.requestCount());
        QCOMPARE(wsFake.openCount(), unixFake.openCount());
    }

    // A refused WS connect surfaces failed() (the errorOccurred -> close path) and never
    // connects; the transport stays usable for a later target.
    void refusedWsConnectFailsClosed() {
        DaemonTransport transport;
        // Port 1 on localhost: nothing listens there, so the connect is refused.
        transport.setWsTarget(QUrl(QStringLiteral("ws://127.0.0.1:1")));
        QSignalSpy failures(&transport, &DaemonTransport::failed);
        QSignalSpy connects(&transport, &DaemonTransport::connected);

        transport.sendFrame(okResponse());
        QTRY_VERIFY_WITH_TIMEOUT(failures.count() >= 1, 5000);
        QCOMPARE(connects.count(), 0);
        QVERIFY(!transport.isConnected());
    }

    // The SASL mechanism gate: wss:// counts as TLS (PLAIN permitted), plain ws:// does not, and
    // a WS target never carries a client certificate (EXTERNAL) even if a previous TCP target
    // configured one.
    void wssSchemeCountsAsTlsForSaslGate() {
        DaemonTransport transport;
        transport.setWsTarget(QUrl(QStringLiteral("ws://node.example:9443")));
        QVERIFY(!transport.tlsActive());

        transport.setWsTarget(QUrl(QStringLiteral("wss://node.example:9443")));
        QVERIFY(transport.tlsActive());
        QVERIFY(!transport.clientCertConfigured());

        daemonapp::daemon::TlsConfig tls;
        tls.clientCertFile = QStringLiteral("/tmp/cert.pem");
        tls.clientKeyFile = QStringLiteral("/tmp/key.pem");
        transport.setTcpTarget(QStringLiteral("node.example"), 443, tls);
        QVERIFY(transport.clientCertConfigured());
        transport.setWsTarget(QUrl(QStringLiteral("wss://node.example")));
        QVERIFY2(!transport.clientCertConfigured(), "mTLS must not leak onto the WS carrier");
    }

    // Connection-service remote-ws target parsing/validation: full ws://+wss:// URLs pass the
    // shape probe, everything else (no scheme, wrong scheme, no host) is rejected, and a
    // malformed Connect lands in "needs setup" without dialing anything.
    void serviceValidatesWsTargets() {
        DaemonConnectionService svc;
        QSignalSpy results(&svc, &connection::IConnectionService::testResult);

        const QStringList accepted{
            QStringLiteral("ws://127.0.0.1:9443"),
            QStringLiteral("wss://node.example"),
            QStringLiteral("wss://node.example:8443/daemon"),
            QStringLiteral("  wss://node.example:8443  "), // tolerated: trimmed before parsing
        };
        const QStringList rejected{
            QStringLiteral("node.example:8443"),         // no scheme: ws vs wss must be explicit
            QStringLiteral("https://node.example:8443"), // not a WebSocket scheme
            QStringLiteral("ws://"),                     // no host
            QString(),
        };

        int expected = 0;
        for (const QString& target : accepted) {
            svc.testConnection(QStringLiteral("remote-ws"), target);
            ++expected;
            QCOMPARE(results.count(), expected);
            QVERIFY2(results.at(expected - 1).at(0).toBool(), qPrintable("rejected: " + target));
        }
        for (const QString& target : rejected) {
            svc.testConnection(QStringLiteral("remote-ws"), target);
            ++expected;
            QCOMPARE(results.count(), expected);
            QVERIFY2(!results.at(expected - 1).at(0).toBool(), qPrintable("accepted: " + target));
        }

        // A malformed target on Connect: refused up front (no transport target to dial).
        svc.connectTo(QStringLiteral("remote-ws"), QStringLiteral("node.example:8443"));
        QCOMPARE(svc.state(), QStringLiteral("needs setup"));
    }

    // remote-ws end-to-end: connectTo() dials the WS fixture, the Health probe resolves the
    // liveness machine to "ready", and an explicit disconnect returns to "offline".
    void serviceConnectsOverWsToReady() {
        WsMuxServer fake;
        fake.setReplyPayload(healthResponse());
        QVERIFY2(fake.start(), "fixture must listen");

        DaemonConnectionService svc;
        svc.connectTo(QStringLiteral("remote-ws"), fake.url().toString());
        QVERIFY(QTest::qWaitFor([&] { return svc.ready(); }, 5000));
        QCOMPARE(fake.lastSubprotocol(), QStringLiteral("daemon-mux"));

        svc.disconnect();
        QCOMPARE(svc.state(), QStringLiteral("offline"));
    }

    // remote-ws auth: an authenticating node drives the service to the login form, login()
    // completes SCRAM over WS, and the issued session token is persisted keyed by the URL target
    // (the same per-target scheme the other modes use).
    void serviceWsLoginPersistsTokenPerTarget() {
        WsMuxServer fake;
        fake.setReplyPayload(healthResponse());
        fake.setAuthMechanisms({QStringLiteral("SCRAM-SHA-256")});
        fake.setCredentials("user", "pencil");
        QVERIFY2(fake.start(), "fixture must listen");
        const QString target = fake.url().toString();

        MemorySettingsStore settings;
        settings.setManagedLocalDaemon(false); // remote-ws must not need the launcher anyway
        DaemonConnectionService svc(nullptr, &settings);

        svc.connectTo(QStringLiteral("remote-ws"), target);
        QVERIFY(
            QTest::qWaitFor([&] { return svc.state() == QLatin1String("authenticating"); }, 5000));

        svc.login(QStringLiteral("user"), QStringLiteral("pencil"));
        QVERIFY(QTest::qWaitFor([&] { return svc.ready(); }, 5000));
        QCOMPARE(settings.connectionToken(target), QStringLiteral("session-token-xyz"));
    }
};

QTEST_GUILESS_MAIN(TestWsTransport)
#include "tst_ws_transport.moc"
