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
