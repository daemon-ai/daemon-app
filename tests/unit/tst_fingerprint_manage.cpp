// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// [wave2:app-approvals-safety] D4: wire guard for per-session remembered-fingerprint management
// (FingerprintList / FingerprintRevoke -> Fingerprints, wire v29). Encodes the two requests via
// NodeApiCodec and decodes them back with the vendored request decoder to assert the session +
// fingerprint fields; round-trips a Fingerprints response through NodeApiCodec::decodeFingerprints.

#include "daemon/node_api_codec.h"
#include "daemon_api_client_decode.h"
#include "daemon_api_client_encode.h"
#include "daemon_api_client_types.h"

#include <QtTest/QtTest>
#include <vector>

using daemonapp::daemon::DecodedRememberedFingerprint;
using daemonapp::daemon::NodeApiCodec;

namespace {

QByteArray zstr(const zcbor_string& z) {
    return {reinterpret_cast<const char*>(z.value), static_cast<int>(z.len)};
}

bool decodeRequest(const QByteArray& cbor, api_request_r* out) {
    size_t consumed = 0;
    return cbor_decode_api_request(reinterpret_cast<const uint8_t*>(cbor.constData()),
                                   static_cast<size_t>(cbor.size()), out,
                                   &consumed) == ZCBOR_SUCCESS;
}

QByteArray encodeFingerprintsResponse(const std::vector<QByteArray>& fps) {
    api_response_r resp{};
    resp.api_response_choice = api_response_r::api_response_response_fingerprints_m_c;
    response_fingerprints& out = resp.api_response_response_fingerprints_m;
    out.response_fingerprints_Fingerprints_remembered_fingerprint_m_count = fps.size();
    for (size_t i = 0; i < fps.size(); ++i) {
        remembered_fingerprint& rf =
            out.response_fingerprints_Fingerprints_remembered_fingerprint_m[i];
        rf.remembered_fingerprint_fingerprint.value =
            reinterpret_cast<const uint8_t*>(fps[i].constData());
        rf.remembered_fingerprint_fingerprint.len = static_cast<size_t>(fps[i].size());
        rf.remembered_fingerprint_label_present = false; // node stores only the hash today
    }
    QByteArray buf(8192, Qt::Uninitialized);
    size_t written = 0;
    if (cbor_encode_api_response(reinterpret_cast<uint8_t*>(buf.data()),
                                 static_cast<size_t>(buf.size()), &resp,
                                 &written) != ZCBOR_SUCCESS) {
        return {};
    }
    buf.truncate(static_cast<int>(written));
    return buf;
}

} // namespace

class TestFingerprintManage : public QObject {
    Q_OBJECT

private slots:
    // FingerprintList carries the session it scopes to.
    void listRequestCarriesSession() {
        const QByteArray cbor =
            NodeApiCodec::encodeFingerprintListRequest(QStringLiteral("sess-9"));
        QVERIFY(!cbor.isEmpty());
        api_request_r req{};
        QVERIFY(decodeRequest(cbor, &req));
        QCOMPARE(req.api_request_choice, api_request_r::api_request_request_fingerprint_list_m_c);
        QCOMPARE(zstr(req.api_request_request_fingerprint_list_m.FingerprintList_session),
                 QByteArray("sess-9"));
    }

    // FingerprintRevoke carries the session and the exact fingerprint to drop.
    void revokeRequestCarriesSessionAndFingerprint() {
        const QByteArray cbor = NodeApiCodec::encodeFingerprintRevokeRequest(
            QStringLiteral("sess-9"), QStringLiteral("abc123def456"));
        QVERIFY(!cbor.isEmpty());
        api_request_r req{};
        QVERIFY(decodeRequest(cbor, &req));
        QCOMPARE(req.api_request_choice, api_request_r::api_request_request_fingerprint_revoke_m_c);
        QCOMPARE(zstr(req.api_request_request_fingerprint_revoke_m.FingerprintRevoke_session),
                 QByteArray("sess-9"));
        QCOMPARE(zstr(req.api_request_request_fingerprint_revoke_m.FingerprintRevoke_fingerprint),
                 QByteArray("abc123def456"));
    }

    // A Fingerprints response decodes into the remembered list; label is absent today (the node
    // stores only the hash) — the decoder must not fabricate one.
    void decodesFingerprints() {
        const std::vector<QByteArray> fps{QByteArray("aaaa1111bbbb2222"),
                                          QByteArray("cccc3333dddd4444")};
        const QByteArray cbor = encodeFingerprintsResponse(fps);
        QVERIFY(!cbor.isEmpty());
        QList<DecodedRememberedFingerprint> decoded;
        QVERIFY(NodeApiCodec::decodeFingerprints(cbor, &decoded));
        QCOMPARE(decoded.size(), 2);
        QCOMPARE(decoded[0].fingerprint, QStringLiteral("aaaa1111bbbb2222"));
        QVERIFY(decoded[0].label.isEmpty());
        QCOMPARE(decoded[1].fingerprint, QStringLiteral("cccc3333dddd4444"));
    }
};

QTEST_GUILESS_MAIN(TestFingerprintManage)
#include "tst_fingerprint_manage.moc"
