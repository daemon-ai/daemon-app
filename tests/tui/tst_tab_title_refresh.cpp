// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "domain/ids.h"
#include "persistence/in_memory_session_store.h"
#include "root_widget_detail.h"
#include "tab_model.h"

#include <QtTest>

// The stale-tab-title fix (GUI parity with SessionStore.changed() ->
// _resolveTitle() in TranscriptPage.qml): the node auto-titles a session from
// its first user message and the roster refetch lands that title in the store
// WITHOUT a content change, so a tab opened before the title landed kept its
// "New session" fallback forever. RootWidget connects the session store's
// changed() to rwdetail::refreshTranscriptTabTitles; this drives the same
// wiring against the in-memory store and the shared TabModel.
class TestTabTitleRefresh : public QObject {
    Q_OBJECT

private slots:
    // A store-side rename (the node auto-title analog: no content change) must
    // re-title the open transcript tab; page tabs keep their labels.
    void storeChangeRetitlesOpenTranscriptTabs() {
        persistence::InMemorySessionStore store;
        TabModel tabs;
        QObject::connect(&store, &persistence::ISessionStore::changed, &tabs,
                         [&] { rwdetail::refreshTranscriptTabTitles(&tabs, &store); });

        const QString id = store.newSessionId(domain::UnitId());
        tabs.openTranscriptPinned(id, rwdetail::resolveSessionTabTitle(&store, id));
        tabs.openPage(TabModel::Settings, QStringLiteral("Settings"));
        QCOMPARE(tabs.titleAt(0), QStringLiteral("New session"));

        store.renameSession(domain::SessionId(id), QStringLiteral("Plan the roadmap"));

        QCOMPARE(tabs.titleAt(0), QStringLiteral("Plan the roadmap"));
        QCOMPARE(tabs.titleAt(1), QStringLiteral("Settings"));
    }

    // With no store title, the refresh falls back to the content-derived title
    // (first non-empty line, heading markers stripped) - same rule as open time.
    void emptyStoreTitleFallsBackToContent() {
        persistence::InMemorySessionStore store;
        TabModel tabs;
        QObject::connect(&store, &persistence::ISessionStore::changed, &tabs,
                         [&] { rwdetail::refreshTranscriptTabTitles(&tabs, &store); });

        const QString id = store.newSessionId(domain::UnitId());
        tabs.previewTranscript(id, rwdetail::resolveSessionTabTitle(&store, id));

        store.setContent(domain::SessionId(id), QStringLiteral("# Hello world\n\nBody."));
        store.renameSession(domain::SessionId(id), QString());

        QCOMPARE(tabs.titleAt(0), QStringLiteral("Hello world"));
    }
};

QTEST_GUILESS_MAIN(TestTabTitleRefresh)
#include "tst_tab_title_refresh.moc"
