#pragma once

#include <Tui/ZColor.h>
#include <Tui/ZCommon.h>

#include <QHash>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QVector>

namespace be {
class DocumentStore;
}

// A single styled run on one screen row: text plus its terminal colors and
// attributes. RenderLine is a fully wrapped row, ready to paint span-by-span via
// ZPainter::writeWithColors / writeWithAttributes.
struct Span {
    QString text;
    Tui::ZColor fg;
    Tui::ZColor bg;
    Tui::ZTextAttributes attr;
};

using RenderLine = QVector<Span>;

// An interactive hot-spot painted inside the transcript: an approval-bar button,
// a clarify choice (radio/checkbox), a clarify freeform field, or the clarify
// submit button. `line` is the row index in the built `lines` it sits on; the
// rest carries the routing keys the host needs to drive the answer.
struct Control {
    enum class Kind { Approve, Deny, Permanent, Choice, Freeform, Submit };

    int line = -1;
    Kind kind = Kind::Approve;
    QString callId;
    QString requestId;
    QString questionId; // clarify controls only
    int choiceIndex = -1; // Choice only
    bool multi = false;   // Choice: checkbox (multi-select) vs radio
    QString choiceLabel;  // Choice only (the choice's text)
};

// The transient, in-progress answer for the interactive block(s) currently being
// filled in, keyed by clarify question id. `selected` holds chosen choice labels
// (one for radios, many for checkboxes); `freeform` holds typed reply text. The
// build re-reads this each frame so the painted radios/checkboxes/field reflect
// the live draft.
struct AnswerDraft {
    QHash<QString, QStringList> selected;
    QHash<QString, QString> freeform;
};

// The result of a build: the wrapped rows plus the interactive controls found in
// them (empty when the transcript has no awaiting-approval / unanswered-clarify
// block).
struct LayoutResult {
    QVector<RenderLine> lines;
    QVector<Control> controls;
};

// The TUI analog of the GUI's QML block delegates: turns a parsed be::DocumentStore
// into wrapped, styled terminal rows. Pure (no widget/painter state) so it unit-
// tests headlessly against a DocumentStore loaded from markdown.
//
// Per BlockType it mirrors the matching QML component (MessageHeader, ReasoningBlock,
// ToolCallBlock, ContentBlock, CodeBlock, ...), reusing be::buildToolView /
// buildReasoningView / buildContentView / ansiToSpans / parseUnifiedDiff so both
// front ends share the exact same view-model logic.
class TranscriptLayout {
public:
    // Build the full transcript at the given content width (columns). A width <= 0
    // is treated as a sane minimum so callers can build before the first resize.
    // `draft` supplies the in-progress clarify answer (radios/checkboxes/field);
    // `activeControl` indexes the returned controls list to paint the focused
    // control with an active wash (-1 = none focused).
    static LayoutResult build(const be::DocumentStore &doc, int width,
                              const AnswerDraft &draft = {}, int activeControl = -1);
};

// Merge an in-progress AnswerDraft against a clarify tool's question list into
// the canonical answers map (questionId -> string | string list), mirroring
// ClarifyBlock.qml's collectAnswers: freeform text augments a multi-select list
// and fills an unanswered single-select. Returns the map ready for
// be::clarifyAnswerPatch.
QVariantMap collectClarifyAnswers(const QVariantMap &toolMetadata, const AnswerDraft &draft);
