// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// tst_transcript_search: the search-feature feasibility spike. Proves the shared
// be::TranscriptSearchController collects matches over a be::DocumentStore in
// document order with correct (blockIndex, charStart, charLen) offsets, honours
// case sensitivity, searches typed agent blocks' visible text (reasoning body,
// not the fenced envelope), and drives next/prev cursor navigation + the
// navigateTo(blockIndex, charOffset) anchor signal both front ends will consume.

#include "core/document_store.h"
#include "core/transcript_search.h"

#include <QSignalSpy>
#include <QtTest/QtTest>

using be::DocumentStore;
using be::TranscriptSearchController;

namespace {
// Three blocks: a heading (row 0) and two paragraphs (rows 1, 2). The paragraph
// text places known substrings at known UTF-16 offsets so the spike can assert
// the exact match geometry.
constexpr auto kFixture = R"(# Alpha heading

The quick brown fox. Foo bar foo.

Another fox here.)";
} // namespace

class TestTranscriptSearch : public QObject {
    Q_OBJECT

private slots:
    void emptyQueryHasNoMatches();
    void collectsMatchesInDocumentOrder();
    void multipleMatchesWithinOneBlock();
    void caseSensitivityFiltersMatches();
    void nextPrevWrapAndEmitNavigate();
    void setQueryEmitsNavigateToFirstMatch();
    void searchesTypedBlockVisibleText();
    void clearResetsState();
};

void TestTranscriptSearch::emptyQueryHasNoMatches() {
    DocumentStore doc;
    doc.loadMarkdown(QString::fromUtf8(kFixture));
    TranscriptSearchController search;
    search.setDocument(&doc);

    QCOMPARE(search.matchCount(), 0);
    QCOMPARE(search.currentMatch(), -1);
    QCOMPARE(search.currentBlockIndex(), -1);
}

void TestTranscriptSearch::collectsMatchesInDocumentOrder() {
    DocumentStore doc;
    doc.loadMarkdown(QString::fromUtf8(kFixture));
    TranscriptSearchController search;
    search.setDocument(&doc);

    search.setQuery(QStringLiteral("fox"));

    // "fox" appears once in row 1 and once in row 2; the heading has none.
    QCOMPARE(search.matchCount(), 2);
    QCOMPARE(search.matchBlockIndex(0), 1);
    QCOMPARE(search.matchCharStart(0), 16);
    QCOMPARE(search.matchCharLen(0), 3);
    QCOMPARE(search.matchBlockIndex(1), 2);
    QCOMPARE(search.matchCharStart(1), 8);
    // Active match starts at the first.
    QCOMPARE(search.currentMatch(), 0);
    QCOMPARE(search.currentBlockIndex(), 1);
    QCOMPARE(search.currentCharStart(), 16);
}

void TestTranscriptSearch::multipleMatchesWithinOneBlock() {
    DocumentStore doc;
    doc.loadMarkdown(QString::fromUtf8(kFixture));
    TranscriptSearchController search;
    search.setDocument(&doc);

    // "foo" (case-insensitive) hits "Foo" then "foo" in row 1, in order.
    search.setQuery(QStringLiteral("foo"));
    QCOMPARE(search.matchCount(), 2);
    QCOMPARE(search.matchBlockIndex(0), 1);
    QCOMPARE(search.matchCharStart(0), 21);
    QCOMPARE(search.matchBlockIndex(1), 1);
    QCOMPARE(search.matchCharStart(1), 29);
}

void TestTranscriptSearch::caseSensitivityFiltersMatches() {
    DocumentStore doc;
    doc.loadMarkdown(QString::fromUtf8(kFixture));
    TranscriptSearchController search;
    search.setDocument(&doc);

    search.setCaseSensitive(true);

    search.setQuery(QStringLiteral("foo"));
    QCOMPARE(search.matchCount(), 1);
    QCOMPARE(search.matchCharStart(0), 29);

    search.setQuery(QStringLiteral("Foo"));
    QCOMPARE(search.matchCount(), 1);
    QCOMPARE(search.matchCharStart(0), 21);
}

void TestTranscriptSearch::nextPrevWrapAndEmitNavigate() {
    DocumentStore doc;
    doc.loadMarkdown(QString::fromUtf8(kFixture));
    TranscriptSearchController search;
    search.setDocument(&doc);

    QSignalSpy navSpy(&search, &TranscriptSearchController::navigateTo);
    search.setQuery(QStringLiteral("fox")); // emits navigateTo(1,16) for the first match
    QCOMPARE(navSpy.count(), 1);
    QCOMPARE(navSpy.takeFirst(), (QList<QVariant>{1, 16}));

    search.next(); // -> match 1 (row 2)
    QCOMPARE(search.currentMatch(), 1);
    QCOMPARE(navSpy.takeFirst(), (QList<QVariant>{2, 8}));

    search.next(); // wraps -> match 0 (row 1)
    QCOMPARE(search.currentMatch(), 0);
    QCOMPARE(navSpy.takeFirst(), (QList<QVariant>{1, 16}));

    search.previous(); // wraps back -> match 1 (row 2)
    QCOMPARE(search.currentMatch(), 1);
    QCOMPARE(navSpy.takeFirst(), (QList<QVariant>{2, 8}));
}

void TestTranscriptSearch::setQueryEmitsNavigateToFirstMatch() {
    DocumentStore doc;
    doc.loadMarkdown(QString::fromUtf8(kFixture));
    TranscriptSearchController search;
    search.setDocument(&doc);

    QSignalSpy matchesSpy(&search, &TranscriptSearchController::matchesChanged);
    search.setQuery(QStringLiteral("brown"));
    QVERIFY(matchesSpy.count() >= 1);
    QCOMPARE(search.matchCount(), 1);
    QCOMPARE(search.currentBlockIndex(), 1);
    QCOMPARE(search.currentCharStart(), 10);
}

void TestTranscriptSearch::searchesTypedBlockVisibleText() {
    DocumentStore doc;
    doc.loadMarkdown(QString::fromUtf8(kFixture));
    // Append a reasoning block: its visible text is metadata["body"], not the
    // fenced markdown envelope it serializes to.
    QVariantMap meta;
    meta.insert(QStringLiteral("status"), QStringLiteral("complete"));
    meta.insert(QStringLiteral("body"), QStringLiteral("Investigating the foobar cache."));
    doc.appendTypedBlock(be::BlockType::Reasoning, meta);
    const int reasoningRow = static_cast<int>(doc.blockCount()) - 1;

    TranscriptSearchController search;
    search.setDocument(&doc);
    search.setQuery(QStringLiteral("foobar"));

    QCOMPARE(search.matchCount(), 1);
    QCOMPARE(search.currentBlockIndex(), reasoningRow);
    QCOMPARE(search.currentCharStart(), 18);
    QCOMPARE(search.searchableText(reasoningRow),
             QStringLiteral("Investigating the foobar cache."));
}

void TestTranscriptSearch::clearResetsState() {
    DocumentStore doc;
    doc.loadMarkdown(QString::fromUtf8(kFixture));
    TranscriptSearchController search;
    search.setDocument(&doc);
    search.setQuery(QStringLiteral("fox"));
    QCOMPARE(search.matchCount(), 2);

    search.clear();
    QCOMPARE(search.query(), QString());
    QCOMPARE(search.matchCount(), 0);
    QCOMPARE(search.currentMatch(), -1);
}

QTEST_MAIN(TestTranscriptSearch)
#include "tst_transcript_search.moc"
