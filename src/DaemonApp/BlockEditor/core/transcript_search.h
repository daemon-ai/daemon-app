#pragma once

#include "core/block_record.h"

#include <QObject>
#include <QString>
#include <QVector>

namespace be {

class DocumentStore;

// Shared, toolkit-agnostic "search in transcript" engine over a be::DocumentStore.
// It collects an ordered list of matches for a query across every block's visible
// ("searchable") text and exposes cursor navigation (next/prev/current) plus a
// navigateTo(blockIndex, charOffset) signal that each front end turns into an
// anchor-style scroll (the GUI's ListView.positionViewAtIndex / the TUI's
// TranscriptView scroll-to-row). It owns no rendering: highlighting the matched
// range inside a block is the front end's job (the hard part deferred past this
// feasibility spike). Match offsets are UTF-16 (QString) indices within the
// block's searchable text, in document order.
//
// This is the de-risking spike for the search feature: it proves the
// match -> block -> offset mapping is clean and unit-testable on top of the same
// document model both front ends already render, without committing the in-block
// highlight UI.
class TranscriptSearchController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString query READ query WRITE setQuery NOTIFY queryChanged)
    Q_PROPERTY(
        bool caseSensitive READ caseSensitive WRITE setCaseSensitive NOTIFY caseSensitiveChanged)
    Q_PROPERTY(int matchCount READ matchCount NOTIFY matchesChanged)
    Q_PROPERTY(int currentMatch READ currentMatch NOTIFY currentMatchChanged)

public:
    // One match: the model row of the containing block plus the [charStart,
    // charStart+charLen) range within that block's searchable text (UTF-16).
    struct Match {
        int blockIndex = -1;
        int charStart = 0;
        int charLen = 0;
    };

    explicit TranscriptSearchController(QObject* parent = nullptr);

    // The document to search. Re-running a query against a changed document is an
    // explicit refresh() (the document model has no change signal here).
    void setDocument(DocumentStore* document);
    [[nodiscard]] DocumentStore* document() const { return m_document; }

    [[nodiscard]] QString query() const { return m_query; }
    void setQuery(const QString& query);

    [[nodiscard]] bool caseSensitive() const { return m_caseSensitive; }
    void setCaseSensitive(bool on);

    [[nodiscard]] int matchCount() const { return static_cast<int>(m_matches.size()); }
    // Index of the active match within [0, matchCount), or -1 when there are none.
    [[nodiscard]] int currentMatch() const { return m_current; }

    // Re-collect matches for the current query against the current document. Called
    // automatically by setQuery/setCaseSensitive/setDocument; expose it so a front
    // end can re-run after the transcript changes.
    Q_INVOKABLE void refresh();
    // Clear the query and all matches.
    Q_INVOKABLE void clear();

    // Move the active match forward / backward (wrapping). Each emits
    // currentMatchChanged and navigateTo for the new active match. No-op when empty.
    Q_INVOKABLE void next();
    Q_INVOKABLE void previous();
    // Jump directly to match `index` (clamped/ignored when out of range).
    Q_INVOKABLE void setCurrent(int index);

    // Active-match accessors (return -1/0 when there is no active match).
    [[nodiscard]] Q_INVOKABLE int currentBlockIndex() const;
    [[nodiscard]] Q_INVOKABLE int currentCharStart() const;
    [[nodiscard]] Q_INVOKABLE int currentCharLen() const;

    // Per-match accessors (for a front end that wants to paint every match).
    [[nodiscard]] Q_INVOKABLE int matchBlockIndex(int index) const;
    [[nodiscard]] Q_INVOKABLE int matchCharStart(int index) const;
    [[nodiscard]] Q_INVOKABLE int matchCharLen(int index) const;

    // The text a given block contributes to search (visible content, not the
    // persisted fenced markdown for typed blocks). Exposed so a front end can map a
    // match offset back onto the rendered text for highlighting.
    [[nodiscard]] QString searchableText(int blockIndex) const;

signals:
    void queryChanged();
    void caseSensitiveChanged();
    void matchesChanged();
    void currentMatchChanged();
    // The active match moved to (blockIndex, charOffset); the front end scrolls the
    // transcript so that block/offset is visible (anchor-style) and emphasises it.
    void navigateTo(int blockIndex, int charOffset);

private:
    static QString searchableTextFor(const BlockRecord& block);
    void emitNavigateToCurrent();

    DocumentStore* m_document = nullptr;
    QString m_query;
    bool m_caseSensitive = false;
    QVector<Match> m_matches;
    int m_current = -1;
};

} // namespace be
