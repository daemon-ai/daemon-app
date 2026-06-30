// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "core/block_record.h"

#include <functional>
#include <QColor>
#include <QSizeF>
#include <QString>
#include <QVector>

namespace be {

enum class SpanKind : quint16 {
    Plain,
    Emphasis,
    Strong,
    Code,
    Link,
    Image,
    Math,
    HiddenDelimiter,
};

// Inline emphasis carried alongside a span's kind, so a Link can also render
// bold/italic (e.g. **[text](url)**) without a second overlapping span.
enum SpanStyle : quint8 {
    StyleBold = 0x1,
    StyleItalic = 0x2,
};

struct InlineSpan {
    SpanKind kind = SpanKind::Plain;
    quint8 styleMask = 0;      // OR of SpanStyle bits; currently used for Link spans
    qsizetype rawStart = 0;    // for Link/Image: start of '[' / '!'
    qsizetype rawEnd = 0;      // for Link/Image: one past ')'
    qsizetype visualStart = 0; // label start in visual text
    qsizetype visualEnd = 0;   // label end in visual text
    QString url;               // populated only for SpanKind::Link
    QString imageUrl;          // SpanKind::Image: the image url; Link: a leading favicon
    qreal imageWidth = 0;      // SpanKind::Image: explicit width in px (0 == unset)
    qreal imageHeight = 0;     // SpanKind::Image: explicit height in px (0 == unset)
    QString mathLatex;         // SpanKind::Math: the LaTeX source between the $ delimiters
    bool mathDisplay = false;  // SpanKind::Math: true for $$display$$, false for $inline$
};

struct BlockProjection {
    BlockId blockId = 0;
    quint32 blockRevision = 0;
    QString visualText;
    QString displayMarkup;
    QVector<InlineSpan> spans;
    QVector<qsizetype> visualToRawUtf16;
    QVector<qsizetype> rawToVisualUtf16;

    qsizetype visualToRaw(qsizetype visualOffset) const;
    qsizetype rawToVisual(qsizetype rawOffset) const;
};

// Colors/fonts baked into the RichText HTML the projector emits. Defaults
// reproduce the upstream hardcoded look; the host (EditorController) overrides
// them from the active theme so a theme switch recolors the rendered markup.
struct Palette {
    QString monoFamily = QStringLiteral("monospace");
    QColor codeBackground = QColor(QStringLiteral("#f1f1ef"));
    QColor codeText; // invalid == inherit the surrounding text color
    QColor link = QColor(QStringLiteral("#63b3ed"));
    QColor text; // invalid == inherit (used for headings)
    // Base body font size in px; headings are scaled relative to it so the
    // user's "Font size" preference moves the whole type scale together.
    int bodyPixelSize = 15;
};

class InlineProjector {
public:
    // contentWidth is the px width of the flow column; it gives a percent inline
    // image (e.g. ![..]{ width=10% }) a concrete basis. 0 means "unknown", in
    // which case a percent width is dropped rather than forcing a guess.
    BlockProjection project(const BlockRecord& block, bool revealMarkdown = false,
                            qsizetype rawCursor = -1, qreal contentWidth = 0) const;

    void setPalette(const Palette& palette) { m_palette = palette; }
    [[nodiscard]] const Palette& palette() const { return m_palette; }

    // Optional hook returning the logical (device-independent) size of a math
    // formula so inline math can emit an explicit width/height. Kept as a
    // callback so core stays free of any MicroTeX dependency; the host wires it
    // to be::app::measureMathLogicalSize. Args: (latex, display, fontPx).
    using MathMeasurer = std::function<QSizeF(const QString&, bool, int)>;
    void setMathMeasurer(MathMeasurer measurer) { m_mathMeasurer = std::move(measurer); }

private:
    // Block-level prefix (heading hashes / list / task marker) hidden when not
    // revealing markdown; returns the offset where inline content begins.
    qsizetype applyBlockPrefix(const BlockRecord& block, BlockProjection& projection,
                               const QString& raw) const;
    // Scan [contentStart, contentEnd) of `raw`, emitting inline spans. When
    // revealAll is set every char is plain text (raw markdown shown verbatim).
    void projectInlineSpans(const QString& raw, qsizetype contentStart, qsizetype contentEnd,
                            bool revealAll, qreal contentWidth, BlockProjection& projection) const;
    // Try each inline-construct matcher at `i`; returns the index just past the
    // consumed span, or -1 when nothing matched (the char is emitted as plain).
    qsizetype matchInline(const QString& raw, qsizetype i, qsizetype contentEnd, qreal contentWidth,
                          BlockProjection& projection) const;
    // Inline matchers: each returns the consumed-through index or -1 (no match).
    qsizetype matchInlineImage(const QString& raw, qsizetype i, qsizetype contentEnd,
                               qreal contentWidth, BlockProjection& projection) const;
    qsizetype matchInlineLink(const QString& raw, qsizetype i, qsizetype contentEnd,
                              BlockProjection& projection) const;
    qsizetype matchAutolink(const QString& raw, qsizetype i, qsizetype contentEnd,
                            BlockProjection& projection) const;
    qsizetype matchInlineMath(const QString& raw, qsizetype i, qsizetype contentEnd,
                              BlockProjection& projection) const;
    qsizetype matchEmphasisRun(const QString& raw, qsizetype i, qsizetype contentEnd,
                               const QString& delimiter, SpanKind kind, quint8 linkStyle,
                               BlockProjection& projection) const;
    qsizetype matchInlineCode(const QString& raw, qsizetype i, qsizetype contentEnd,
                              BlockProjection& projection) const;
    // Back-fill rawToVisual gaps and interpolate the visualToRaw map.
    void finalizeOffsetMaps(BlockProjection& projection, const QString& raw) const;

    QString makeDisplayMarkup(const BlockRecord& block, const BlockProjection& projection) const;
    // Render one span to display HTML (delegates the non-trivial kinds below).
    QString renderSpan(const InlineSpan& span, const QString& text) const;
    QString renderCodeSpan(const QString& text) const;
    QString renderLinkSpan(const InlineSpan& span, QString text) const;
    QString renderImageSpan(const InlineSpan& span) const;
    QString renderMathSpan(const InlineSpan& span) const;
    // Wrap the assembled inline HTML in any block-level styling (heading/fence).
    QString wrapBlockMarkup(const BlockRecord& block, const QString& escaped) const;

    QString stripInlineMarkup(const QString& raw, qsizetype start, qsizetype end) const;
    // stripInlineMarkup sub-matchers: each appends to `out`, returns next `k` or -1.
    qsizetype stripImage(const QString& raw, qsizetype k, qsizetype end, QString& out) const;
    qsizetype stripLink(const QString& raw, qsizetype k, qsizetype end, QString& out) const;
    qsizetype stripEmphasisRun(const QString& raw, qsizetype k, qsizetype end,
                               const QString& delimiter, QString& out) const;
    qsizetype stripCodeRun(const QString& raw, qsizetype k, qsizetype end, QString& out) const;

    void appendVisual(BlockProjection& projection, const QString& raw, qsizetype rawIndex,
                      qsizetype length, SpanKind kind) const;
    // Push a single-placeholder span (image/math) and map its whole raw range
    // onto that one visual position; shared by appendImage/appendMath.
    void appendPlaceholderSpan(BlockProjection& projection, InlineSpan span, qsizetype rawStart,
                               qsizetype rawEnd) const;
    void appendImage(BlockProjection& projection, qsizetype rawStart, qsizetype rawEnd,
                     const QString& url, qreal width = 0, qreal height = 0) const;
    void appendMath(BlockProjection& projection, qsizetype rawStart, qsizetype rawEnd,
                    const QString& latex, bool displayMode) const;
    void appendLink(BlockProjection& projection, qsizetype linkRawStart, qsizetype linkRawEnd,
                    const QString& label, const QString& url, const QString& imageUrl = QString(),
                    quint8 styleMask = 0) const;
    void appendPresentation(BlockProjection& projection, const QString& text,
                            qsizetype rawAffinity) const;

    Palette m_palette;
    MathMeasurer m_mathMeasurer;
};

} // namespace be
