// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <KSyntaxHighlighting/AbstractHighlighter>
#include <KSyntaxHighlighting/Definition>
#include <KSyntaxHighlighting/State>
#include <KSyntaxHighlighting/Theme>
#include <QColor>
#include <QList>
#include <QObject>
#include <QString>

namespace editor {

class TextDocument;

// One styled span within a line.
struct FormatRun {
    int start = 0;
    int length = 0;
    QColor color;
    bool bold = false;
    bool italic = false;
};

// Incremental syntax highlighter over a TextDocument, built on the C++
// KSyntaxHighlighting engine (the same library daemon-app vendors). It mirrors
// the KTextEditor pattern: each line caches its end State and its format runs,
// a `firstInvalid` watermark tracks where re-highlighting must resume, and
// edits invalidate from the first changed line forward. Highlighting is served
// for the requested viewport and continues past it only until the lexer state
// stabilizes. The QML line model consumes runs(line) to colour text.
class CodeHighlighter : public QObject, public KSyntaxHighlighting::AbstractHighlighter {
    Q_OBJECT

public:
    explicit CodeHighlighter(TextDocument* doc, QObject* parent = nullptr);

    void setLanguageForFile(const QString& fileName); // pick definition by name/extension
    void setLanguageName(const QString& name);
    void setDarkTheme(bool dark);
    [[nodiscard]] QString definitionName() const;

    // Ensure lines up to `lastLine` (+ a small lookahead) are highlighted now,
    // bounded so the UI thread never highlights the whole document synchronously;
    // the remainder is finished progressively by the background timer.
    void ensureHighlighted(int lastLine);
    [[nodiscard]] const QList<FormatRun>& runs(int line);
    [[nodiscard]] bool foldingStartsAt(int line) const;

signals:
    // Highlighting for [firstLine, lastLine] changed; the model re-emits those rows.
    void highlightingChanged(int firstLine, int lastLine);

protected:
    void applyFormat(int offset, int length, const KSyntaxHighlighting::Format& format) override;
    void applyFolding(int offset, int length, KSyntaxHighlighting::FoldingRegion region) override;

private slots:
    void onLineChanged(int line);
    void onLinesReset();

private:
    void invalidateFrom(int line);
    void ensureCacheSize();
    // Highlight the contiguous gap from the watermark up to `target` (clamped),
    // carrying lexer state forward, advancing the watermark, and emitting
    // highlightingChanged for the band. Bounded - never a whole-document sweep.
    void highlightTo(int target);

    TextDocument* m_doc = nullptr;
    KSyntaxHighlighting::Theme m_theme;
    bool m_dark = false;

    QList<KSyntaxHighlighting::State> m_endState; // end state per line
    QList<QList<FormatRun>> m_runs;               // format runs per line
    QList<bool> m_foldStart;                      // a fold region begins on this line
    // Watermark, mirroring KateBuffer::m_lineHighlighted: lines [0, m_firstInvalid)
    // are highlighted-and-valid; edits drop it back to the changed line.
    int m_firstInvalid = 0;
    static constexpr int kLookahead = 64; // KateBuffer's default look-ahead

    // Scratch for the line currently being highlighted.
    QList<FormatRun> m_curRuns;
    bool m_curFold = false;
};

} // namespace editor
