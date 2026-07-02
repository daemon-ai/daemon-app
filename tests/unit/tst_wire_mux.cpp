// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemon/daemon_transport.h"
#include "daemon/node_api_client.h"
#include "daemon/node_api_codec.h"
#include "wire_mux_fixture.h"

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

using daemonapp::daemon::DaemonTransport;
using daemonapp::daemon::NodeApiClient;
using daemonapp::daemon::NodeApiCodec;
using daemonapp::test::WireMuxServer;

namespace {
// ApiResponse::Ok is the externally-tagged unit variant -> the bare CBOR text string "Ok".
QByteArray okResponse() {
    return {"\x62Ok", 3};
}
} // namespace

// Phase 1 (L0): the daemon-app multiplexer over the wire envelope. Drives the real
// DaemonTransport+NodeApiClient against the WireMuxServer fixture: the Hello handshake, one-shot
// Call/Reply correlation, a push Open/Item/End stream, and graceful Cancel.
class TestWireMux : public QObject {
    Q_OBJECT

private slots:
    // A one-shot Call round-trips: Hello is negotiated, the Reply is delivered to the matching
    // correlation id, and the inner ApiResponse bytes are surfaced unchanged.
    void oneShotCallRoundTrips() {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        const QString path = tmp.filePath(QStringLiteral("mux.sock"));
        WireMuxServer fake;
        fake.setReplyPayload(okResponse());
        QVERIFY2(fake.start(path), "fixture must listen");

        DaemonTransport transport;
        transport.setSocketPath(path);
        NodeApiClient client(&transport);

        QSignalSpy responses(&client, &NodeApiClient::responseReady);
        QVERIFY(responses.isValid());
        client.sendRequest(NodeApiCodec::encodeHealthRequest(), QStringLiteral("c1"));

        QTRY_COMPARE_WITH_TIMEOUT(responses.count(), 1, 3000);
        QCOMPARE(responses.at(0).at(0).toString(), QStringLiteral("c1"));
        QCOMPARE(responses.at(0).at(1).toByteArray(), okResponse());
        QVERIFY(fake.helloCount() >= 1); // the client handshook before sending the Call
        QVERIFY(client.streamingAvailable());
    }

    // The client surfaces the daemon's "api/<N>" Hello feature as its API contract version - the
    // input to the connection service's version gate. A daemon without the advertisement (an old
    // build) reports 0.
    void helloSurfacesDaemonApiVersion() {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());

        // A current daemon (the fixture default) advertises the client's own contract version.
        const QString path = tmp.filePath(QStringLiteral("mux-api.sock"));
        WireMuxServer fake;
        fake.setReplyPayload(okResponse());
        QVERIFY2(fake.start(path), "fixture must listen");
        DaemonTransport transport;
        transport.setSocketPath(path);
        NodeApiClient client(&transport);
        QSignalSpy handshakes(&client, &NodeApiClient::handshakeReady);
        client.sendRequest(NodeApiCodec::encodeHealthRequest(), QStringLiteral("c1"));
        QTRY_COMPARE_WITH_TIMEOUT(handshakes.count(), 1, 3000);
        QCOMPARE(client.daemonApiVersion(), NodeApiCodec::kDaemonApiVersion);

        // A pre-advertisement daemon (features without "api/<N>") reports 0.
        const QString oldPath = tmp.filePath(QStringLiteral("mux-api-old.sock"));
        WireMuxServer oldFake;
        oldFake.setReplyPayload(okResponse());
        oldFake.setHelloFeatures({QStringLiteral("mux"), QStringLiteral("stream")});
        QVERIFY2(oldFake.start(oldPath), "fixture must listen");
        DaemonTransport oldTransport;
        oldTransport.setSocketPath(oldPath);
        NodeApiClient oldClient(&oldTransport);
        QSignalSpy oldHandshakes(&oldClient, &NodeApiClient::handshakeReady);
        oldClient.sendRequest(NodeApiCodec::encodeHealthRequest(), QStringLiteral("c1"));
        QTRY_COMPARE_WITH_TIMEOUT(oldHandshakes.count(), 1, 3000);
        QCOMPARE(oldClient.daemonApiVersion(), quint32(0));
    }

    // Two concurrent one-shot Calls are correlated independently by id (no head-of-line blocking of
    // one behind the other).
    void concurrentCallsAreCorrelated() {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        const QString path = tmp.filePath(QStringLiteral("mux2.sock"));
        WireMuxServer fake;
        fake.setReplyPayload(okResponse());
        QVERIFY2(fake.start(path), "fixture must listen");

        DaemonTransport transport;
        transport.setSocketPath(path);
        NodeApiClient client(&transport);
        QSignalSpy responses(&client, &NodeApiClient::responseReady);

        client.sendRequest(NodeApiCodec::encodeHealthRequest(), QStringLiteral("a"));
        client.sendRequest(NodeApiCodec::encodeHealthRequest(), QStringLiteral("b"));
        QTRY_COMPARE_WITH_TIMEOUT(responses.count(), 2, 3000);
        QStringList seen{responses.at(0).at(0).toString(), responses.at(1).at(0).toString()};
        seen.sort();
        QCOMPARE(seen, (QStringList{QStringLiteral("a"), QStringLiteral("b")}));
    }

    // A push stream: Open is sent after the handshake, Items are delivered to the stream id, and a
    // clean End closes it.
    void pushStreamDeliversItemsThenEnd() {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        const QString path = tmp.filePath(QStringLiteral("mux-stream.sock"));
        WireMuxServer fake;
        QVERIFY2(fake.start(path), "fixture must listen");

        DaemonTransport transport;
        transport.setSocketPath(path);
        NodeApiClient client(&transport);
        QSignalSpy items(&client, &NodeApiClient::streamItem);
        QSignalSpy ends(&client, &NodeApiClient::streamEnded);

        const quint64 id =
            client.openStream(NodeApiCodec::encodeSubscribeRequest(QStringLiteral("s1"), 0, 64));
        QVERIFY(id != 0);
        QTRY_COMPARE_WITH_TIMEOUT(fake.lastOpenId(), id, 3000); // server saw the Open

        fake.pushItem(okResponse());
        QTRY_COMPARE_WITH_TIMEOUT(items.count(), 1, 3000);
        QCOMPARE(items.at(0).at(0).toULongLong(), id);
        QCOMPARE(items.at(0).at(1).toByteArray(), okResponse());

        fake.endStream(false);
        QTRY_COMPARE_WITH_TIMEOUT(ends.count(), 1, 3000);
        QCOMPARE(ends.at(0).at(0).toULongLong(), id);
        QCOMPARE(ends.at(0).at(1).toBool(), false); // clean close
    }

    // A node that advertises SCRAM-SHA-256 must authenticate before any Call escapes: the queued
    // request stays buffered until AuthOk, then flushes exactly once. Drives the real SCRAM client
    // against the fixture's real SCRAM server (a dynamic round trip with a random client nonce).
    void scramAuthGatesOutboxThenFlushes() {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        const QString path = tmp.filePath(QStringLiteral("mux-scram.sock"));
        WireMuxServer fake;
        fake.setReplyPayload(okResponse());
        fake.setAuthMechanisms({QStringLiteral("SCRAM-SHA-256")});
        fake.setCredentials("user", "pencil");
        QVERIFY2(fake.start(path), "fixture must listen");

        DaemonTransport transport;
        transport.setSocketPath(path);
        NodeApiClient client(&transport);
        NodeApiClient::AuthCredentials creds;
        creds.username = QStringLiteral("user");
        creds.password = QStringLiteral("pencil");
        client.setAuthCredentials(creds);

        QSignalSpy authed(&client, &NodeApiClient::authenticated);
        QSignalSpy tokens(&client, &NodeApiClient::tokenIssued);
        QSignalSpy responses(&client, &NodeApiClient::responseReady);

        // Queue a Call up front: it must NOT reach the server until authentication completes.
        client.sendRequest(NodeApiCodec::encodeHealthRequest(), QStringLiteral("c1"));

        QTRY_COMPARE_WITH_TIMEOUT(authed.count(), 1, 3000);
        QTRY_COMPARE_WITH_TIMEOUT(responses.count(), 1, 3000);
        QCOMPARE(responses.at(0).at(0).toString(), QStringLiteral("c1"));
        QCOMPARE(fake.callsBeforeAuthOk(), 0); // outbox gate held until AuthOk
        QCOMPARE(fake.authOkCount(), 1);
        QCOMPARE(tokens.count(), 1);
        QCOMPARE(tokens.at(0).at(0).toString(), QStringLiteral("session-token-xyz"));
        QVERIFY(client.isAuthenticated());
        QCOMPARE(client.principal().username, QStringLiteral("user"));
        QVERIFY(client.principal().hasCapability(QStringLiteral("session_write")));
    }

    // Wrong password: the SCRAM proof fails server-side -> AuthError. The client surfaces
    // authFailed, stays unauthenticated, and no request escapes.
    void wrongPasswordFailsClosed() {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        const QString path = tmp.filePath(QStringLiteral("mux-badpw.sock"));
        WireMuxServer fake;
        fake.setReplyPayload(okResponse());
        fake.setAuthMechanisms({QStringLiteral("SCRAM-SHA-256")});
        fake.setCredentials("user", "pencil");
        QVERIFY2(fake.start(path), "fixture must listen");

        DaemonTransport transport;
        transport.setSocketPath(path);
        NodeApiClient client(&transport);
        NodeApiClient::AuthCredentials creds;
        creds.username = QStringLiteral("user");
        creds.password = QStringLiteral("WRONG");
        client.setAuthCredentials(creds);

        QSignalSpy failedAuth(&client, &NodeApiClient::authFailed);
        QSignalSpy responses(&client, &NodeApiClient::responseReady);
        client.sendRequest(NodeApiCodec::encodeHealthRequest(), QStringLiteral("c1"));

        QTRY_COMPARE_WITH_TIMEOUT(failedAuth.count(), 1, 3000);
        QCOMPARE(responses.count(), 0);
        QCOMPARE(fake.callsBeforeAuthOk(), 0);
        QVERIFY(!client.isAuthenticated());
    }

    // No credentials for an auth-required node: the client fails closed (no frames) and surfaces a
    // generic "authentication required" so the UI shows the login form.
    void noCredentialsRequiresLogin() {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        const QString path = tmp.filePath(QStringLiteral("mux-nocreds.sock"));
        WireMuxServer fake;
        fake.setAuthMechanisms({QStringLiteral("SCRAM-SHA-256")});
        QVERIFY2(fake.start(path), "fixture must listen");

        DaemonTransport transport;
        transport.setSocketPath(path);
        NodeApiClient client(&transport);

        QSignalSpy failedAuth(&client, &NodeApiClient::authFailed);
        client.sendRequest(NodeApiCodec::encodeHealthRequest(), QStringLiteral("c1"));

        QTRY_COMPARE_WITH_TIMEOUT(failedAuth.count(), 1, 3000);
        QCOMPARE(failedAuth.at(0).at(0).toString(), QStringLiteral("authentication required"));
        QCOMPARE(fake.authStartCount(), 0); // never even started a mechanism
        QVERIFY(!client.isAuthenticated());
    }

    // AuthResume fast-path: a stored token authenticates without a mechanism exchange.
    void authResumeReconnects() {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        const QString path = tmp.filePath(QStringLiteral("mux-resume.sock"));
        WireMuxServer fake;
        fake.setReplyPayload(okResponse());
        fake.setAuthMechanisms({QStringLiteral("SCRAM-SHA-256")});
        fake.setExpectedResumeToken("session-token-xyz");
        QVERIFY2(fake.start(path), "fixture must listen");

        DaemonTransport transport;
        transport.setSocketPath(path);
        NodeApiClient client(&transport);
        NodeApiClient::AuthCredentials creds;
        creds.resumeToken = QStringLiteral("session-token-xyz");
        client.setAuthCredentials(creds);

        QSignalSpy authed(&client, &NodeApiClient::authenticated);
        QSignalSpy responses(&client, &NodeApiClient::responseReady);
        client.sendRequest(NodeApiCodec::encodeHealthRequest(), QStringLiteral("c1"));

        QTRY_COMPARE_WITH_TIMEOUT(authed.count(), 1, 3000);
        QTRY_COMPARE_WITH_TIMEOUT(responses.count(), 1, 3000);
        QCOMPARE(fake.authStartCount(), 0); // resume skips the mechanism exchange
        QCOMPARE(fake.callsBeforeAuthOk(), 0);
    }

    // Cancel tears the stream down: the server acks with End and the client surfaces streamEnded.
    void cancelClosesStream() {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        const QString path = tmp.filePath(QStringLiteral("mux-cancel.sock"));
        WireMuxServer fake;
        QVERIFY2(fake.start(path), "fixture must listen");

        DaemonTransport transport;
        transport.setSocketPath(path);
        NodeApiClient client(&transport);
        QSignalSpy ends(&client, &NodeApiClient::streamEnded);

        const quint64 id =
            client.openStream(NodeApiCodec::encodeSubscribeRequest(QStringLiteral("s1"), 0, 64));
        QTRY_COMPARE_WITH_TIMEOUT(fake.lastOpenId(), id, 3000);
        client.cancelStream(id);
        QTRY_COMPARE_WITH_TIMEOUT(ends.count(), 1, 3000);
        QCOMPARE(ends.at(0).at(0).toULongLong(), id);
    }
};

QTEST_GUILESS_MAIN(TestWireMux)
#include "tst_wire_mux.moc"
