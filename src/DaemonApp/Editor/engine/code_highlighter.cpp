// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "engine/code_highlighter.h"

#include "engine/text_document.h"

#ifndef Q_OS_WASM
#include <KSyntaxHighlighting/FoldingRegion>
#include <KSyntaxHighlighting/Format>
#include <KSyntaxHighlighting/Repository>
#endif

namespace editor {
namespace {

#ifndef Q_OS_WASM
// One shared repository for the whole app (definitions/themes are loaded from
// the baked-in Qt resources via QRC_SYNTAX).
KSyntaxHighlighting::Repository& sharedRepository() {
    static KSyntaxHighlighting::Repository repo;
    return repo;
}
#endif

} // namespace

CodeHighlighter::CodeHighlighter(TextDocument* doc, QObject* parent) : QObject(parent), m_doc(doc) {
#ifndef Q_OS_WASM
    m_theme = sharedRepository().defaultTheme(KSyntaxHighlighting::Repository::LightTheme);
    setTheme(m_theme);
#endif
    if (m_doc) {
        connect(m_doc, &TextDocument::lineChanged, this, &CodeHighlighter::onLineChanged);
        connect(m_doc, &TextDocument::linesReset, this, &CodeHighlighter::onLinesReset);
    }
    onLinesReset();
}

QString CodeHighlighter::definitionName() const {
#ifdef Q_OS_WASM
    return {};
#else
    return definition().name();
#endif
}

void CodeHighlighter::setLanguageForFile(const QString& fileName) {
#ifdef Q_OS_WASM
    Q_UNUSED(fileName)
    invalidateFrom(0);
#else
    const auto def = sharedRepository().definitionForFileName(fileName);
    setDefinition(def.isValid() ? def
                                : sharedRepository().definitionForName(QStringLiteral("None")));
    invalidateFrom(0);
#endif
}

void CodeHighlighter::setLanguageName(const QString& name) {
#ifdef Q_OS_WASM
    Q_UNUSED(name)
    invalidateFrom(0);
#else
    const auto def = sharedRepository().definitionForName(name);
    if (def.isValid())
        setDefinition(def);
    invalidateFrom(0);
#endif
}

void CodeHighlighter::setDarkTheme(bool dark) {
    if (m_dark == dark)
        return;
    m_dark = dark;
#ifndef Q_OS_WASM
    m_theme = sharedRepository().defaultTheme(dark ? KSyntaxHighlighting::Repository::DarkTheme
                                                   : KSyntaxHighlighting::Repository::LightTheme);
    setTheme(m_theme);
#endif
    invalidateFrom(0);
}

void CodeHighlighter::ensureCacheSize() {
    const int n = m_doc ? m_doc->lineCount() : 0;
#ifdef Q_OS_WASM
    if (m_runs.size() == n)
        return;
    m_runs.resize(n);
    m_foldStart.resize(n);
#else
    if (m_endState.size() == n)
        return;
    if (m_endState.size() > n) {
        m_endState.resize(n);
        m_runs.resize(n);
        m_foldStart.resize(n);
    } else {
        while (m_endState.size() < n) {
            m_endState.push_back(KSyntaxHighlighting::State());
            m_runs.push_back(QList<FormatRun>());
            m_foldStart.push_back(false);
        }
    }
#endif
}

void CodeHighlighter::invalidateFrom(int line) {
    if (line < m_firstInvalid)
        m_firstInvalid = qMax(0, line);
}

void CodeHighlighter::onLineChanged(int line) {
    ensureCacheSize();
    invalidateFrom(line);
}

void CodeHighlighter::onLinesReset() {
    ensureCacheSize();
    m_firstInvalid = 0;
}

void CodeHighlighter::highlightTo(int target) {
#ifdef Q_OS_WASM
    Q_UNUSED(target)
    ensureCacheSize();
    if (m_doc != nullptr) {
        m_firstInvalid = m_doc->lineCount();
    }
#else
    // KateBuffer::doHighlight: highlight the contiguous gap from the watermark up
    // to `target`, carrying the previous line's lexer state forward, and advance
    // the watermark. Bounded to `target` only - never a whole-document sweep.
    if (!m_doc)
        return;
    ensureCacheSize();
    const int n = m_doc->lineCount();
    if (n == 0)
        return;
    target = qBound(0, target, n - 1);
    if (m_firstInvalid > target)
        return; // already valid up to here (KateBuffer's `line < m_lineHighlighted`)

    int l = m_firstInvalid;
    KSyntaxHighlighting::State state = (l > 0) ? m_endState[l - 1] : KSyntaxHighlighting::State();
    const int changedFirst = l;
    while (l <= target) {
        m_curRuns.clear();
        m_curFold = false;
        state = highlightLine(m_doc->line(l), state);
        m_runs[l] = m_curRuns;
        m_foldStart[l] = m_curFold;
        m_endState[l] = state;
        ++l;
    }
    m_firstInvalid = l; // watermark: lines [0, m_firstInvalid) are valid
    emit highlightingChanged(changedFirst, l - 1);
#endif
}

void CodeHighlighter::ensureHighlighted(int lastLine) {
    // KateBuffer::ensureHighlighted(line, lookAhead=64): bring the watermark up to
    // the requested line plus a small look-ahead, lazily and on demand.
    highlightTo(lastLine + kLookahead);
}

const QList<FormatRun>& CodeHighlighter::runs(int line) {
    static const QList<FormatRun> empty;
    if (line < 0)
        return empty;
    highlightTo(line);
    if (line >= m_runs.size())
        return empty;
    return m_runs[line];
}

bool CodeHighlighter::foldingStartsAt(int line) const {
    return line >= 0 && line < m_foldStart.size() && m_foldStart[line];
}

#ifndef Q_OS_WASM
void CodeHighlighter::applyFormat(int offset, int length,
                                  const KSyntaxHighlighting::Format& format) {
    if (length <= 0)
        return;
    FormatRun run;
    run.start = offset;
    run.length = length;
    run.color = format.textColor(m_theme);
    run.bold = format.isBold(m_theme);
    run.italic = format.isItalic(m_theme);
    m_curRuns.push_back(run);
}

void CodeHighlighter::applyFolding(int offset, int length,
                                   KSyntaxHighlighting::FoldingRegion region) {
    Q_UNUSED(offset);
    Q_UNUSED(length);
    if (region.type() == KSyntaxHighlighting::FoldingRegion::Begin)
        m_curFold = true;
}
#endif

} // namespace editor
