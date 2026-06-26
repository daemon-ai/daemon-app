// Wiring test for EditorController's in-transcript find engine. The match
// geometry itself is covered by tst_transcript_search (the shared
// be::TranscriptSearchController); this only proves the controller exposes a
// non-null `search`, keeps it bound to its own DocumentStore, and re-collects
// matches on both load paths: loadMarkdown() (the reload/session-switch
// path, which does not emit documentChanged) and documentChanged-driven
// mutations (message append / typed-block ingest / rewind).

#include "app/editor_controller.h"
#include "core/transcript_search.h"

#include <QtTest>

using be::app::EditorController;

class EditorSearchTests : public QObject {
    Q_OBJECT

private slots:
    // The `search` engine is exposed (non-null) and re-collects against the
    // controller's own store after loadMarkdown(), with the query persisting
    // across loads (the reload path calls refresh() directly).
    void loadMarkdownRefreshesMatches() {
        EditorController ed;
        be::TranscriptSearchController* search = ed.search();
        QVERIFY(search != nullptr);

        ed.loadMarkdown(QStringLiteral("# Zephyr heading\n\nZephyr again here."), false);
        search->setQuery(QStringLiteral("Zephyr"));
        QCOMPARE(search->matchCount(), 2); // one per block

        // A reload with no occurrences clears the matches (refresh re-runs the
        // persisted query against the freshly loaded document).
        ed.loadMarkdown(QStringLiteral("Nothing to find here."), false);
        QCOMPARE(search->matchCount(), 0);

        // Re-loading content that contains the query re-finds it.
        ed.loadMarkdown(QStringLiteral("Zephyr Zephyr Zephyr"), false);
        QCOMPARE(search->matchCount(), 3);
    }

    // A documentChanged-driven mutation (appendUserMessage) re-collects matches
    // for the active query without an explicit refresh call.
    void documentChangeRefreshesMatches() {
        EditorController ed;
        be::TranscriptSearchController* search = ed.search();
        QVERIFY(search != nullptr);

        ed.loadMarkdown(QString(), false);
        search->setQuery(QStringLiteral("hello"));
        QCOMPARE(search->matchCount(), 0);

        ed.appendUserMessage(QStringLiteral("hello hello world"));
        QCOMPARE(search->matchCount(), 2);
    }
};

QTEST_MAIN(EditorSearchTests)
#include "tst_editor_search.moc"
