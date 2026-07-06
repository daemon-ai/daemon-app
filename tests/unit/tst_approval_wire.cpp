// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// Wire-level guard for the "allow permanently" plumbing (wire v28) on BOTH approval surfaces:
//  - inline: the Respond `Approved` body carries the optional `allow_permanent` only when the
//    operator chose it, so a plain approve stays byte-shape-identical to pre-v28;
//  - durable: the inbox `ApprovalDecide` request carries the same optional `allow_permanent`.
// Encodes via NodeApiCodec and decodes the bytes back with the vendored codec's public request
// decoder to assert the on-wire fields directly.

#include "daemon/node_api_codec.h"
#include "daemon_api_client_decode.h"
#include "daemon_api_client_types.h"

#include <QtTest/QtTest>

using daemonapp::daemon::NodeApiCodec;

namespace {

// Decode an encoded api-request back into the generated struct; asserts it is a
// Respond carrying an Approved body and returns that body by out-param.
bool decodeApprovedBody(const QByteArray& cbor, host_response_body_approved* out) {
    api_request_r req{};
    size_t consumed = 0;
    const int rc = cbor_decode_api_request(reinterpret_cast<const uint8_t*>(cbor.constData()),
                                           static_cast<size_t>(cbor.size()), &req, &consumed);
    if (rc != ZCBOR_SUCCESS) {
        return false;
    }
    if (req.api_request_choice != api_request_r::api_request_request_respond_m_c) {
        return false;
    }
    const host_response_body_t_r& body =
        req.api_request_request_respond_m.Respond_response.host_response_body;
    if (body.host_response_body_t_choice !=
        host_response_body_t_r::host_response_body_t_host_response_body_approved_m_c) {
        return false;
    }
    *out = body.host_response_body_t_host_response_body_approved_m;
    return true;
}

// Decode an encoded api-request; asserts it is an ApprovalDecide (the durable inbox path) and
// returns it by out-param.
bool decodeApprovalDecide(const QByteArray& cbor, request_approval_decide* out) {
    api_request_r req{};
    size_t consumed = 0;
    const int rc = cbor_decode_api_request(reinterpret_cast<const uint8_t*>(cbor.constData()),
                                           static_cast<size_t>(cbor.size()), &req, &consumed);
    if (rc != ZCBOR_SUCCESS) {
        return false;
    }
    if (req.api_request_choice != api_request_r::api_request_request_approval_decide_m_c) {
        return false;
    }
    *out = req.api_request_request_approval_decide_m;
    return true;
}

} // namespace

class TestApprovalWire : public QObject {
    Q_OBJECT

private slots:
    // "Allow permanently" on an offered gate rides allow_permanent:true.
    void allowPermanentSetsWireField() {
        const QByteArray cbor = NodeApiCodec::encodeRespondApprovalRequest(
            QStringLiteral("s1"), 7, /*allow=*/true, /*allowPermanent=*/true);
        QVERIFY(!cbor.isEmpty());
        host_response_body_approved body{};
        QVERIFY(decodeApprovedBody(cbor, &body));
        QCOMPARE(body.Approved_approved, true);
        QVERIFY(body.Approved_allow_permanent_present);
        QCOMPARE(body.Approved_allow_permanent.Approved_allow_permanent, true);
    }

    // A plain approve leaves allow_permanent absent (byte-shape parity with pre-v28).
    void plainApproveOmitsAllowPermanent() {
        const QByteArray cbor = NodeApiCodec::encodeRespondApprovalRequest(
            QStringLiteral("s1"), 7, /*allow=*/true, /*allowPermanent=*/false);
        QVERIFY(!cbor.isEmpty());
        host_response_body_approved body{};
        QVERIFY(decodeApprovedBody(cbor, &body));
        QCOMPARE(body.Approved_approved, true);
        QVERIFY(!body.Approved_allow_permanent_present);

        // The default overload (no flag) is identical to an explicit false.
        const QByteArray def =
            NodeApiCodec::encodeRespondApprovalRequest(QStringLiteral("s1"), 7, /*allow=*/true);
        QCOMPARE(def, cbor);
    }

    // A deny never carries a permanent decision, even if the flag is passed.
    void denyNeverPermanent() {
        const QByteArray cbor = NodeApiCodec::encodeRespondApprovalRequest(
            QStringLiteral("s1"), 7, /*allow=*/false, /*allowPermanent=*/true);
        QVERIFY(!cbor.isEmpty());
        host_response_body_approved body{};
        QVERIFY(decodeApprovedBody(cbor, &body));
        QCOMPARE(body.Approved_approved, false);
        QVERIFY(!body.Approved_allow_permanent_present);
    }

    // --- durable inbox path (ApprovalDecide) ---

    // "Allow permanently" from the inbox rides ApprovalDecide.allow_permanent:true.
    void decideAllowPermanentSetsWireField() {
        const QByteArray cbor = NodeApiCodec::encodeApprovalDecideRequest(
            QStringLiteral("s1"), QStringLiteral("req-1"), /*allow=*/true, /*allowPermanent=*/true);
        QVERIFY(!cbor.isEmpty());
        request_approval_decide decide{};
        QVERIFY(decodeApprovalDecide(cbor, &decide));
        QCOMPARE(decide.ApprovalDecide_allow, true);
        QVERIFY(decide.ApprovalDecide_allow_permanent_present);
        QCOMPARE(decide.ApprovalDecide_allow_permanent.ApprovalDecide_allow_permanent, true);
    }

    // A plain inbox approve leaves allow_permanent absent, and the default overload matches.
    void decidePlainApproveOmitsAllowPermanent() {
        const QByteArray cbor =
            NodeApiCodec::encodeApprovalDecideRequest(QStringLiteral("s1"), QStringLiteral("req-1"),
                                                      /*allow=*/true, /*allowPermanent=*/false);
        QVERIFY(!cbor.isEmpty());
        request_approval_decide decide{};
        QVERIFY(decodeApprovalDecide(cbor, &decide));
        QCOMPARE(decide.ApprovalDecide_allow, true);
        QVERIFY(!decide.ApprovalDecide_allow_permanent_present);

        const QByteArray def = NodeApiCodec::encodeApprovalDecideRequest(
            QStringLiteral("s1"), QStringLiteral("req-1"), /*allow=*/true);
        QCOMPARE(def, cbor);
    }

    // A durable deny never carries a permanent decision, even if the flag is passed.
    void decideDenyNeverPermanent() {
        const QByteArray cbor =
            NodeApiCodec::encodeApprovalDecideRequest(QStringLiteral("s1"), QStringLiteral("req-1"),
                                                      /*allow=*/false, /*allowPermanent=*/true);
        QVERIFY(!cbor.isEmpty());
        request_approval_decide decide{};
        QVERIFY(decodeApprovalDecide(cbor, &decide));
        QCOMPARE(decide.ApprovalDecide_allow, false);
        QVERIFY(!decide.ApprovalDecide_allow_permanent_present);
    }

    // --- [wave2:app-approvals-safety] D3: deny-with-reason (wire v29) ---

    // A deny with a reason rides ApprovalDecide.reason (durable inbox path).
    void decideDenyReasonSetsWireField() {
        const QByteArray cbor = NodeApiCodec::encodeApprovalDecideRequest(
            QStringLiteral("s1"), QStringLiteral("req-1"), /*allow=*/false,
            /*allowPermanent=*/false, QStringLiteral("not this path"));
        QVERIFY(!cbor.isEmpty());
        request_approval_decide decide{};
        QVERIFY(decodeApprovalDecide(cbor, &decide));
        QCOMPARE(decide.ApprovalDecide_allow, false);
        QVERIFY(decide.ApprovalDecide_reason_present);
        const QByteArray reason(
            reinterpret_cast<const char*>(
                decide.ApprovalDecide_reason.ApprovalDecide_reason_tstr.value),
            static_cast<int>(decide.ApprovalDecide_reason.ApprovalDecide_reason_tstr.len));
        QCOMPARE(reason, QByteArray("not this path"));
    }

    // A deny reason rides the inline Approved.reason too.
    void respondDenyReasonSetsWireField() {
        const QByteArray cbor = NodeApiCodec::encodeRespondApprovalRequest(
            QStringLiteral("s1"), 7, /*allow=*/false, /*allowPermanent=*/false,
            QStringLiteral("wrong dir"));
        QVERIFY(!cbor.isEmpty());
        host_response_body_approved body{};
        QVERIFY(decodeApprovedBody(cbor, &body));
        QCOMPARE(body.Approved_approved, false);
        QVERIFY(body.Approved_reason_present);
        const QByteArray reason(
            reinterpret_cast<const char*>(body.Approved_reason.Approved_reason_tstr.value),
            static_cast<int>(body.Approved_reason.Approved_reason_tstr.len));
        QCOMPARE(reason, QByteArray("wrong dir"));
    }

    // An empty reason (and an ALLOW even with a reason) leaves the field absent — no fabricated
    // reason on the wire, and byte-shape parity with a plain decision.
    void reasonAbsentWhenEmptyOrAllow() {
        const QByteArray denyNoReason = NodeApiCodec::encodeApprovalDecideRequest(
            QStringLiteral("s1"), QStringLiteral("req-1"), /*allow=*/false,
            /*allowPermanent=*/false, QString());
        request_approval_decide d1{};
        QVERIFY(decodeApprovalDecide(denyNoReason, &d1));
        QVERIFY(!d1.ApprovalDecide_reason_present);
        // Default overload (no reason arg) is byte-identical.
        const QByteArray def = NodeApiCodec::encodeApprovalDecideRequest(
            QStringLiteral("s1"), QStringLiteral("req-1"), /*allow=*/false);
        QCOMPARE(def, denyNoReason);

        // An allow carrying a reason still omits it (reason is deny-only).
        const QByteArray allowWithReason = NodeApiCodec::encodeApprovalDecideRequest(
            QStringLiteral("s1"), QStringLiteral("req-1"), /*allow=*/true, /*allowPermanent=*/false,
            QStringLiteral("ignored"));
        request_approval_decide d2{};
        QVERIFY(decodeApprovalDecide(allowWithReason, &d2));
        QVERIFY(!d2.ApprovalDecide_reason_present);
    }
};

QTEST_GUILESS_MAIN(TestApprovalWire)
#include "tst_approval_wire.moc"
