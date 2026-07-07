// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// Durable inbox "allow permanently" path (wire v28):
//  - decodeApprovals surfaces ApprovalInfo.fingerprint presence as
//    DecodedApprovalInfo::hasFingerprint, which DaemonApprovalsInbox projects onto each row's
//    `canAllowPermanent` (the offer signal);
//  - inbox.approve(id, allowPermanent=true) forwards `allow_permanent` on the ApprovalDecide
//    request ONLY for a fingerprinted (offerable) row, and never for a non-fingerprinted one.

#include "daemon/daemon_approvals_inbox.h"
#include "daemon/daemon_cache_store.h"
#include "daemon/daemon_transport.h"
#include "daemon/node_api_client.h"
#include "daemon/node_api_codec.h"
#include "daemon/repositories.h"
#include "daemon_api_client_decode.h"
#include "daemon_api_client_encode.h"
#include "daemon_api_client_types.h"
#include "uimodels/variant_list_model.h"
#include "wire_mux_fixture.h"

#include <QTemporaryDir>
#include <QtTest/QtTest>

using daemonapp::daemon::ApprovalRepository;
using daemonapp::daemon::DaemonApprovalsInbox;
using daemonapp::daemon::DaemonCacheStore;
using daemonapp::daemon::DecodedApprovalInfo;
using daemonapp::daemon::NodeApiClient;
using daemonapp::daemon::NodeApiCodec;

namespace {

void setStr(zcbor_string& z, const QByteArray& b) {
    z.value = reinterpret_cast<const uint8_t*>(b.constData());
    z.len = static_cast<size_t>(b.size());
}

// An ApprovalsPending response with two rows: a-1 carries a fingerprint (node can remember it
// permanently), a-2 does not. Backing strings are static so they outlive the encode.
QByteArray approvalsResponse() {
    static const QByteArray s = QByteArrayLiteral("sess-1");
    static const QByteArray r1 = QByteArrayLiteral("a-1");
    static const QByteArray r2 = QByteArrayLiteral("a-2");
    static const QByteArray p1 = QByteArrayLiteral("run build");
    static const QByteArray p2 = QByteArrayLiteral("write ci.yml");
    static const QByteArray fp = QByteArrayLiteral("sha256:deadbeef");

    api_response_r resp{};
    resp.api_response_choice = api_response_r::api_response_response_approvals_m_c;
    approval_page& page = resp.api_response_response_approvals_m.response_approvals_Approvals;
    page.approval_page_items_approval_info_m_count = 2;

    approval_info& a1 = page.approval_page_items_approval_info_m[0];
    setStr(a1.approval_info_session, s);
    setStr(a1.approval_info_request_id, r1);
    setStr(a1.approval_info_prompt, p1);
    a1.approval_info_path_present = false;
    a1.approval_info_fingerprint_present = true;
    a1.approval_info_fingerprint.approval_info_fingerprint_choice =
        approval_info_fingerprint_r::approval_info_fingerprint_tstr_c;
    setStr(a1.approval_info_fingerprint.approval_info_fingerprint_tstr, fp);

    approval_info& a2 = page.approval_page_items_approval_info_m[1];
    setStr(a2.approval_info_session, s);
    setStr(a2.approval_info_request_id, r2);
    setStr(a2.approval_info_prompt, p2);
    a2.approval_info_path_present = false;
    a2.approval_info_fingerprint_present = false;

    page.approval_page_next_present = false;

    QByteArray out(64 * 1024, Qt::Uninitialized);
    size_t written = 0;
    if (cbor_encode_api_response(reinterpret_cast<uint8_t*>(out.data()),
                                 static_cast<size_t>(out.size()), &resp,
                                 &written) != ZCBOR_SUCCESS) {
        return {};
    }
    out.resize(static_cast<qsizetype>(written));
    return out;
}

// [waveB:app-v30] D5: one ApprovalInfo carrying an fs.diff ToolDetail (JSON body {path, diff}).
QByteArray approvalsResponseWithDiff() {
    static const QByteArray s = QByteArrayLiteral("sess-1");
    static const QByteArray r = QByteArrayLiteral("d-1");
    static const QByteArray p = QByteArrayLiteral("edit main.cpp");
    static const QByteArray kind = QByteArrayLiteral("fs.diff");
    static const QByteArray body =
        QByteArrayLiteral("{\"path\":\"main.cpp\",\"diff\":\"--- a/main.cpp\\n+++ b/main.cpp\\n@@ "
                          "-1 +1 @@\\n-a\\n+b\\n\"}");

    api_response_r resp{};
    resp.api_response_choice = api_response_r::api_response_response_approvals_m_c;
    approval_page& page = resp.api_response_response_approvals_m.response_approvals_Approvals;
    page.approval_page_items_approval_info_m_count = 1;
    approval_info& a = page.approval_page_items_approval_info_m[0];
    setStr(a.approval_info_session, s);
    setStr(a.approval_info_request_id, r);
    setStr(a.approval_info_prompt, p);
    a.approval_info_path_present = false;
    a.approval_info_fingerprint_present = false;
    a.approval_info_detail_present = true;
    tool_detail& td = a.approval_info_detail.approval_info_detail_tool_detail_m;
    setStr(td.tool_detail_kind, kind);
    setStr(td.tool_detail_body, body);
    page.approval_page_next_present = false;

    QByteArray out(64 * 1024, Qt::Uninitialized);
    size_t written = 0;
    if (cbor_encode_api_response(reinterpret_cast<uint8_t*>(out.data()),
                                 static_cast<size_t>(out.size()), &resp,
                                 &written) != ZCBOR_SUCCESS) {
        return {};
    }
    out.resize(static_cast<qsizetype>(written));
    return out;
}

// If `cbor` is an ApprovalDecide request, return its request_id + whether allow_permanent is set.
struct DecidedWire {
    bool isDecide = false;
    QString requestId;
    bool allow = false;
    bool allowPermanentPresent = false;
    bool allowPermanent = false;
    bool reasonPresent = false; // [wave2:app-approvals-safety] D3
    QString reason;
};

DecidedWire decodeDecide(const QByteArray& cbor) {
    DecidedWire d;
    api_request_r req{};
    size_t consumed = 0;
    if (cbor_decode_api_request(reinterpret_cast<const uint8_t*>(cbor.constData()),
                                static_cast<size_t>(cbor.size()), &req,
                                &consumed) != ZCBOR_SUCCESS) {
        return d;
    }
    if (req.api_request_choice != api_request_r::api_request_request_approval_decide_m_c) {
        return d;
    }
    const request_approval_decide& dec = req.api_request_request_approval_decide_m;
    d.isDecide = true;
    d.requestId =
        QString::fromUtf8(reinterpret_cast<const char*>(dec.ApprovalDecide_request_id.value),
                          static_cast<int>(dec.ApprovalDecide_request_id.len));
    d.allow = dec.ApprovalDecide_allow;
    d.allowPermanentPresent = dec.ApprovalDecide_allow_permanent_present;
    d.allowPermanent = dec.ApprovalDecide_allow_permanent.ApprovalDecide_allow_permanent;
    d.reasonPresent = dec.ApprovalDecide_reason_present;
    if (d.reasonPresent) {
        d.reason = QString::fromUtf8(
            reinterpret_cast<const char*>(
                dec.ApprovalDecide_reason.ApprovalDecide_reason_tstr.value),
            static_cast<int>(dec.ApprovalDecide_reason.ApprovalDecide_reason_tstr.len));
    }
    return d;
}

} // namespace

class TestDaemonApprovalsInbox : public QObject {
    Q_OBJECT

private slots:
    // The mapper surfaces ApprovalInfo.fingerprint presence as hasFingerprint (the offer signal).
    void decodeSurfacesFingerprintPresence() {
        const QByteArray cbor = approvalsResponse();
        QVERIFY(!cbor.isEmpty());
        QList<DecodedApprovalInfo> pending;
        QString next;
        QVERIFY(NodeApiCodec::decodeApprovals(cbor, &pending, &next));
        QCOMPARE(pending.size(), 2);
        QCOMPARE(pending.at(0).requestId, QStringLiteral("a-1"));
        QVERIFY(pending.at(0).hasFingerprint); // fingerprinted -> offerable
        // [wave2:app-approvals-safety] D3: the fingerprint hex string is carried for the chip.
        QCOMPARE(pending.at(0).fingerprint, QStringLiteral("sha256:deadbeef"));
        QCOMPARE(pending.at(1).requestId, QStringLiteral("a-2"));
        QVERIFY(!pending.at(1).hasFingerprint); // no fingerprint -> not offerable
        QVERIFY(pending.at(1).fingerprint.isEmpty());
    }

    // [waveB:app-v30] D5: an fs.diff ToolDetail decodes into detailKind + the raw JSON body, and
    // the inbox projects the parsed {path, diff} onto the row; an unknown/absent detail degrades
    // (detailKind empty, no diff).
    void diffDetailDecodesAndProjects() {
        const QByteArray cbor = approvalsResponseWithDiff();
        QVERIFY(!cbor.isEmpty());
        QList<DecodedApprovalInfo> pending;
        QVERIFY(NodeApiCodec::decodeApprovals(cbor, &pending, nullptr));
        QCOMPARE(pending.size(), 1);
        QCOMPARE(pending.at(0).detailKind, QStringLiteral("fs.diff"));
        QVERIFY(pending.at(0).detailBody.contains("main.cpp"));

        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = dir.filePath(QStringLiteral("approvals.sock"));
        daemonapp::test::WireMuxServer fake;
        fake.setReplyPayload(cbor);
        QVERIFY2(fake.start(path), "listen");

        DaemonCacheStore cache(dir.filePath(QStringLiteral("ai-diff.db")));
        daemonapp::daemon::DaemonTransport transport;
        transport.setSocketPath(path);
        NodeApiClient client(&transport);
        ApprovalRepository repo(&client, &cache);
        DaemonApprovalsInbox inbox(&repo);

        repo.refreshPending();
        auto* rows = qobject_cast<uimodels::VariantListModel*>(inbox.pending());
        QVERIFY(rows != nullptr);
        QTRY_COMPARE_WITH_TIMEOUT(rows->count(), 1, 3000);
        const int i = rows->indexOfId(QStringLiteral("d-1"));
        QVERIFY(i >= 0);
        QCOMPARE(rows->at(i).value(QStringLiteral("detailKind")).toString(),
                 QStringLiteral("fs.diff"));
        QCOMPARE(rows->at(i).value(QStringLiteral("diffPath")).toString(),
                 QStringLiteral("main.cpp"));
        QVERIFY(
            rows->at(i).value(QStringLiteral("diff")).toString().contains(QStringLiteral("+b")));
    }

    // [wave2:app-approvals-safety] D3: deny-with-reason threads ApprovalDecide.reason on a deny;
    // a plain deny leaves it absent.
    void inboxDenyForwardsReason() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = dir.filePath(QStringLiteral("approvals.sock"));
        daemonapp::test::WireMuxServer fake;
        fake.setReplyPayload(approvalsResponse());
        QVERIFY2(fake.start(path), "listen");

        DaemonCacheStore cache(dir.filePath(QStringLiteral("ai.db")));
        daemonapp::daemon::DaemonTransport transport;
        transport.setSocketPath(path);
        NodeApiClient client(&transport);
        ApprovalRepository repo(&client, &cache);
        DaemonApprovalsInbox inbox(&repo);

        repo.refreshPending();
        auto* rows = qobject_cast<uimodels::VariantListModel*>(inbox.pending());
        QVERIFY(rows != nullptr);
        QTRY_COMPARE_WITH_TIMEOUT(rows->count(), 2, 3000);

        inbox.deny(QStringLiteral("a-1"), QStringLiteral("wrong directory"));
        inbox.deny(QStringLiteral("a-2"), QString()); // plain deny

        QTRY_VERIFY_WITH_TIMEOUT(
            [&] {
                int denies = 0;
                for (const QByteArray& p : fake.callPayloads()) {
                    const DecidedWire d = decodeDecide(p);
                    if (d.isDecide && !d.allow) {
                        ++denies;
                    }
                }
                return denies >= 2;
            }(),
            3000);

        DecidedWire d1;
        DecidedWire d2;
        for (const QByteArray& p : fake.callPayloads()) {
            const DecidedWire d = decodeDecide(p);
            if (!d.isDecide) {
                continue;
            }
            if (d.requestId == QStringLiteral("a-1")) {
                d1 = d;
            } else if (d.requestId == QStringLiteral("a-2")) {
                d2 = d;
            }
        }
        QVERIFY(d1.isDecide);
        QCOMPARE(d1.allow, false);
        QVERIFY(d1.reasonPresent);
        QCOMPARE(d1.reason, QStringLiteral("wrong directory"));

        QVERIFY(d2.isDecide);
        QCOMPARE(d2.allow, false);
        QVERIFY(!d2.reasonPresent); // empty reason -> absent
    }

    // End-to-end: the inbox projects canAllowPermanent onto rows and forwards allow_permanent on
    // ApprovalDecide only for the offerable (fingerprinted) row.
    void inboxForwardsAllowPermanentOnlyWhenOffered() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = dir.filePath(QStringLiteral("approvals.sock"));
        daemonapp::test::WireMuxServer fake;
        fake.setReplyPayload(approvalsResponse());
        QVERIFY2(fake.start(path), "listen");

        DaemonCacheStore cache(dir.filePath(QStringLiteral("ai.db")));
        daemonapp::daemon::DaemonTransport transport;
        transport.setSocketPath(path);
        NodeApiClient client(&transport);
        ApprovalRepository repo(&client, &cache);
        DaemonApprovalsInbox inbox(&repo);

        repo.refreshPending();

        auto* rows = qobject_cast<uimodels::VariantListModel*>(inbox.pending());
        QVERIFY(rows != nullptr);
        QTRY_COMPARE_WITH_TIMEOUT(rows->count(), 2, 3000);

        // The offer signal reaches the rows: a-1 offerable, a-2 not.
        const int i1 = rows->indexOfId(QStringLiteral("a-1"));
        const int i2 = rows->indexOfId(QStringLiteral("a-2"));
        QVERIFY(i1 >= 0 && i2 >= 0);
        QCOMPARE(rows->at(i1).value(QStringLiteral("canAllowPermanent")).toBool(), true);
        QCOMPARE(rows->at(i2).value(QStringLiteral("canAllowPermanent")).toBool(), false);

        // "Allow permanently" on both; only the offerable row forwards allow_permanent.
        inbox.approve(QStringLiteral("a-1"), /*allowPermanent=*/true);
        inbox.approve(QStringLiteral("a-2"), /*allowPermanent=*/true);

        QTRY_VERIFY_WITH_TIMEOUT(
            [&] {
                int decides = 0;
                for (const QByteArray& p : fake.callPayloads()) {
                    if (decodeDecide(p).isDecide) {
                        ++decides;
                    }
                }
                return decides >= 2;
            }(),
            3000);

        DecidedWire d1;
        DecidedWire d2;
        for (const QByteArray& p : fake.callPayloads()) {
            const DecidedWire d = decodeDecide(p);
            if (!d.isDecide) {
                continue;
            }
            if (d.requestId == QStringLiteral("a-1")) {
                d1 = d;
            } else if (d.requestId == QStringLiteral("a-2")) {
                d2 = d;
            }
        }
        QVERIFY(d1.isDecide);
        QCOMPARE(d1.allow, true);
        QVERIFY(d1.allowPermanentPresent); // fingerprinted -> permanence forwarded
        QCOMPARE(d1.allowPermanent, true);

        QVERIFY(d2.isDecide);
        QCOMPARE(d2.allow, true);
        QVERIFY(!d2.allowPermanentPresent); // not fingerprinted -> offer-gated, absent
    }
};

QTEST_GUILESS_MAIN(TestDaemonApprovalsInbox)
#include "tst_daemon_approvals_inbox.moc"
