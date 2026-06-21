// Unit tests for the TUI's TranscriptLayout - the terminal analog of the GUI's
// QML block delegates. They feed seed-style agent markdown through be::DocumentStore
// (the shared parse engine) into TranscriptLayout::build and assert the produced
// styled rows: structural glyphs/titles are present, the persisted fences/JSON are
// gone, and ANSI/diff spans carry the right semantic colors.

#include "transcript_render.h"
#include "tui_palette.h"

#include "core/document_store.h"

#include <Tui/ZColor.h>

#include <QtTest>

namespace {

// Sample assistant turn: a user message, then an assistant message with a
// reasoning block, an ANSI tool, and a diff tool. \u001b is a literal JSON escape
// (QJsonDocument decodes it to ESC) so the raw string stays portable.
QString sampleMarkdown()
{
    return QStringLiteral(R"md(```msg
{"id":"u1","role":"user"}
```

Build it please.

```msg
{"id":"m1","role":"assistant"}
```

```reasoning
{"status":"complete","durationMs":1200,"body":"Inspecting then building."}
```

```tool
{"callId":"c1","name":"terminal","tone":"terminal","status":"ok","durationMs":1200,"argsSummary":"ninja","detailKind":"ansi-stream","stdout":"\u001b[32mPASS\u001b[0m ok\n"}
```

```tool
{"callId":"c2","name":"apply_patch","tone":"edit","status":"ok","durationMs":260,"argsSummary":"main.cpp","detailKind":"diff","diff":"--- a/x\n+++ b/x\n@@ -1 +1 @@\n-old\n+new\n"}
```
)md");
}

QString flatten(const QVector<RenderLine> &lines)
{
    QStringList rows;
    for (const RenderLine &line : lines) {
        QString row;
        for (const Span &s : line) {
            row += s.text;
        }
        rows << row;
    }
    return rows.join(QLatin1Char('\n'));
}

// True when some span whose text contains `needle` also satisfies `pred`.
template <typename Pred>
bool anySpan(const QVector<RenderLine> &lines, const QString &needle, Pred pred)
{
    for (const RenderLine &line : lines) {
        for (const Span &s : line) {
            if (s.text.contains(needle) && pred(s)) {
                return true;
            }
        }
    }
    return false;
}

} // namespace

class TranscriptRenderTests : public QObject {
    Q_OBJECT

private slots:
    void rendersStructureNotFences();
    void messageHeaders();
    void toolHeaderGlyphsAndTitles();
    void ansiOutputCarriesColor();
    void diffLinesCarryColor();
};

void TranscriptRenderTests::rendersStructureNotFences()
{
    be::DocumentStore doc;
    doc.loadMarkdown(sampleMarkdown());
    const QVector<RenderLine> lines = TranscriptLayout::build(doc, 80);
    const QString text = flatten(lines);

    // The persisted canonical form is gone: no code fences, no raw JSON keys.
    QVERIFY(!text.contains(QStringLiteral("```")));
    QVERIFY(!text.contains(QStringLiteral("callId")));
    QVERIFY(!text.contains(QStringLiteral("detailKind")));
    QVERIFY(!text.contains(QStringLiteral("\"role\"")));

    // The user's prose still renders.
    QVERIFY(text.contains(QStringLiteral("Build it please.")));
}

void TranscriptRenderTests::messageHeaders()
{
    be::DocumentStore doc;
    doc.loadMarkdown(sampleMarkdown());
    const QString text = flatten(TranscriptLayout::build(doc, 80));
    QVERIFY(text.contains(QStringLiteral("YOU")));
    QVERIFY(text.contains(QStringLiteral("DAEMON")));
    // Card left rule + reasoning header marker.
    QVERIFY(text.contains(tpal::barGlyph()));
    QVERIFY(text.contains(tpal::reasoningGlyph()));
    QVERIFY(text.contains(QStringLiteral("Reasoning")));
}

void TranscriptRenderTests::toolHeaderGlyphsAndTitles()
{
    be::DocumentStore doc;
    doc.loadMarkdown(sampleMarkdown());
    const QVector<RenderLine> lines = TranscriptLayout::build(doc, 80);
    const QString text = flatten(lines);

    // Settled tool names appear as titles, with the ok status glyph and the
    // tone glyphs for terminal/edit.
    QVERIFY(text.contains(QStringLiteral("terminal")));
    QVERIFY(text.contains(QStringLiteral("apply_patch")));
    QVERIFY(text.contains(tpal::statusGlyph(QStringLiteral("ok"))));
    QVERIFY(text.contains(tpal::toneGlyph(QStringLiteral("terminal"))));
    QVERIFY(text.contains(tpal::toneGlyph(QStringLiteral("edit"))));
    // The argsSummary subtitle is carried through.
    QVERIFY(text.contains(QStringLiteral("ninja")));

    // The ok status glyph is painted in the ok (green) color.
    QVERIFY(anySpan(lines, tpal::statusGlyph(QStringLiteral("ok")),
                    [](const Span &s) { return s.fg == tpal::statusOk(); }));
}

void TranscriptRenderTests::ansiOutputCarriesColor()
{
    be::DocumentStore doc;
    doc.loadMarkdown(sampleMarkdown());
    const QVector<RenderLine> lines = TranscriptLayout::build(doc, 80);

    // The ANSI \u001b[32m run colors "PASS" with ANSI green (index 2); the reset
    // returns " ok" to the default foreground.
    QVERIFY(anySpan(lines, QStringLiteral("PASS"),
                    [](const Span &s) { return s.fg == tpal::ansi(2); }));
    QVERIFY(anySpan(lines, QStringLiteral("ok"),
                    [](const Span &s) { return s.fg == tpal::fg(); }));
}

void TranscriptRenderTests::diffLinesCarryColor()
{
    be::DocumentStore doc;
    doc.loadMarkdown(sampleMarkdown());
    const QVector<RenderLine> lines = TranscriptLayout::build(doc, 80);

    QVERIFY(anySpan(lines, QStringLiteral("+new"),
                    [](const Span &s) { return s.fg == tpal::diffAdd(); }));
    QVERIFY(anySpan(lines, QStringLiteral("-old"),
                    [](const Span &s) { return s.fg == tpal::diffDel(); }));
    QVERIFY(anySpan(lines, QStringLiteral("@@"),
                    [](const Span &s) { return s.fg == tpal::diffHunk(); }));
}

QTEST_MAIN(TranscriptRenderTests)

#include "tst_transcript_render.moc"
