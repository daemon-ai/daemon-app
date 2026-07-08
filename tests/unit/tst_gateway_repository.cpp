// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// Phase F (node OpenAI-compatible gateway):
//  - GatewayRepository reads the live status (GatewayGet -> GatewayStatus) and toggles it
//    (GatewaySet{enabled, ? addr} -> GatewayStatus); an Unsupported GatewayGet flips supported()
//    false (older node) without a spurious failure; a GatewaySet error surfaces operationFailed;
//  - DaemonConnectionService retains the last Health report as the single health cache and emits
//    healthChanged, exposing the per-service entries (gateway, local-inference, ...).

#include "daemon/daemon_connection_service.h"
#include "daemon/daemon_transport.h"
#include "daemon/node_api_client.h"
#include "daemon/node_api_codec.h"
#include "daemon/repositories.h"
#include "daemon_api_client_encode.h"
#include "daemon_api_client_types.h"
#include "wire_mux_fixture.h"

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

using daemonapp::daemon::DaemonConnectionService;
using daemonapp::daemon::DaemonTransport;
using daemonapp::daemon::GatewayRepository;
using daemonapp::daemon::NodeApiClient;
using daemonapp::daemon::NodeApiCodec;
using daemonapp::test::WireMuxServer;

namespace {

void setStr(zcbor_string& z, const QByteArray& b) {
    z.value = reinterpret_cast<const uint8_t*>(b.constData());
    z.len = static_cast<size_t>(b.size());
}

// A GatewayStatus response via the generated encoder. Backing strings are static so they outlive
// the encode. Empty addr/lastError encode the optional-null arm.
QByteArray gatewayStatusResponse(bool enabled, bool listening, const QByteArray& addr,
                                 const QByteArray& lastError) {
    static QByteArray sAddr;
    static QByteArray sErr;
    sAddr = addr;
    sErr = lastError;

    api_response_r resp{};
    resp.api_response_choice = api_response_r::api_response_response_gateway_status_m_c;
    gateway_status& s =
        resp.api_response_response_gateway_status_m.response_gateway_status_GatewayStatus;
    s.gateway_status_enabled = enabled;
    s.gateway_status_listening = listening;
    if (!sAddr.isEmpty()) {
        s.gateway_status_addr_choice = gateway_status::gateway_status_addr_tstr_c;
        setStr(s.gateway_status_addr_tstr, sAddr);
    } else {
        s.gateway_status_addr_choice = gateway_status::gateway_status_addr_null_m_c;
    }
    if (!sErr.isEmpty()) {
        s.gateway_status_last_error_choice = gateway_status::gateway_status_last_error_tstr_c;
        setStr(s.gateway_status_last_error_tstr, sErr);
    } else {
        s.gateway_status_last_error_choice = gateway_status::gateway_status_last_error_null_m_c;
    }

    QByteArray out(4096, Qt::Uninitialized);
    size_t written = 0;
    if (cbor_encode_api_response(reinterpret_cast<uint8_t*>(out.data()),
                                 static_cast<size_t>(out.size()), &resp,
                                 &written) != ZCBOR_SUCCESS) {
        return {};
    }
    out.resize(static_cast<qsizetype>(written));
    return out;
}

// A Health response carrying the named services (all ok, so the connection reaches "ready").
QByteArray healthResponse(const QList<QByteArray>& names) {
    static QList<QByteArray> backing;
    backing = names;

    api_response_r resp{};
    resp.api_response_choice = api_response_r::api_response_response_health_m_c;
    health_report& r = resp.api_response_response_health_m.response_health_Health;
    r.health_report_all_ok = true;
    r.health_report_services_service_health_m_count = static_cast<size_t>(backing.size());
    for (int i = 0; i < backing.size(); ++i) {
        service_health& sv = r.health_report_services_service_health_m[i];
        setStr(sv.service_health_name, backing.at(i));
        sv.service_health_ok = true;
        sv.service_health_restarts = 0;
        sv.service_health_detail_choice = service_health::service_health_detail_null_m_c;
    }

    QByteArray out(8192, Qt::Uninitialized);
    size_t written = 0;
    if (cbor_encode_api_response(reinterpret_cast<uint8_t*>(out.data()),
                                 static_cast<size_t>(out.size()), &resp,
                                 &written) != ZCBOR_SUCCESS) {
        return {};
    }
    out.resize(static_cast<qsizetype>(written));
    return out;
}

// {"Error": {"Unsupported": "<msg>"}} — the externally-tagged Error envelope.
QByteArray unsupportedError(const char* msg) {
    QByteArray b;
    b.append(static_cast<char>(0xA1));
    daemonapp::test::cborText(b, "Error");
    b.append(static_cast<char>(0xA1));
    daemonapp::test::cborText(b, "Unsupported");
    daemonapp::test::cborText(b, msg);
    return b;
}

} // namespace

class TestGatewayRepository : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_tmp;

private slots:
    // A GatewayGet round-trip populates the cached status; the request is byte-identical to the
    // encoder's output.
    void refreshPopulatesStatus() {
        const QString sock = m_tmp.filePath(QStringLiteral("gw-get.sock"));
        WireMuxServer fake;
        QVERIFY2(fake.start(sock), "listen");
        fake.setReplyPayload(
            gatewayStatusResponse(true, true, QByteArrayLiteral("127.0.0.1:8899"), QByteArray()));
        DaemonTransport transport;
        transport.setSocketPath(sock);
        NodeApiClient client(&transport);
        GatewayRepository repo(&client);

        QSignalSpy changed(&repo, &GatewayRepository::statusChanged);
        repo.refresh();
        QTRY_COMPARE_WITH_TIMEOUT(changed.count(), 1, 3000);
        QVERIFY(repo.loaded());
        QVERIFY(repo.supported());
        QVERIFY(repo.enabled());
        QVERIFY(repo.listening());
        QCOMPARE(repo.addr(), QStringLiteral("127.0.0.1:8899"));
        QVERIFY(repo.lastError().isEmpty());

        const QList<QByteArray> calls = fake.callPayloads();
        QCOMPARE(calls.size(), 1);
        QCOMPARE(calls.at(0), NodeApiCodec::encodeGatewayGetRequest());
    }

    // GatewaySet(enabled, addr) round-trips: the reply is a GatewayStatus (statusChanged) and the
    // request is byte-identical to the encoder's output.
    void setEnabledRoundTrips() {
        const QString sock = m_tmp.filePath(QStringLiteral("gw-set.sock"));
        WireMuxServer fake;
        QVERIFY2(fake.start(sock), "listen");
        fake.setReplyPayload(
            gatewayStatusResponse(true, false, QByteArrayLiteral("127.0.0.1:9000"), QByteArray()));
        DaemonTransport transport;
        transport.setSocketPath(sock);
        NodeApiClient client(&transport);
        GatewayRepository repo(&client);

        QSignalSpy changed(&repo, &GatewayRepository::statusChanged);
        repo.setEnabled(true, QStringLiteral("127.0.0.1:9000"));
        QTRY_COMPARE_WITH_TIMEOUT(changed.count(), 1, 3000);
        QVERIFY(repo.enabled());
        QVERIFY(!repo.listening()); // enabled but not yet bound

        const QList<QByteArray> calls = fake.callPayloads();
        QCOMPARE(calls.size(), 1);
        QCOMPARE(calls.at(0),
                 NodeApiCodec::encodeGatewaySetRequest(true, QStringLiteral("127.0.0.1:9000")));
    }

    // An Unsupported GatewayGet marks the gateway unsupported (older node) via statusChanged,
    // WITHOUT surfacing operationFailed (it is a capability gap, not a failure).
    void unsupportedMarksUnsupported() {
        const QString sock = m_tmp.filePath(QStringLiteral("gw-uns.sock"));
        WireMuxServer fake;
        QVERIFY2(fake.start(sock), "listen");
        fake.setReplyPayload(unsupportedError("no gateway"));
        DaemonTransport transport;
        transport.setSocketPath(sock);
        NodeApiClient client(&transport);
        GatewayRepository repo(&client);

        QSignalSpy changed(&repo, &GatewayRepository::statusChanged);
        QSignalSpy failed(&repo, &GatewayRepository::operationFailed);
        repo.refresh();
        QTRY_COMPARE_WITH_TIMEOUT(changed.count(), 1, 3000);
        QVERIFY(repo.loaded());
        QVERIFY(!repo.supported());
        QCOMPARE(failed.count(), 0);
    }

    // A GatewaySet error surfaces operationFailed (never a silent no-op).
    void setErrorSurfaces() {
        const QString sock = m_tmp.filePath(QStringLiteral("gw-serr.sock"));
        WireMuxServer fake;
        QVERIFY2(fake.start(sock), "listen");
        fake.setReplyPayload(unsupportedError("cannot bind"));
        DaemonTransport transport;
        transport.setSocketPath(sock);
        NodeApiClient client(&transport);
        GatewayRepository repo(&client);

        QSignalSpy failed(&repo, &GatewayRepository::operationFailed);
        repo.setEnabled(false);
        QTRY_COMPARE_WITH_TIMEOUT(failed.count(), 1, 3000);
        QCOMPARE(failed.at(0).at(0).toString(), QStringLiteral("cannot bind"));
    }

    // DaemonConnectionService retains the last Health report as the single health cache and emits
    // healthChanged with the per-service entries populated.
    void connectionRetainsHealthServices() {
        const QString sock = m_tmp.filePath(QStringLiteral("gw-health.sock"));
        WireMuxServer fake;
        QVERIFY2(fake.start(sock), "listen");
        fake.setReplyPayload(
            healthResponse({QByteArrayLiteral("gateway"), QByteArrayLiteral("local-inference")}));

        DaemonConnectionService svc; // attach-only (no settings)
        QSignalSpy healthSpy(&svc, &DaemonConnectionService::healthChanged);
        svc.connectTo(QStringLiteral("local"), sock);
        QVERIFY(QTest::qWaitFor([&] { return svc.ready(); }, 5000));

        QVERIFY(healthSpy.count() >= 1);
        const auto services = svc.healthServices();
        QCOMPARE(services.size(), 2);
        QCOMPARE(services.at(0).name, QStringLiteral("gateway"));
        QVERIFY(services.at(0).ok);
        QCOMPARE(services.at(1).name, QStringLiteral("local-inference"));
    }
};

QTEST_GUILESS_MAIN(TestGatewayRepository)
#include "tst_gateway_repository.moc"
