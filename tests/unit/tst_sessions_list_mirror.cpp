// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// M4 sub-gate 1 (spec 09 §13; LEDGER-a7): the roster list consumer on the MIRROR path — the
// SHARED SessionsListModel (GUI SessionsList.qml + ArchivedSection.qml; TUI SessionListView bind
// this one VM) reading through MirrorSessionStore:
//  - mirror deliveries render as rows (title fallback, pinned floats first) and live-update on
//    Store::committed (no model reset storms — the store re-derives, the VM reloads);
//  - the Archived scope drives the store's refreshArchivedSessions → the ingestor's scoped
//    SessionsQuery job (mirror side) and renders the additively-delivered archived rows;
//  - the Agent scope filters bound_profile and enqueues the ByProfile scoped job;
//  - documented M4 degradations hold: snippet/content empty, tag roles empty.

#include "daemon/mirror_session_store.h"
#include "mirror/mirror_service.h"
#include "sessions_list_model.h"

#include <QSignalSpy>
#include <QtTest/QtTest>

using daemonapp::daemon::MirrorSessionStore;

namespace {

class RecordingExecutor : public mirror::FetchExecutor {
public:
    void execute(const mirror::FetchJob& job) override { executed.append(job); }
    [[nodiscard]] bool has(mirror::FetchOp op, const QString& scope) const {
        for (const mirror::FetchJob& j : executed) {
            if (j.op == op && j.scope == scope) {
                return true;
            }
        }
        return false;
    }
    QList<mirror::FetchJob> executed;
};

mirror::Session sess(const QString& id, const QString& title, quint64 activityMs = 0) {
    mirror::Session s;
    s.session = id;
    s.title = title;
    s.last_activity_ms = activityMs;
    return s;
}

} // namespace

class TestSessionsListMirror : public QObject {
    Q_OBJECT

private slots:
    void rendersMirrorRowsAndLiveUpdates() {
        mirror::MirrorService svc;
        svc.openInMemory();
        MirrorSessionStore store(&svc.store(), &svc.ingestor(), nullptr);

        SessionsListModel model;
        model.setStore(&store);
        QCOMPARE(model.rowCount(), 0);

        mirror::Session pinned = sess(QStringLiteral("s-pin"), QStringLiteral("Pinned"), 10);
        pinned.pinned = true;
        svc.ingestor().deliverSessions({sess(QStringLiteral("s-new"), QStringLiteral("Newer"), 30),
                                        sess(QStringLiteral("s-old"), QString(), 20), pinned},
                                       /*isFinalPage=*/true);

        QCOMPARE(model.rowCount(), 3);
        // Pinned floats first despite the older activity; then recency.
        QCOMPARE(model.idAt(0), QStringLiteral("s-pin"));
        QCOMPARE(model.idAt(1), QStringLiteral("s-new"));
        const QModelIndex top = model.index(0);
        QCOMPARE(model.data(top, SessionsListModel::TitleRole).toString(),
                 QStringLiteral("Pinned"));
        QVERIFY(model.data(top, SessionsListModel::PinnedRole).toBool());
        // Documented M4 degradation: no snippet (transcript re-homes in sub-gate 6). (The tag
        // roles died with the permanently-empty Tags surface — AD 1a.4.)
        QVERIFY(model.data(top, SessionsListModel::SnippetRole).toString().isEmpty());
        // The untitled row renders the "(untitled)" fallback, same as the legacy path.
        QCOMPARE(model.idAt(2), QStringLiteral("s-old"));
        QVERIFY(!model.data(model.index(2), SessionsListModel::TitleRole).toString().isEmpty());

        // Live update: a node-side retitle lands as a mirror delta and re-renders the row.
        svc.ingestor().deliverSession(sess(QStringLiteral("s-new"), QStringLiteral("Renamed"), 30));
        QCOMPARE(model.data(model.index(1), SessionsListModel::TitleRole).toString(),
                 QStringLiteral("Renamed"));
    }

    void archivedScopeFetchesAndRendersArchivedRows() {
        mirror::MirrorService svc;
        svc.openInMemory();
        RecordingExecutor exec;
        svc.setFetchExecutor(&exec);
        MirrorSessionStore store(&svc.store(), &svc.ingestor(), nullptr);

        svc.ingestor().deliverSessions({sess(QStringLiteral("s-a"), QStringLiteral("A"), 1)},
                                       /*isFinalPage=*/true);

        SessionsListModel model;
        model.setStore(&store);
        // NodeType::Archived == 1 — entering the scope triggers the store's scoped refresh, which
        // lands on the ingestor as the "archived" SessionsQuery job (the mirror-side F6 read).
        model.setScope(1, -1, QString());
        QVERIFY(exec.has(mirror::FetchOp::SessionsQuery, QStringLiteral("archived")));
        QCOMPARE(model.rowCount(), 0); // nothing archived yet

        mirror::Session arch = sess(QStringLiteral("s-arch"), QStringLiteral("Old"), 5);
        arch.archived = true;
        svc.ingestor().deliverSessionsAdditive({arch}, /*isFinalPage=*/true);
        QCOMPARE(model.rowCount(), 1);
        QCOMPARE(model.idAt(0), QStringLiteral("s-arch"));

        // Back to AllSessions (0): archived rows leave the list; s-a is still there.
        model.setScope(0, -1, QString());
        QCOMPARE(model.rowCount(), 1);
        QCOMPARE(model.idAt(0), QStringLiteral("s-a"));

        svc.setFetchExecutor(nullptr);
    }

    void agentScopeFiltersBoundProfileAndFetches() {
        mirror::MirrorService svc;
        svc.openInMemory();
        RecordingExecutor exec;
        svc.setFetchExecutor(&exec);
        MirrorSessionStore store(&svc.store(), &svc.ingestor(), nullptr);

        mirror::Session bound = sess(QStringLiteral("s-b"), QStringLiteral("B"), 2);
        bound.bound_profile = QStringLiteral("prof-1");
        svc.ingestor().deliverSessions({sess(QStringLiteral("s-a"), QStringLiteral("A"), 1), bound},
                                       /*isFinalPage=*/true);

        SessionsListModel model;
        model.setStore(&store);
        // NodeType::Agent == 11; the string slot carries the profile id (lensKey).
        model.setScope(11, -1, QStringLiteral("prof-1"));
        QVERIFY(exec.has(mirror::FetchOp::SessionsQuery,
                         QStringLiteral("profile") + QChar(0x1f) + QStringLiteral("prof-1")));
        QCOMPARE(model.rowCount(), 1);
        QCOMPARE(model.idAt(0), QStringLiteral("s-b"));

        svc.setFetchExecutor(nullptr);
    }
};

QTEST_GUILESS_MAIN(TestSessionsListMirror)
#include "tst_sessions_list_mirror.moc"
