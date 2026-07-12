// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// M4 sub-gate 2 (spec 09 §13): the sidebar session section on the MIRROR path — the
// SHARED SidebarModel (GUI Sidebar.qml + TUI sidebar view bind this one VM) reading through
// MirrorSessionStore: the All Sessions / Archived counts project mirror rows, the Fleet
// membership section lists each agent's ByProfile sessions from the mirror (bound_profile), and
// the section live-updates on mirror deltas.

#include "daemon/mirror_session_store.h"
#include "mirror/mirror_service.h"
#include "profiles/mock_profile_store.h"
#include "sidebar_model.h"

#include <QtTest/QtTest>

using daemonapp::daemon::MirrorSessionStore;

namespace {

mirror::Session sess(const QString& id, const QString& title, const QString& profile = QString()) {
    mirror::Session s;
    s.session = id;
    s.title = title;
    s.bound_profile = profile;
    return s;
}

// Row index of the first row with `type` (and, when non-empty, ProfileRole == profile); -1 if none.
int rowOf(const SidebarModel& m, domain::NodeType type, const QString& profile = QString()) {
    for (int i = 0; i < m.rowCount(); ++i) {
        const QModelIndex idx = m.index(i);
        if (m.data(idx, SidebarModel::NodeTypeRole).toInt() != static_cast<int>(type)) {
            continue;
        }
        if (!profile.isEmpty() && m.data(idx, SidebarModel::ProfileRole).toString() != profile) {
            continue;
        }
        return i;
    }
    return -1;
}

} // namespace

class TestSidebarMirror : public QObject {
    Q_OBJECT

private slots:
    void countsAndFleetSessionsProjectMirrorRows() {
        mirror::MirrorService svc;
        svc.openInMemory();
        MirrorSessionStore store(&svc.store(), &svc.ingestor(), nullptr);
        profiles::MockProfileStore profileStore; // seeds prof-1/2/3 agents

        SidebarModel model;
        model.setProfiles(&profileStore);
        model.setStore(&store);

        // Two live sessions (one bound to prof-1), one archived (additive scoped landing).
        svc.ingestor().deliverSessions(
            {sess(QStringLiteral("s-a"), QStringLiteral("Alpha"), QStringLiteral("prof-1")),
             sess(QStringLiteral("s-b"), QStringLiteral("Beta"))},
            /*isFinalPage=*/true);
        mirror::Session arch = sess(QStringLiteral("s-arch"), QStringLiteral("Old"));
        arch.archived = true;
        svc.ingestor().deliverSessionsAdditive({arch}, /*isFinalPage=*/true);

        const int allRow = rowOf(model, domain::NodeType::AllSessions);
        QVERIFY(allRow >= 0);
        QCOMPARE(model.data(model.index(allRow), SidebarModel::CountRole).toInt(), 2);
        const int archRow = rowOf(model, domain::NodeType::Archived);
        QVERIFY(archRow >= 0);
        QCOMPARE(model.data(model.index(archRow), SidebarModel::CountRole).toInt(), 1);

        // Fleet membership: the prof-1 agent row counts its mirror ByProfile sessions and its
        // expanded leaf renders the mirror title.
        const int agentRow = rowOf(model, domain::NodeType::Agent, QStringLiteral("prof-1"));
        QVERIFY(agentRow >= 0);
        QCOMPARE(model.data(model.index(agentRow), SidebarModel::CountRole).toInt(), 1);
        const int leafRow = rowOf(model, domain::NodeType::AgentSession, QStringLiteral("prof-1"));
        QVERIFY(leafRow >= 0);
        QCOMPARE(model.data(model.index(leafRow), SidebarModel::LabelRole).toString(),
                 QStringLiteral("Alpha"));
        QCOMPARE(model.data(model.index(leafRow), SidebarModel::SessionIdRole).toString(),
                 QStringLiteral("s-a"));
    }

    void sidebarLiveUpdatesOnMirrorDeltas() {
        mirror::MirrorService svc;
        svc.openInMemory();
        MirrorSessionStore store(&svc.store(), &svc.ingestor(), nullptr);

        SidebarModel model;
        model.setStore(&store);
        const int allRow = rowOf(model, domain::NodeType::AllSessions);
        QVERIFY(allRow >= 0);
        QCOMPARE(model.data(model.index(allRow), SidebarModel::CountRole).toInt(), 0);

        svc.ingestor().deliverSessions({sess(QStringLiteral("s-a"), QStringLiteral("A"))},
                                       /*isFinalPage=*/true);
        QCOMPARE(model.data(model.index(allRow), SidebarModel::CountRole).toInt(), 1);

        // A node-side archive flips the row out of All Sessions into Archived (keyed upsert).
        mirror::Session archived = sess(QStringLiteral("s-a"), QStringLiteral("A"));
        archived.archived = true;
        svc.ingestor().deliverSession(archived);
        QCOMPARE(model.data(model.index(allRow), SidebarModel::CountRole).toInt(), 0);
        const int archRow = rowOf(model, domain::NodeType::Archived);
        QCOMPARE(model.data(model.index(archRow), SidebarModel::CountRole).toInt(), 1);
    }
};

QTEST_GUILESS_MAIN(TestSidebarMirror)
#include "tst_sidebar_mirror.moc"
