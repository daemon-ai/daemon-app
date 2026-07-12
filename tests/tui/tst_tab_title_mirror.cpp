// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// M4 sub-gate 3 (spec 09 §13; LEDGER-a7): the tab-chip / picker reads on the MIRROR path — the
// TUI tab-title wiring (rwdetail::resolveSessionTabTitle + refreshTranscriptTabTitles, the same
// helpers RootWidget binds to storeMirror->changed()) resolves titles from the mirror-backed
// store: a node-side retitle lands as a mirror keyed upsert and re-titles the open transcript
// tab, GUI parity with Session.qml's _titleFor over SessionStoreMirror.

#include "daemon/mirror_session_store.h"
#include "mirror/mirror_service.h"
#include "root_widget_detail.h"
#include "tab_model.h"

#include <QtTest>

using daemonapp::daemon::MirrorSessionStore;

class TestTabTitleMirror : public QObject {
    Q_OBJECT

private slots:
    // The node auto-title analog on the mirror path: SessionMetaChanged → SessionGet → keyed
    // upsert → store changed() → the open transcript tab re-titles from the mirror row.
    void mirrorDeltaRetitlesOpenTranscriptTab() {
        mirror::MirrorService svc;
        svc.openInMemory();
        MirrorSessionStore store(&svc.store(), &svc.ingestor(), nullptr);
        TabModel tabs;
        QObject::connect(&store, &persistence::ISessionStore::changed, &tabs,
                         [&] { rwdetail::refreshTranscriptTabTitles(&tabs, &store); });

        mirror::Session s;
        s.session = QStringLiteral("s-a");
        svc.ingestor().deliverSessions({s}, /*isFinalPage=*/true);

        tabs.openTranscriptPinned(QStringLiteral("s-a"),
                                  rwdetail::resolveSessionTabTitle(&store, QStringLiteral("s-a")));
        tabs.openPage(TabModel::Settings, QStringLiteral("Settings"));
        QCOMPARE(tabs.titleAt(0), QStringLiteral("New session")); // untitled fallback

        s.title = QStringLiteral("Plan the roadmap");
        svc.ingestor().deliverSession(s); // the SessionGet hydration upsert

        QCOMPARE(tabs.titleAt(0), QStringLiteral("Plan the roadmap"));
        QCOMPARE(tabs.titleAt(1), QStringLiteral("Settings")); // page tabs untouched
    }
};

QTEST_GUILESS_MAIN(TestTabTitleMirror)
#include "tst_tab_title_mirror.moc"
