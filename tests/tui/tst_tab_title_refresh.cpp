// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemon/mirror_session_store.h"
#include "domain/ids.h"
#include "mirror/mirror_service.h"
#include "mirror/seeder.h"
#include "root_widget_detail.h"
#include "tab_model.h"

#include <QtTest>

// The stale-tab-title fix (GUI parity with SessionStoreMirror.changed() ->
// _resolveTitle() in TranscriptPage.qml): the node auto-titles a session from
// its first user message and the roster delta lands that title in the MIRROR
// row WITHOUT a content change, so a tab opened before the title landed kept
// its "New session" fallback forever. RootWidget connects the session store's
// changed() to rwdetail::refreshTranscriptTabTitles; this drives the same
// wiring against the mirror-backed store (AD: the InMemory fixture died with
// the legacy stores — the Seeder feeds the SAME store production renders).
class TestTabTitleRefresh : public QObject {
    Q_OBJECT

private slots:
    // A node-side rename (a roster delta: no content change) must re-title the open transcript
    // tab; page tabs keep their labels.
    void storeChangeRetitlesOpenTranscriptTabs() {
        mirror::MirrorService svc;
        svc.openInMemory();
        daemonapp::daemon::MirrorSessionStore store(&svc.store(), &svc.ingestor());
        mirror::Seeder seeder(svc.store());
        TabModel tabs;
        QObject::connect(&store, &persistence::ISessionStore::changed, &tabs,
                         [&] { rwdetail::refreshTranscriptTabTitles(&tabs, &store); });

        mirror::Session s;
        s.session = QStringLiteral("s-1");
        seeder.upsertSession(s); // untitled row → the "New session" fallback
        tabs.openTranscriptPinned(QStringLiteral("s-1"),
                                  rwdetail::resolveSessionTabTitle(&store, QStringLiteral("s-1")));
        tabs.openPage(TabModel::Settings, QStringLiteral("Settings"));
        QCOMPARE(tabs.titleAt(0), QStringLiteral("New session"));

        s.title = QStringLiteral("Plan the roadmap");
        seeder.upsertSession(s); // the node's auto-title lands as a mirror row delta

        QCOMPARE(tabs.titleAt(0), QStringLiteral("Plan the roadmap"));
        QCOMPARE(tabs.titleAt(1), QStringLiteral("Settings"));
    }

    // With no store title, the refresh falls back to the content-derived title (first prose line
    // of the mirror transcript projection; fence metadata skipped) — same rule as open time.
    void emptyStoreTitleFallsBackToContent() {
        mirror::MirrorService svc;
        svc.openInMemory();
        daemonapp::daemon::MirrorSessionStore store(&svc.store(), &svc.ingestor());
        mirror::Seeder seeder(svc.store());
        TabModel tabs;
        QObject::connect(&store, &persistence::ISessionStore::changed, &tabs,
                         [&] { rwdetail::refreshTranscriptTabTitles(&tabs, &store); });

        mirror::Session s;
        s.session = QStringLiteral("s-2");
        seeder.upsertSession(s);
        tabs.previewTranscript(QStringLiteral("s-2"),
                               rwdetail::resolveSessionTabTitle(&store, QStringLiteral("s-2")));

        // An engine transcript write lands in the mirror window: the msg-fence marker is
        // metadata (skipped); the body's heading is the derived title.
        mirror::TranscriptBlock b;
        b.session = QStringLiteral("s-2");
        b.seq = 1;
        b.kind = QStringLiteral("Message");
        b.role = QStringLiteral("Assistant");
        b.text = QStringLiteral("# Hello world\n\nBody.");
        svc.ingestor().deliverTranscriptBlock(b);

        QCOMPARE(tabs.titleAt(0), QStringLiteral("Hello world"));
    }
};

QTEST_GUILESS_MAIN(TestTabTitleRefresh)
#include "tst_tab_title_refresh.moc"
