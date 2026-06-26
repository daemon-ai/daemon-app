// Unit tests for the TUI's TranscriptLayout - the terminal analog of the GUI's
// QML block delegates. They feed seed-style agent markdown through be::DocumentStore
// (the shared parse engine) into TranscriptLayout::build and assert the produced
// styled rows: structural glyphs/titles are present, the persisted fences/JSON are
// gone, and ANSI/diff spans carry the right semantic colors.

#include "core/block_record.h"
#include "core/document_store.h"
#include "transcript_render.h"
#include "tui_palette.h"

#include <QtTest>
#include <Tui/ZColor.h>

namespace {

// Sample assistant turn: a user message, then an assistant message with a
// reasoning block, an ANSI tool, and a diff tool. \u001b is a literal JSON escape
// (QJsonDocument decodes it to ESC) so the raw string stays portable.
QString sampleMarkdown() {
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

// An interactive turn: a dangerous tool awaiting approval and a multi-question
// clarify (single-select, multi-select, freeform) - the shapes the inline
// controls must surface.
QString interactiveMarkdown() {
    return QStringLiteral(R"md(```msg
{"id":"m1","role":"assistant"}
```

```tool
{"callId":"c8","name":"terminal","tone":"terminal","status":"running","needsApproval":true,"allowPermanent":true,"approvalCommand":"rm -rf build"}
```

```tool
{"callId":"c7","name":"clarify","requestId":"q1","questions":[{"id":"db","prompt":"Which database?","choices":["PostgreSQL","SQLite"]},{"id":"scope","prompt":"What to migrate?","choices":["Schema","Data"],"multiSelect":true},{"id":"notes","prompt":"Extra constraints?","allowFreeform":true}]}
```
)md");
}

QString flatten(const QVector<RenderLine>& lines) {
    QStringList rows;
    for (const RenderLine& line : lines) {
        QString row;
        for (const Span& s : line) {
            row += s.text;
        }
        rows << row;
    }
    return rows.join(QLatin1Char('\n'));
}

// True when some span whose text contains `needle` also satisfies `pred`.
template <typename Pred>
bool anySpan(const QVector<RenderLine>& lines, const QString& needle, Pred pred) {
    for (const RenderLine& line : lines) {
        for (const Span& s : line) {
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
    void approvalEmitsButtonControls();
    void clarifyEmitsChoiceAndFreeformControls();
    void clarifyDraftReflectsSelection();
    void multiParagraphMessageKeepsBlankLines();
    void blockLineMapsAddressEveryRow();
};

void TranscriptRenderTests::rendersStructureNotFences() {
    be::DocumentStore doc;
    doc.loadMarkdown(sampleMarkdown());
    const QVector<RenderLine> lines = TranscriptLayout::build(doc, 80).lines;
    const QString text = flatten(lines);

    // The persisted canonical form is gone: no code fences, no raw JSON keys.
    QVERIFY(!text.contains(QStringLiteral("```")));
    QVERIFY(!text.contains(QStringLiteral("callId")));
    QVERIFY(!text.contains(QStringLiteral("detailKind")));
    QVERIFY(!text.contains(QStringLiteral("\"role\"")));

    // The user's prose still renders.
    QVERIFY(text.contains(QStringLiteral("Build it please.")));
}

void TranscriptRenderTests::messageHeaders() {
    be::DocumentStore doc;
    doc.loadMarkdown(sampleMarkdown());
    const QString text = flatten(TranscriptLayout::build(doc, 80).lines);
    QVERIFY(text.contains(QStringLiteral("YOU")));
    QVERIFY(text.contains(QStringLiteral("DAEMON")));
    // Card left rule + reasoning header marker.
    QVERIFY(text.contains(tpal::barGlyph()));
    QVERIFY(text.contains(tpal::reasoningGlyph()));
    QVERIFY(text.contains(QStringLiteral("Reasoning")));
}

void TranscriptRenderTests::toolHeaderGlyphsAndTitles() {
    be::DocumentStore doc;
    doc.loadMarkdown(sampleMarkdown());
    const QVector<RenderLine> lines = TranscriptLayout::build(doc, 80).lines;
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
                    [](const Span& s) { return s.fg == tpal::statusOk(); }));
}

void TranscriptRenderTests::ansiOutputCarriesColor() {
    be::DocumentStore doc;
    doc.loadMarkdown(sampleMarkdown());
    const QVector<RenderLine> lines = TranscriptLayout::build(doc, 80).lines;

    // The ANSI \u001b[32m run colors "PASS" with ANSI green (index 2); the reset
    // returns " ok" to the default foreground.
    QVERIFY(anySpan(lines, QStringLiteral("PASS"),
                    [](const Span& s) { return s.fg == tpal::ansi(2); }));
    QVERIFY(anySpan(lines, QStringLiteral("ok"), [](const Span& s) { return s.fg == tpal::fg(); }));
}

void TranscriptRenderTests::diffLinesCarryColor() {
    be::DocumentStore doc;
    doc.loadMarkdown(sampleMarkdown());
    const QVector<RenderLine> lines = TranscriptLayout::build(doc, 80).lines;

    QVERIFY(anySpan(lines, QStringLiteral("+new"),
                    [](const Span& s) { return s.fg == tpal::diffAdd(); }));
    QVERIFY(anySpan(lines, QStringLiteral("-old"),
                    [](const Span& s) { return s.fg == tpal::diffDel(); }));
    QVERIFY(anySpan(lines, QStringLiteral("@@"),
                    [](const Span& s) { return s.fg == tpal::diffHunk(); }));
}

void TranscriptRenderTests::approvalEmitsButtonControls() {
    be::DocumentStore doc;
    doc.loadMarkdown(interactiveMarkdown());
    const LayoutResult res = TranscriptLayout::build(doc, 80);
    const QString text = flatten(res.lines);

    // The read-only command line plus three inline buttons.
    QVERIFY(text.contains(QStringLiteral("Approve:")));
    QVERIFY(text.contains(QStringLiteral("Approve")));
    QVERIFY(text.contains(QStringLiteral("Deny")));
    QVERIFY(text.contains(QStringLiteral("Allow permanently")));

    int approve = 0, deny = 0, permanent = 0;
    for (const Control& c : res.controls) {
        if (c.kind == Control::Kind::Approve) {
            ++approve;
            QCOMPARE(c.callId, QStringLiteral("c8"));
        } else if (c.kind == Control::Kind::Deny) {
            ++deny;
        } else if (c.kind == Control::Kind::Permanent) {
            ++permanent;
        }
    }
    QCOMPARE(approve, 1);
    QCOMPARE(deny, 1);
    QCOMPARE(permanent, 1);
}

void TranscriptRenderTests::clarifyEmitsChoiceAndFreeformControls() {
    be::DocumentStore doc;
    doc.loadMarkdown(interactiveMarkdown());
    const LayoutResult res = TranscriptLayout::build(doc, 80);

    int radios = 0, checkboxes = 0, freeform = 0, submit = 0;
    for (const Control& c : res.controls) {
        if (c.kind == Control::Kind::Choice) {
            (c.multi ? checkboxes : radios) += 1;
            QCOMPARE(c.callId, QStringLiteral("c7"));
        } else if (c.kind == Control::Kind::Freeform) {
            ++freeform;
            QCOMPARE(c.questionId, QStringLiteral("notes"));
        } else if (c.kind == Control::Kind::Submit) {
            ++submit;
            QCOMPARE(c.requestId, QStringLiteral("q1"));
        }
    }
    QCOMPARE(radios, 2);     // db: PostgreSQL / SQLite (single-select)
    QCOMPARE(checkboxes, 2); // scope: Schema / Data (multi-select)
    QCOMPARE(freeform, 1);   // notes: freeform
    QCOMPARE(submit, 1);

    const QString text = flatten(res.lines);
    QVERIFY(text.contains(QStringLiteral("( ) PostgreSQL")));
    QVERIFY(text.contains(QStringLiteral("[ ] Schema")));
    QVERIFY(text.contains(QStringLiteral("(select all that apply)")));
    QVERIFY(text.contains(QStringLiteral("Submit")));
}

void TranscriptRenderTests::clarifyDraftReflectsSelection() {
    be::DocumentStore doc;
    doc.loadMarkdown(interactiveMarkdown());

    // A draft that picks PostgreSQL (radio) and Schema (checkbox) and types a note.
    AnswerDraft draft;
    draft.selected.insert(QStringLiteral("db"), {QStringLiteral("PostgreSQL")});
    draft.selected.insert(QStringLiteral("scope"), {QStringLiteral("Schema")});
    draft.freeform.insert(QStringLiteral("notes"), QStringLiteral("be careful"));
    const QString text = flatten(TranscriptLayout::build(doc, 80, draft).lines);

    QVERIFY(text.contains(QStringLiteral("(o) PostgreSQL"))); // selected radio
    QVERIFY(text.contains(QStringLiteral("( ) SQLite")));     // unselected radio
    QVERIFY(text.contains(QStringLiteral("[x] Schema")));     // checked box
    QVERIFY(text.contains(QStringLiteral("[ ] Data")));       // unchecked box
    QVERIFY(text.contains(QStringLiteral("be careful")));     // freeform text

    // The collected answers map matches the canonical clarify contract.
    QVariantMap meta;
    for (const be::BlockRecord& b : doc.blocks()) {
        if (b.metadata.value(QStringLiteral("callId")).toString() == QStringLiteral("c7")) {
            meta = b.metadata;
        }
    }
    const QVariantMap answers = collectClarifyAnswers(meta, draft);
    QCOMPARE(answers.value(QStringLiteral("db")).toString(), QStringLiteral("PostgreSQL"));
    QCOMPARE(answers.value(QStringLiteral("scope")).toStringList(),
             QStringList{QStringLiteral("Schema")});
    QCOMPARE(answers.value(QStringLiteral("notes")).toString(), QStringLiteral("be careful"));
}

void TranscriptRenderTests::multiParagraphMessageKeepsBlankLines() {
    // A multiline composer message with an intentional blank line lands as two
    // markdown paragraphs sharing one message id. They must render with a blank
    // row between them (mirroring the GUI's paragraph margins) - a single newline
    // (one paragraph, soft-wrapped) stays tight, while the blank line is kept.
    be::DocumentStore doc;
    doc.loadMarkdown(QStringLiteral(R"md(```msg
{"id":"u1","role":"user"}
```

first paragraph

second paragraph
)md"));
    const QStringList rows =
        flatten(TranscriptLayout::build(doc, 80).lines).split(QLatin1Char('\n'));

    int firstIdx = -1, secondIdx = -1;
    for (int i = 0; i < rows.size(); ++i) {
        if (rows.at(i).contains(QStringLiteral("first paragraph"))) {
            firstIdx = i;
        } else if (rows.at(i).contains(QStringLiteral("second paragraph"))) {
            secondIdx = i;
        }
    }
    QVERIFY(firstIdx >= 0);
    QVERIFY(secondIdx > firstIdx);
    // Exactly one (blank) row separates the two paragraphs.
    QCOMPARE(secondIdx - firstIdx, 2);
    QVERIFY(rows.at(firstIdx + 1).trimmed().isEmpty());
}

void TranscriptRenderTests::blockLineMapsAddressEveryRow() {
    be::DocumentStore doc;
    doc.loadMarkdown(sampleMarkdown());
    const LayoutResult res = TranscriptLayout::build(doc, 80);

    // blockFirstLine is sized to the document's block count; lineBlock to the
    // produced rows - the two maps that let a find match scroll to / emphasise
    // its block.
    QCOMPARE(res.blockFirstLine.size(), doc.blocks().size());
    QCOMPARE(res.lineBlock.size(), res.lines.size());

    // Every rendered row is attributed to a real (in-range) document block.
    for (int li = 0; li < res.lineBlock.size(); ++li) {
        const int b = res.lineBlock.at(li);
        QVERIFY(b >= 0 && b < doc.blocks().size());
    }

    // Each non-tombstoned block has a first-line anchor that points at a row it
    // actually owns, and that row index is in range.
    for (int b = 0; b < doc.blocks().size(); ++b) {
        if (doc.blocks().at(b).tombstoned) {
            continue;
        }
        const int line = res.blockFirstLine.at(b);
        QVERIFY(line >= 0 && line < res.lines.size());
        QCOMPARE(res.lineBlock.at(line), b);
    }

    // The block holding the user's prose maps to a row that renders that text -
    // the exact relationship scrollBlockIntoView relies on.
    int proseBlock = -1;
    for (int b = 0; b < doc.blocks().size(); ++b) {
        if (doc.blocks().at(b).markdown().contains(QStringLiteral("Build it please."))) {
            proseBlock = b;
            break;
        }
    }
    QVERIFY(proseBlock >= 0);
    const int proseLine = res.blockFirstLine.at(proseBlock);
    QVERIFY(proseLine >= 0);
    QString rowText;
    for (const Span& s : res.lines.at(proseLine)) {
        rowText += s.text;
    }
    QVERIFY(rowText.contains(QStringLiteral("Build it please.")));
}

QTEST_MAIN(TranscriptRenderTests)

#include "tst_transcript_render.moc"
