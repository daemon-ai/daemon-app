// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// Durable checkpoints (E4/TOOL-9):
//  - CheckpointRepository lists a session's checkpoints (CheckpointList page loop across `after`
//    cursors) and rewinds (CheckpointRewind -> Ok => rewound + re-fetch; Error => operationFailed);
//  - DaemonCheckpointTimeline projects the rows into the ICheckpointTimeline model the popover +
//    timeline strip bind (label "before <tool>", tokens -1 = unknown, newest = current) and
//    refuses createCheckpoint (no wire op).

#include "daemon/daemon_cache_store.h"
#include "daemon/daemon_checkpoint_timeline.h"
#include "daemon/daemon_transport.h"
#include "daemon/node_api_client.h"
#include "daemon/node_api_codec.h"
#include "daemon/repositories.h"
#include "daemon_api_client_decode.h"
#include "daemon_api_client_encode.h"
#include "daemon_api_client_types.h"
#include "uimodels/variant_list_model.h"
#include "wire_mux_fixture.h"

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

using daemonapp::daemon::CheckpointRepository;
using daemonapp::daemon::DaemonCacheStore;
using daemonapp::daemon::DaemonCheckpointTimeline;
using daemonapp::daemon::DaemonTransport;
using daemonapp::daemon::DecodedCheckpointInfo;
using daemonapp::daemon::NodeApiClient;
using daemonapp::daemon::NodeApiCodec;
using daemonapp::test::WireMuxServer;

namespace {

void setStr(zcbor_string& z, const QByteArray& b) {
    z.value = reinterpret_cast<const uint8_t*>(b.constData());
    z.len = static_cast<size_t>(b.size());
}

// One Checkpoints page via the generated encoder. Backing strings are static so they outlive the
// encode. `withNext` sets the resume cursor to "cp-1" (page loop continuation).
QByteArray checkpointsResponse(const QList<QByteArray>& ids, bool withNext) {
    static const QByteArray session = QByteArrayLiteral("sess-1");
    static const QByteArray tool = QByteArrayLiteral("fs_write");
    static const QByteArray nextCur = QByteArrayLiteral("cp-1");

    api_response_r resp{};
    resp.api_response_choice = api_response_r::api_response_response_checkpoints_m_c;
    checkpoint_page& page =
        resp.api_response_response_checkpoints_m.response_checkpoints_Checkpoints;
    page.checkpoint_page_items_checkpoint_info_m_count = static_cast<size_t>(ids.size());
    for (int i = 0; i < ids.size(); ++i) {
        checkpoint_info& c = page.checkpoint_page_items_checkpoint_info_m[i];
        setStr(c.checkpoint_info_id, ids.at(i));
        setStr(c.checkpoint_info_session, session);
        setStr(c.checkpoint_info_tool, tool);
        c.checkpoint_info_created_unix = 1700000000 + static_cast<uint64_t>(i);
        c.checkpoint_info_turn_ordinal_present = true;
        c.checkpoint_info_turn_ordinal.checkpoint_info_turn_ordinal_choice =
            checkpoint_info_turn_ordinal_r::checkpoint_info_turn_ordinal_uint64_m_c;
        c.checkpoint_info_turn_ordinal.checkpoint_info_turn_ordinal_uint64_m =
            static_cast<uint64_t>(i);
        c.checkpoint_info_cursor_present = false;
    }
    page.checkpoint_page_next_present = withNext;
    if (withNext) {
        page.checkpoint_page_next.checkpoint_page_next_choice =
            checkpoint_page_next_r::checkpoint_page_next_tstr_c;
        setStr(page.checkpoint_page_next.checkpoint_page_next_tstr, nextCur);
    }

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

// ApiResponse::Ok is the externally-tagged unit variant -> the bare CBOR text string "Ok".
QByteArray okResponse() {
    QByteArray b;
    daemonapp::test::cborText(b, "Ok");
    return b;
}

// {"Error": {"Unsupported": "no such checkpoint"}}
QByteArray errorResponse() {
    QByteArray b;
    b.append(static_cast<char>(0xA1));
    daemonapp::test::cborText(b, "Error");
    b.append(static_cast<char>(0xA1));
    daemonapp::test::cborText(b, "Unsupported");
    daemonapp::test::cborText(b, "no such checkpoint");
    return b;
}

} // namespace

class TestCheckpointRepository : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_tmp;

private slots:
    // A CheckpointList round-trip populates checkpoints() (single page).
    void refreshPopulates() {
        const QString sock = m_tmp.filePath(QStringLiteral("cp.sock"));
        WireMuxServer fake;
        QVERIFY2(fake.start(sock), "listen");
        fake.setReplyPayload(
            checkpointsResponse({QByteArrayLiteral("cp-1"), QByteArrayLiteral("cp-2")}, false));
        DaemonTransport transport;
        transport.setSocketPath(sock);
        NodeApiClient client(&transport);
        DaemonCacheStore cache(m_tmp.filePath(QStringLiteral("cp.db")));
        CheckpointRepository repo(&client, &cache);

        QSignalSpy refreshed(&repo, &CheckpointRepository::checkpointsRefreshed);
        repo.refresh(QStringLiteral("sess-1"));
        QTRY_COMPARE_WITH_TIMEOUT(refreshed.count(), 1, 3000);
        QCOMPARE(refreshed.at(0).at(0).toString(), QStringLiteral("sess-1"));
        QCOMPARE(repo.checkpoints().size(), 2);
        QCOMPARE(repo.checkpoints().at(0).id, QStringLiteral("cp-1"));
        QCOMPARE(repo.checkpoints().at(0).tool, QStringLiteral("fs_write"));
        QVERIFY(repo.checkpoints().at(0).hasTurnOrdinal);
        // The list request carried the session scope: byte-identical to the encoder's output.
        const QList<QByteArray> calls = fake.callPayloads();
        QCOMPARE(calls.size(), 1);
        QCOMPARE(calls.at(0), NodeApiCodec::encodeCheckpointListRequest(QStringLiteral("sess-1")));
    }

    // Wire v25 page loop: two pages accumulate into ONE checkpointsRefreshed; the continuation
    // request carries the `after` cursor.
    void pagesReassembleAcrossTheCursorLoop() {
        const QString sock = m_tmp.filePath(QStringLiteral("cp-pg.sock"));
        WireMuxServer fake;
        QVERIFY2(fake.start(sock), "listen");
        fake.setReplySequence({checkpointsResponse({QByteArrayLiteral("cp-1")}, true),
                               checkpointsResponse({QByteArrayLiteral("cp-2")}, false)});
        DaemonTransport transport;
        transport.setSocketPath(sock);
        NodeApiClient client(&transport);
        DaemonCacheStore cache(m_tmp.filePath(QStringLiteral("cp-pg.db")));
        CheckpointRepository repo(&client, &cache);

        QSignalSpy refreshed(&repo, &CheckpointRepository::checkpointsRefreshed);
        repo.refresh(QStringLiteral("sess-1"));
        QTRY_COMPARE_WITH_TIMEOUT(refreshed.count(), 1, 3000);
        QTest::qWait(100); // settle: a page-loop bug would emit again / issue extra requests
        QCOMPARE(refreshed.count(), 1);
        QCOMPARE(repo.checkpoints().size(), 2);

        const QList<QByteArray> calls = fake.callPayloads();
        QCOMPARE(calls.size(), 2);
        QCOMPARE(calls.at(1), NodeApiCodec::encodeCheckpointListRequest(QStringLiteral("sess-1"),
                                                                        QStringLiteral("cp-1")));
    }

    // CheckpointRewind -> Ok emits rewound + re-fetches the (pruned) timeline.
    void rewindOkRefetches() {
        const QString sock = m_tmp.filePath(QStringLiteral("cp-rw.sock"));
        WireMuxServer fake;
        QVERIFY2(fake.start(sock), "listen");
        fake.setReplySequence(
            {okResponse(), checkpointsResponse({QByteArrayLiteral("cp-1")}, false)});
        DaemonTransport transport;
        transport.setSocketPath(sock);
        NodeApiClient client(&transport);
        DaemonCacheStore cache(m_tmp.filePath(QStringLiteral("cp-rw.db")));
        CheckpointRepository repo(&client, &cache);

        QSignalSpy rewound(&repo, &CheckpointRepository::rewound);
        QSignalSpy refreshed(&repo, &CheckpointRepository::checkpointsRefreshed);
        repo.rewind(QStringLiteral("sess-1"), QStringLiteral("cp-1"));
        QTRY_COMPARE_WITH_TIMEOUT(rewound.count(), 1, 3000);
        QCOMPARE(rewound.at(0).at(0).toString(), QStringLiteral("sess-1"));
        QTRY_COMPARE_WITH_TIMEOUT(refreshed.count(), 1, 3000);
        const QList<QByteArray> calls = fake.callPayloads();
        QCOMPARE(calls.at(0), NodeApiCodec::encodeCheckpointRewindRequest(QStringLiteral("sess-1"),
                                                                          QStringLiteral("cp-1")));
    }

    // CheckpointRewind -> Error surfaces operationFailed (never a silent no-op).
    void rewindErrorSurfaces() {
        const QString sock = m_tmp.filePath(QStringLiteral("cp-err.sock"));
        WireMuxServer fake;
        QVERIFY2(fake.start(sock), "listen");
        fake.setReplyPayload(errorResponse());
        DaemonTransport transport;
        transport.setSocketPath(sock);
        NodeApiClient client(&transport);
        DaemonCacheStore cache(m_tmp.filePath(QStringLiteral("cp-err.db")));
        CheckpointRepository repo(&client, &cache);

        QSignalSpy failed(&repo, &CheckpointRepository::operationFailed);
        repo.rewind(QStringLiteral("sess-1"), QStringLiteral("cp-9"));
        QTRY_COMPARE_WITH_TIMEOUT(failed.count(), 1, 3000);
        QCOMPARE(failed.at(0).at(0).toString(), QStringLiteral("no such checkpoint"));
    }

    // The facade projects repo rows into popover/strip rows and refuses manual creates.
    void timelineProjectsRows() {
        const QString sock = m_tmp.filePath(QStringLiteral("cp-tl.sock"));
        WireMuxServer fake;
        QVERIFY2(fake.start(sock), "listen");
        fake.setReplyPayload(
            checkpointsResponse({QByteArrayLiteral("cp-1"), QByteArrayLiteral("cp-2")}, false));
        DaemonTransport transport;
        transport.setSocketPath(sock);
        NodeApiClient client(&transport);
        DaemonCacheStore cache(m_tmp.filePath(QStringLiteral("cp-tl.db")));
        CheckpointRepository repo(&client, &cache);
        DaemonCheckpointTimeline timeline(&repo);

        QVERIFY(!timeline.canCreate());
        QCOMPARE(timeline.createCheckpoint(QStringLiteral("manual")), QString());

        QSignalSpy changed(&timeline, &session::ICheckpointTimeline::changed);
        timeline.setSessionId(QStringLiteral("sess-1"));
        QTRY_VERIFY_WITH_TIMEOUT(timeline.count() == 2, 3000);
        Q_UNUSED(changed)

        auto* model = qobject_cast<uimodels::VariantListModel*>(timeline.checkpoints());
        QVERIFY(model != nullptr);
        const QVariantMap first = model->at(0);
        QCOMPARE(first.value(QStringLiteral("id")).toString(), QStringLiteral("cp-1"));
        QCOMPARE(first.value(QStringLiteral("label")).toString(),
                 QStringLiteral("before fs_write"));
        QCOMPARE(first.value(QStringLiteral("tokens")).toInt(), -1); // unknown -> badge omitted
        QCOMPARE(first.value(QStringLiteral("current")).toBool(), false);
        QCOMPARE(model->at(1).value(QStringLiteral("current")).toBool(), true); // newest = tip
    }
};

QTEST_GUILESS_MAIN(TestCheckpointRepository)
#include "tst_checkpoint_repository.moc"
