// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "transcript_render.h"

#include "core/agent_block.h"
#include "core/block_record.h"
#include "core/document_store.h"
#include "tui_palette.h"

#include <QMetaType>
#include <QPair>
#include <QRegularExpression>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <utility>

namespace {

using Tui::ZColor;
using Tui::ZTextAttribute;
using Tui::ZTextAttributes;

// A resolved terminal style for a run of text.
struct Style {
    ZColor fg = tpal::fg();
    ZColor bg = tpal::bg();
    ZTextAttributes attr;
};

Span mkSpan(const QString& text, const Style& s) {
    return Span{text, s.fg, s.bg, s.attr};
}

bool sameStyle(const Span& a, const Style& b) {
    return a.fg == b.fg && a.bg == b.bg && a.attr == b.attr;
}

// --- text wrapping ----------------------------------------------------------

// Greedy word-wrap. Internal whitespace is collapsed; blank source lines are
// preserved as empty rows. Words longer than `width` are hard-broken.
QStringList wordWrap(const QString& text, int width) {
    const int w = qMax(1, width);
    QStringList out;
    const QStringList paras = text.split(QLatin1Char('\n'));
    for (const QString& para : paras) {
        static const QRegularExpression ws(QStringLiteral("\\s+"));
        const QStringList words = para.split(ws, Qt::SkipEmptyParts);
        if (words.isEmpty()) {
            out.push_back(QString());
            continue;
        }
        QString line;
        for (const QString& wd : words) {
            QString word = wd;
            while (word.size() > w) {
                if (!line.isEmpty()) {
                    out.push_back(line);
                    line.clear();
                }
                out.push_back(word.left(w));
                word = word.mid(w);
            }
            if (line.isEmpty()) {
                line = word;
            } else if (line.size() + 1 + word.size() <= w) {
                line += QLatin1Char(' ') + word;
            } else {
                out.push_back(line);
                line = word;
            }
        }
        if (!line.isEmpty()) {
            out.push_back(line);
        }
    }
    return out;
}

// Parse light inline emphasis (**bold**, *italic*/_italic_, `code`) over a base
// style into styled runs. Operates on one already-wrapped line.
RenderLine inlineRuns(const QString& line, const Style& base) {
    RenderLine runs;
    QString buf;
    const auto flush = [&] {
        if (!buf.isEmpty()) {
            runs.push_back(mkSpan(buf, base));
            buf.clear();
        }
    };
    const qsizetype n = line.size();
    qsizetype i = 0;
    while (i < n) {
        const QChar c = line.at(i);
        if (c == QLatin1Char('`')) {
            const qsizetype j = line.indexOf(QLatin1Char('`'), i + 1);
            if (j > i) {
                flush();
                Style cs = base;
                cs.bg = tpal::codeBg();
                cs.fg = tpal::fg();
                cs.attr = {};
                runs.push_back(mkSpan(line.mid(i + 1, j - i - 1), cs));
                i = j + 1;
                continue;
            }
        }
        if (c == QLatin1Char('*') && i + 1 < n && line.at(i + 1) == QLatin1Char('*')) {
            const qsizetype j = line.indexOf(QStringLiteral("**"), i + 2);
            if (j > i) {
                flush();
                Style bs = base;
                bs.attr |= ZTextAttribute::Bold;
                runs.push_back(mkSpan(line.mid(i + 2, j - i - 2), bs));
                i = j + 2;
                continue;
            }
        }
        if (c == QLatin1Char('*') || c == QLatin1Char('_')) {
            const qsizetype j = line.indexOf(c, i + 1);
            if (j > i + 1) {
                flush();
                Style is = base;
                is.attr |= ZTextAttribute::Italic;
                runs.push_back(mkSpan(line.mid(i + 1, j - i - 1), is));
                i = j + 1;
                continue;
            }
        }
        buf.append(c);
        ++i;
    }
    flush();
    if (runs.isEmpty()) {
        runs.push_back(mkSpan(QString(), base));
    }
    return runs;
}

// --- low-level emitters -----------------------------------------------------

// Word-wrapped prose with inline emphasis.
void emitProse(QVector<RenderLine>& dst, const QString& text, int width, const Style& base) {
    const QStringList wrapped = wordWrap(text, width);
    for (const QString& wl : wrapped) {
        dst.push_back(inlineRuns(wl, base));
    }
}

// One mono (no inline parsing) source line, char-wrapped, uniform style.
void emitMonoLine(QVector<RenderLine>& dst, const QString& text, int width, const Style& s) {
    const int w = qMax(1, width);
    if (text.isEmpty()) {
        dst.push_back(RenderLine{mkSpan(QString(), s)});
        return;
    }
    for (int i = 0; i < text.size(); i += w) {
        dst.push_back(RenderLine{mkSpan(text.mid(i, w), s)});
    }
}

// Multi-line mono text (splits on newlines, then char-wraps each line).
void emitMono(QVector<RenderLine>& dst, const QString& text, int width, const Style& s) {
    const QStringList lines = text.split(QLatin1Char('\n'));
    for (const QString& l : lines) {
        emitMonoLine(dst, l, width, s);
    }
}

// ANSI/SGR terminal text -> styled, char-wrapped rows (newlines preserved).
void emitAnsi(QVector<RenderLine>& dst, const QString& text, int width) {
    const int w = qMax(1, width);
    const QVariantList spans = be::ansiToSpans(text);
    RenderLine cur;
    int col = 0;
    const auto flush = [&] {
        dst.push_back(cur);
        cur = RenderLine{};
        col = 0;
    };
    for (const QVariant& sv : spans) {
        const QVariantMap m = sv.toMap();
        Style st;
        const int fg = m.value(QStringLiteral("fg")).toInt();
        const int bg = m.value(QStringLiteral("bg")).toInt();
        st.fg = tpal::ansi(fg);
        st.bg = tpal::ansiBg(bg);
        ZTextAttributes a{};
        if (m.value(QStringLiteral("bold")).toBool()) {
            a |= ZTextAttribute::Bold;
        }
        if (m.value(QStringLiteral("italic")).toBool()) {
            a |= ZTextAttribute::Italic;
        }
        if (m.value(QStringLiteral("underline")).toBool()) {
            a |= ZTextAttribute::Underline;
        }
        if (m.value(QStringLiteral("dim")).toBool()) {
            st.fg = tpal::faint();
        }
        if (m.value(QStringLiteral("inverse")).toBool()) {
            std::swap(st.fg, st.bg);
        }
        st.attr = a;
        const QString t = m.value(QStringLiteral("text")).toString();
        for (const QChar c : t) {
            if (c == QLatin1Char('\n')) {
                flush();
                continue;
            }
            if (col >= w) {
                flush();
            }
            if (!cur.isEmpty() && sameStyle(cur.last(), st)) {
                cur.last().text.append(c);
            } else {
                cur.push_back(mkSpan(QString(c), st));
            }
            ++col;
        }
    }
    if (!cur.isEmpty()) {
        flush();
    }
}

// Unified diff -> red/green/hunk styled rows on a code background.
void emitDiff(QVector<RenderLine>& dst, const QString& diff, int width) {
    const QVariantList lines = be::parseUnifiedDiff(diff);
    for (const QVariant& v : lines) {
        const QVariantMap m = v.toMap();
        const QString kind = m.value(QStringLiteral("kind")).toString();
        Style s;
        s.bg = tpal::codeBg();
        if (kind == QStringLiteral("add")) {
            s.fg = tpal::diffAdd();
        } else if (kind == QStringLiteral("del")) {
            s.fg = tpal::diffDel();
        } else if (kind == QStringLiteral("hunk")) {
            s.fg = tpal::diffHunk();
        } else if (kind == QStringLiteral("meta")) {
            s.fg = tpal::muted();
        } else {
            s.fg = tpal::fg();
        }
        emitMonoLine(dst, m.value(QStringLiteral("text")).toString(), width, s);
    }
}

// --- card framing -----------------------------------------------------------

// Build a card's left rule prefix (a tone-colored bar + a space).
RenderLine barPrefix(const ZColor& tone) {
    Style bar;
    bar.fg = tone;
    Style sp;
    return RenderLine{mkSpan(tpal::barGlyph(), bar), mkSpan(QStringLiteral(" "), sp)};
}

// Append `inner` rows to `dst`, each prefixed with the card bar.
void emitCard(QVector<RenderLine>& dst, const ZColor& tone, const QVector<RenderLine>& inner) {
    for (const RenderLine& row : inner) {
        RenderLine line = barPrefix(tone);
        line += row;
        dst.push_back(line);
    }
}

void addBlank(QVector<RenderLine>& dst) {
    dst.push_back(RenderLine{});
}

// --- block helpers ----------------------------------------------------------

QString stripLeading(const QString& text, const QRegularExpression& re) {
    QString out = text;
    const QRegularExpressionMatch m = re.match(out);
    if (m.hasMatch()) {
        out.remove(0, m.capturedLength());
    }
    return out;
}

QString headingText(const be::BlockRecord& b) {
    static const QRegularExpression re(QStringLiteral("^\\s*#{1,6}\\s+"));
    return stripLeading(b.markdown(), re).trimmed();
}

// --- message header ---------------------------------------------------------

void emitMessageHeader(QVector<RenderLine>& dst, be::MessageRole role, bool first, int /*width*/) {
    if (!first) {
        addBlank(dst); // turn gap
    }
    QString label;
    ZColor tone;
    switch (role) {
    case be::MessageRole::User:
        label = QObject::tr("YOU");
        tone = tpal::accent();
        break;
    case be::MessageRole::Assistant:
        label = QObject::tr("DAEMON");
        tone = tpal::statusOk();
        break;
    default:
        return; // System has no banner; its blocks render as notices.
    }
    Style bar;
    bar.fg = tone;
    Style txt;
    txt.fg = tone;
    txt.attr |= ZTextAttribute::Bold;
    Style sp;
    dst.push_back(RenderLine{
        mkSpan(tpal::barGlyph(), bar),
        mkSpan(QStringLiteral(" "), sp),
        mkSpan(label, txt),
    });
}

void emitSystemNotice(QVector<RenderLine>& dst, const QString& text, int width) {
    Style s;
    s.fg = tpal::muted();
    s.attr |= ZTextAttribute::Italic;
    const QStringList wrapped = wordWrap(text, qMax(1, width - 4));
    for (const QString& wl : wrapped) {
        const int pad = qMax(0, static_cast<int>(width - wl.size()) / 2);
        Style sp;
        RenderLine line;
        if (pad > 0) {
            line.push_back(mkSpan(QString(pad, QLatin1Char(' ')), sp));
        }
        line.push_back(mkSpan(wl, s));
        dst.push_back(line);
    }
}

// --- agent block delegates --------------------------------------------------

void emitReasoning(QVector<RenderLine>& dst, const be::BlockRecord& b, int width) {
    const QVariantMap view = be::buildReasoningView(b.metadata);
    const int inner = qMax(1, width - 2);
    const bool running =
        view.value(QStringLiteral("status")).toString() == QStringLiteral("running");
    const QString duration = view.value(QStringLiteral("durationLabel")).toString();

    QVector<RenderLine> body;
    Style head;
    head.fg = tpal::faint();
    head.attr |= ZTextAttribute::Bold;
    QString headText = tpal::reasoningGlyph() + QLatin1Char(' ') +
                       (running ? QObject::tr("Reasoning\u2026") : QObject::tr("Reasoning"));
    RenderLine headLine{mkSpan(headText, head)};
    if (!duration.isEmpty()) {
        Style dim;
        dim.fg = tpal::muted();
        headLine.push_back(mkSpan(QStringLiteral("  ") + duration, dim));
    }
    body.push_back(headLine);

    Style prose;
    prose.fg = tpal::faint();
    prose.attr |= ZTextAttribute::Italic;
    emitProse(body, view.value(QStringLiteral("body")).toString(), inner, prose);

    emitCard(dst, tpal::muted(), body);
    addBlank(dst);
}

// Web/file search hits: bold accent title, muted url, prose snippet per hit.
void emitSearchResults(QVector<RenderLine>& body, const QVariantList& hits, int inner) {
    for (const QVariant& hv : hits) {
        const QVariantMap hit = hv.toMap();
        Style title;
        title.fg = tpal::accent();
        title.attr |= ZTextAttribute::Bold;
        emitProse(body, hit.value(QStringLiteral("title")).toString(), inner, title);
        Style url;
        url.fg = tpal::muted();
        emitMono(body, hit.value(QStringLiteral("url")).toString(), inner, url);
        Style snip;
        snip.fg = tpal::fg();
        emitProse(body, hit.value(QStringLiteral("snippet")).toString(), inner, snip);
    }
}

void emitToolBody(QVector<RenderLine>& body, const QVariantMap& view, int inner) {
    const QString detail = view.value(QStringLiteral("detailKind")).toString();
    if (detail == QStringLiteral("ansi-stream") || detail == QStringLiteral("pty")) {
        const QString out = view.value(QStringLiteral("stdout")).toString();
        const QString err = view.value(QStringLiteral("stderr")).toString();
        if (!out.isEmpty()) {
            emitAnsi(body, out, inner);
        }
        if (!err.isEmpty()) {
            emitAnsi(body, err, inner);
        }
        // D1: a shell/execute detail carries the exit code - a trailing muted line (body-only;
        // the GUI shows the same code inside the node's own output framing).
        if (view.contains(QStringLiteral("exitCode"))) {
            Style dim;
            dim.fg = tpal::muted();
            emitMonoLine(body,
                         QObject::tr("exit %1").arg(view.value(QStringLiteral("exitCode")).toInt()),
                         inner, dim);
        }
        return;
    }
    if (detail == QStringLiteral("diff")) {
        emitDiff(body, view.value(QStringLiteral("diff")).toString(), inner);
        return;
    }
    if (detail == QStringLiteral("search-results")) {
        emitSearchResults(body, view.value(QStringLiteral("hits")).toList(), inner);
        return;
    }
    if (detail == QStringLiteral("image") || detail == QStringLiteral("generated-image")) {
        Style s;
        s.fg = tpal::muted();
        const QString url = view.value(QStringLiteral("imageUrl")).toString();
        emitMono(body, QObject::tr("[image: ") + url + QLatin1Char(']'), inner, s);
        return;
    }
    // Default: a free-text body, else any stdout/stderr present.
    const QString fallback = view.value(QStringLiteral("body")).toString();
    Style s;
    s.fg = tpal::fg();
    if (!fallback.isEmpty()) {
        emitMono(body, fallback, inner, s);
        return;
    }
    const QString out = view.value(QStringLiteral("stdout")).toString();
    const QString err = view.value(QStringLiteral("stderr")).toString();
    if (!out.isEmpty()) {
        emitMono(body, out, inner, s);
    }
    if (!err.isEmpty()) {
        Style es;
        es.fg = tpal::statusError();
        emitMono(body, err, inner, es);
    }
}

// Normalize a clarify tool's metadata into the list of question maps the form
// renders (mirrors ClarifyBlock.qml's `questions` normalization, including the
// legacy single-question fallback).
QVariantList clarifyQuestions(const QVariantMap& metadata) {
    QVariantList qs = metadata.value(QStringLiteral("questions")).toList();
    if (!qs.isEmpty()) {
        return qs;
    }
    QVariantMap one;
    one.insert(QStringLiteral("id"), QStringLiteral("q"));
    one.insert(
        QStringLiteral("prompt"),
        metadata.value(QStringLiteral("question"), QObject::tr("The agent needs your input."))
            .toString());
    one.insert(QStringLiteral("choices"), metadata.value(QStringLiteral("choices")).toList());
    one.insert(QStringLiteral("multiSelect"), false);
    one.insert(QStringLiteral("allowFreeform"), true);
    return QVariantList{one};
}

QString clarifyQuestionId(const QVariantMap& q, int index) {
    const QVariant id = q.value(QStringLiteral("id"));
    return id.isValid() ? id.toString() : (QStringLiteral("q") + QString::number(index));
}

// A choice/button style: an active control gets an accent wash; otherwise a
// muted surface chip in `toneFg`.
Style controlStyle(bool active, const ZColor& toneFg) {
    Style s;
    if (active) {
        s.fg = tpal::bg();
        s.bg = tpal::accent();
        s.attr |= ZTextAttribute::Bold;
    } else {
        s.fg = toneFg;
        s.bg = tpal::surfaceAlt();
    }
    return s;
}

// The per-build inputs threaded to every block emitter: the content width plus
// the interactive-answer state. `controlSeq` is the running global control
// counter and persists across blocks, so BuildCtx is passed by reference.
struct BuildCtx {
    int width;
    const AnswerDraft& draft;
    int activeControl;
    int controlSeq = 0;
};

// State for the interactive-control emitters of one tool card: the in-progress
// draft, the focused control index, the running counter, the inner content width,
// the call/request ids stamped onto every emitted control, and the (body-row,
// control) pairs the caller maps to absolute dst rows after framing.
struct ControlBuilder {
    const AnswerDraft& draft;
    int activeControl;
    int& controlSeq;
    int inner;
    QString callId;
    QString requestId;
    QVector<QPair<int, Control>>& pending;

    void add(int bodyRow, Control c) {
        c.callId = callId;
        c.requestId = requestId;
        pending.push_back({bodyRow, c});
    }
};

// Tool-card header row: <status glyph> <tone glyph> <bold title> [dim subtitle]
// [dim duration].
RenderLine toolHeader(const QVariantMap& view, const ZColor& statusColor) {
    const QString status = view.value(QStringLiteral("status")).toString();
    const QString tone = view.value(QStringLiteral("tone")).toString();
    const QString title = view.value(QStringLiteral("title")).toString();
    const QString subtitle = view.value(QStringLiteral("subtitle")).toString();
    const QString duration = view.value(QStringLiteral("durationLabel")).toString();

    Style st;
    st.fg = statusColor;
    Style tn;
    tn.fg = tpal::toneColor(tone);
    Style ttl;
    ttl.fg = tpal::fg();
    ttl.attr |= ZTextAttribute::Bold;
    Style dim;
    dim.fg = tpal::muted();
    RenderLine header{
        mkSpan(tpal::statusGlyph(status), st),
        mkSpan(QStringLiteral(" "), Style{}),
        mkSpan(tpal::toneGlyph(tone), tn),
        mkSpan(QStringLiteral(" "), Style{}),
        mkSpan(title, ttl),
    };
    if (!subtitle.isEmpty()) {
        header.push_back(mkSpan(QStringLiteral("  ") + subtitle, dim));
    }
    if (!duration.isEmpty()) {
        header.push_back(mkSpan(QStringLiteral("  ") + duration, dim));
    }
    return header;
}

// Awaiting-approval controls: a warn command line + Approve / Deny / [Allow
// permanently] buttons painted on one row.
void emitApprovalBar(QVector<RenderLine>& body, const be::BlockRecord& b, const QVariantMap& view,
                     ControlBuilder& cb) {
    Style warn;
    warn.fg = tpal::warn();
    warn.attr |= ZTextAttribute::Bold;
    QString cmd = view.value(QStringLiteral("approvalCommand")).toString();
    if (cmd.isEmpty()) {
        cmd = view.value(QStringLiteral("subtitle")).toString();
    }
    emitMono(body, QObject::tr("\u26a0 Approve: ") + cmd, cb.inner, warn);

    const bool allowPermanent = b.metadata.value(QStringLiteral("allowPermanent")).toBool();
    Style sp;
    RenderLine row;
    const int rowIdx = static_cast<int>(body.size());
    const auto button = [&](const QString& label, Control::Kind kind, const ZColor& toneFg) {
        const int gi = cb.controlSeq++;
        row.push_back(mkSpan(QStringLiteral(" "), sp));
        row.push_back(mkSpan(QStringLiteral(" ") + label + QStringLiteral(" "),
                             controlStyle(gi == cb.activeControl, toneFg)));
        Control c;
        c.kind = kind;
        cb.add(rowIdx, c);
    };
    button(QObject::tr("Approve"), Control::Kind::Approve, tpal::statusOk());
    button(QObject::tr("Deny"), Control::Kind::Deny, tpal::statusError());
    // [wave2:app-approvals-safety] D3: deny-with-reason (wire v29) — opens an operator reason
    // prompt.
    button(QObject::tr("Deny with reason"), Control::Kind::DenyReason, tpal::statusError());
    if (allowPermanent) {
        button(QObject::tr("Allow permanently"), Control::Kind::Permanent, tpal::warn());
    }
    body.push_back(row);
}

// The radio/checkbox rows for one clarify question (one control per choice).
void emitClarifyChoices(QVector<RenderLine>& body, const QVariantMap& q, const QString& qid,
                        ControlBuilder& cb) {
    const QStringList choices = q.value(QStringLiteral("choices")).toStringList();
    const bool multi = q.value(QStringLiteral("multiSelect")).toBool();
    const QStringList sel = cb.draft.selected.value(qid);
    for (int ci = 0; ci < choices.size(); ++ci) {
        const QString& label = choices.at(ci);
        const bool on = sel.contains(label);
        const QString box = multi ? (on ? QStringLiteral("[x] ") : QStringLiteral("[ ] "))
                                  : (on ? QStringLiteral("(o) ") : QStringLiteral("( ) "));
        const int gi = cb.controlSeq++;
        const Style s = controlStyle(gi == cb.activeControl, on ? tpal::accent() : tpal::fg());
        const int rowIdx = static_cast<int>(body.size());
        body.push_back(RenderLine{mkSpan(QStringLiteral("  ") + box + label, s)});
        Control c;
        c.kind = Control::Kind::Choice;
        c.questionId = qid;
        c.choiceIndex = ci;
        c.multi = multi;
        c.choiceLabel = label;
        cb.add(rowIdx, c);
    }
}

// The freeform reply field for one clarify question: a live caret when focused,
// the typed text otherwise, or an italic placeholder when empty and unfocused.
void emitClarifyFreeform(QVector<RenderLine>& body, const QString& qid, bool choicesEmpty,
                         ControlBuilder& cb) {
    const QString typed = cb.draft.freeform.value(qid);
    const int gi = cb.controlSeq++;
    const bool active = gi == cb.activeControl;
    Style s = controlStyle(active, tpal::fg());
    const int rowIdx = static_cast<int>(body.size());
    if (typed.isEmpty() && !active) {
        Style ph;
        ph.fg = tpal::muted();
        ph.attr |= ZTextAttribute::Italic;
        const QString placeholder =
            choicesEmpty ? QObject::tr("Type a reply\u2026") : QObject::tr("Or type a reply\u2026");
        body.push_back(RenderLine{mkSpan(QStringLiteral("  \u203a "), s), mkSpan(placeholder, ph)});
    } else {
        const QString text = typed + (active ? QStringLiteral("\u2588") : QString());
        body.push_back(RenderLine{mkSpan(QStringLiteral("  \u203a ") + text, s)});
    }
    Control c;
    c.kind = Control::Kind::Freeform;
    c.questionId = qid;
    cb.add(rowIdx, c);
}

// One clarify question: bold prompt, optional multi-select hint, choices, and an
// optional freeform field.
void emitClarifyQuestion(QVector<RenderLine>& body, const QVariantMap& q, int qi,
                         ControlBuilder& cb) {
    const QString qid = clarifyQuestionId(q, qi);
    const QStringList choices = q.value(QStringLiteral("choices")).toStringList();
    const bool multi = q.value(QStringLiteral("multiSelect")).toBool();
    const QVariant af = q.value(QStringLiteral("allowFreeform"));
    const bool allowFreeform = af.isValid() ? af.toBool() : choices.isEmpty();

    Style prompt;
    prompt.fg = tpal::fg();
    prompt.attr |= ZTextAttribute::Bold;
    emitProse(body, q.value(QStringLiteral("prompt")).toString(), cb.inner, prompt);
    if (multi) {
        Style hint;
        hint.fg = tpal::muted();
        hint.attr |= ZTextAttribute::Italic;
        body.push_back(RenderLine{mkSpan(QObject::tr("  (select all that apply)"), hint)});
    }

    emitClarifyChoices(body, q, qid, cb);
    if (allowFreeform) {
        emitClarifyFreeform(body, qid, choices.isEmpty(), cb);
    }
}

// The unanswered clarify form: every question followed by a Submit button.
void emitClarifyForm(QVector<RenderLine>& body, const be::BlockRecord& b, ControlBuilder& cb) {
    const QVariantList questions = clarifyQuestions(b.metadata);
    for (int qi = 0; qi < questions.size(); ++qi) {
        emitClarifyQuestion(body, questions.at(qi).toMap(), qi, cb);
    }

    const int gi = cb.controlSeq++;
    Style sp;
    const int rowIdx = static_cast<int>(body.size());
    RenderLine row{mkSpan(QStringLiteral("  "), sp),
                   mkSpan(QStringLiteral(" ") + QObject::tr("Submit") + QStringLiteral(" "),
                          controlStyle(gi == cb.activeControl, tpal::statusOk()))};
    body.push_back(row);
    Control c;
    c.kind = Control::Kind::Submit;
    cb.add(rowIdx, c);
}

// The resolved clarify summary: one checkmark row per answered question.
void emitClarifyResolved(QVector<RenderLine>& body, const be::BlockRecord& b) {
    const QVariantList questions = clarifyQuestions(b.metadata);
    const QVariantMap answers = b.metadata.value(QStringLiteral("answers")).toMap();
    for (int qi = 0; qi < questions.size(); ++qi) {
        const QVariantMap q = questions.at(qi).toMap();
        const QString qid = clarifyQuestionId(q, qi);
        const QVariant val = answers.value(qid);
        if (!val.isValid()) {
            continue;
        }
        const QString shown = val.typeId() == QMetaType::QVariantList
                                  ? val.toStringList().join(QStringLiteral(", "))
                                  : val.toString();
        if (shown.isEmpty()) {
            continue;
        }
        Style ok;
        ok.fg = tpal::statusOk();
        Style promptStyle;
        promptStyle.fg = tpal::muted();
        Style val2;
        val2.fg = tpal::fg();
        val2.attr |= ZTextAttribute::Bold;
        body.push_back(RenderLine{
            mkSpan(QStringLiteral("\u2713 "), ok),
            mkSpan(q.value(QStringLiteral("prompt")).toString() + QStringLiteral(": "),
                   promptStyle),
            mkSpan(shown, val2),
        });
    }
}

void emitToolCall(QVector<RenderLine>& dst, QVector<Control>& controls, const be::BlockRecord& b,
                  BuildCtx& ctx) {
    const QVariantMap view = be::buildToolView(b.metadata);
    const int inner = qMax(1, ctx.width - 2);
    const QString status = view.value(QStringLiteral("status")).toString();
    const QString tone = view.value(QStringLiteral("tone")).toString();
    const bool awaitingApproval = view.value(QStringLiteral("awaitingApproval")).toBool();
    const QString variant = view.value(QStringLiteral("variant")).toString();
    const bool answered = view.value(QStringLiteral("answered")).toBool();

    ZColor barColor = tpal::toneColor(tone);
    ZColor statusColor = tpal::statusOk();
    if (status == QStringLiteral("running")) {
        statusColor = tpal::statusRunning();
        barColor = tpal::statusRunning();
    } else if (status == QStringLiteral("error")) {
        statusColor = tpal::statusError();
        barColor = tpal::statusError();
    }

    QVector<RenderLine> body;
    body.push_back(toolHeader(view, statusColor));
    emitToolBody(body, view, inner);

    // Pending interactive controls, recorded as (body-row, partially-filled
    // Control). After emitCard maps body rows to absolute dst rows we patch each
    // control's `line` and append it to the output controls list.
    QVector<QPair<int, Control>> pending;
    ControlBuilder cb{ctx.draft,
                      ctx.activeControl,
                      ctx.controlSeq,
                      inner,
                      view.value(QStringLiteral("callId")).toString(),
                      view.value(QStringLiteral("requestId")).toString(),
                      pending};

    if (awaitingApproval) {
        emitApprovalBar(body, b, view, cb);
    } else if (variant == QStringLiteral("clarify") && !answered) {
        emitClarifyForm(body, b, cb);
    } else if (variant == QStringLiteral("clarify") && answered) {
        emitClarifyResolved(body, b);
    }

    const int dstStart = static_cast<int>(dst.size());
    emitCard(dst, barColor, body);
    for (const auto& pc : pending) {
        Control c = pc.second;
        c.line = dstStart + pc.first;
        controls.push_back(c);
    }
    addBlank(dst);
}

void emitContent(QVector<RenderLine>& dst, const be::BlockRecord& b, int width) {
    const QVariantMap view = be::buildContentView(b.metadata);
    const int inner = qMax(1, width - 2);
    const QString kind = view.value(QStringLiteral("kind")).toString();
    const QString bodyText = view.value(QStringLiteral("body")).toString();

    QVector<RenderLine> body;
    if (kind == QStringLiteral("ansi-stream") || kind == QStringLiteral("pty")) {
        emitAnsi(body, bodyText, inner);
    } else {
        Style s;
        s.fg = tpal::fg();
        emitMono(body, bodyText, inner, s);
    }
    emitCard(dst, tpal::muted(), body);
    addBlank(dst);
}

void emitCodeFence(QVector<RenderLine>& dst, const be::BlockRecord& b, int width) {
    const int inner = qMax(1, width - 2);
    const QString lang = b.metadata.value(QStringLiteral("fenceLanguage")).toString();
    const QString code = be::fencedBodyOf(b.markdown());

    QVector<RenderLine> body;
    Style label;
    label.fg = tpal::muted();
    label.bg = tpal::codeBg();
    body.push_back(RenderLine{
        mkSpan(QStringLiteral("\u2039") + (lang.isEmpty() ? QObject::tr("code") : lang) +
                   QStringLiteral("\u203a"),
               label),
    });
    Style mono;
    mono.fg = tpal::fg();
    mono.bg = tpal::codeBg();
    emitMono(body, code, inner, mono);
    emitCard(dst, tpal::toneColor(QStringLiteral("code")), body);
    addBlank(dst);
}

void emitList(QVector<RenderLine>& dst, const be::BlockRecord& b, int width) {
    static const QRegularExpression bulletRe(QStringLiteral("^(\\s*)([-*+])\\s+"));
    static const QRegularExpression orderedRe(QStringLiteral("^(\\s*)(\\d+[.)])\\s+"));
    static const QRegularExpression taskRe(QStringLiteral("^(\\s*)[-*+]\\s+\\[([ xX])\\]\\s+"));

    QString text = b.markdown();
    QString marker = QStringLiteral("\u2022"); // •
    Style markerStyle;
    markerStyle.fg = tpal::accent();

    if (b.type == be::BlockType::TaskListItem) {
        const QRegularExpressionMatch m = taskRe.match(text);
        if (m.hasMatch()) {
            const bool done =
                m.captured(1).trimmed().compare(QStringLiteral("x"), Qt::CaseInsensitive) == 0;
            marker = done ? QStringLiteral("\u2713") : QStringLiteral("\u25cb"); // ✓ / ○
            markerStyle.fg = done ? tpal::statusOk() : tpal::muted();
            text.remove(0, m.capturedLength());
        }
    } else if (b.type == be::BlockType::OrderedListItem) {
        const QRegularExpressionMatch m = orderedRe.match(text);
        if (m.hasMatch()) {
            marker = m.captured(2);
            text.remove(0, m.capturedLength());
        }
    } else {
        const QRegularExpressionMatch m = bulletRe.match(text);
        if (m.hasMatch()) {
            text.remove(0, m.capturedLength());
        }
    }

    const int indentCols = 2 + static_cast<int>(b.indent);
    const int inner = qMax(1, width - indentCols - 2);
    Style base;
    base.fg = tpal::fg();
    QVector<RenderLine> wrapped;
    emitProse(wrapped, text.trimmed(), inner, base);

    Style sp;
    for (int i = 0; i < wrapped.size(); ++i) {
        RenderLine line;
        line.push_back(mkSpan(QString(indentCols, QLatin1Char(' ')), sp));
        if (i == 0) {
            line.push_back(mkSpan(marker, markerStyle));
            line.push_back(mkSpan(QStringLiteral(" "), sp));
        } else {
            line.push_back(mkSpan(QStringLiteral("  "), sp));
        }
        line += wrapped[i];
        dst.push_back(line);
    }
}

void emitQuote(QVector<RenderLine>& dst, const be::BlockRecord& b, int width) {
    static const QRegularExpression quoteRe(QStringLiteral("^\\s*>\\s?"));
    const QString text = stripLeading(b.markdown(), quoteRe);
    const int inner = qMax(1, width - 2);
    Style base;
    base.fg = tpal::faint();
    base.attr |= ZTextAttribute::Italic;
    QVector<RenderLine> wrapped;
    emitProse(wrapped, text, inner, base);
    emitCard(dst, tpal::muted(), wrapped);
}

void emitTable(QVector<RenderLine>& dst, const be::BlockRecord& b, int width) {
    const QStringList lines = b.markdown().split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    static const QRegularExpression sepRe(QStringLiteral("^\\s*\\|?[\\s:\\-|]+\\|?\\s*$"));
    for (const QString& l : lines) {
        Style s;
        s.fg = sepRe.match(l).hasMatch() ? tpal::muted() : tpal::fg();
        emitMonoLine(dst, l, width, s);
    }
    addBlank(dst);
}

void emitParagraph(QVector<RenderLine>& dst, const be::BlockRecord& b, int width) {
    if (b.role == be::MessageRole::System) {
        emitSystemNotice(dst, b.markdown().trimmed(), width);
        return;
    }
    Style base;
    base.fg = tpal::fg();
    emitProse(dst, b.markdown(), width, base);
}

// The concatenated markdown of every (non-tombstoned) block tagged with
// `messageId`, used to seed an anchor for restore / edit-composer prefill.
QString messageTextFor(const be::DocumentStore& doc, const QString& id) {
    QStringList parts;
    for (const be::BlockRecord& mb : doc.blocks()) {
        if (!mb.tombstoned && mb.messageId == id) {
            parts << mb.markdown();
        }
    }
    return parts.join(QStringLiteral("\n\n"));
}

// Carries the message/paragraph transition state across the block loop so headers,
// turn gaps, and paragraph margins land exactly where the GUI puts them.
struct FlowState {
    QString prevMessageId;
    be::MessageRole prevRole = be::MessageRole::None;
    be::BlockType prevType = be::BlockType::Unknown;
    bool first = true;

    // Emit the inter-block separators before block `b`: a message banner (and
    // rewind anchor for user messages) when entering a new message, or a blank
    // row between consecutive paragraphs of one message. Advances the state.
    void emitSeparators(LayoutResult& result, const be::DocumentStore& doc,
                        const be::BlockRecord& b, int width) {
        const bool msgChanged = (b.messageId != prevMessageId) || (b.role != prevRole);
        if (b.role != be::MessageRole::None && msgChanged && !b.messageId.isEmpty()) {
            emitMessageHeader(result.lines, b.role, first, width);
            // A user message header is a rewind anchor: record its banner row,
            // routing id, and own text for restore / edit prefill.
            if (b.role == be::MessageRole::User) {
                result.anchors.push_back(Anchor{static_cast<int>(result.lines.size()) - 1,
                                                b.messageId, messageTextFor(doc, b.messageId)});
            }
        } else if (!msgChanged && b.type == be::BlockType::Paragraph &&
                   prevType == be::BlockType::Paragraph) {
            // Consecutive paragraphs in one message render with a blank row between
            // them (mirrors the GUI's paragraph margins), so a multiline composer
            // message keeps the blank lines the user typed (markdown collapses each
            // run of blank lines into a single paragraph break).
            addBlank(result.lines);
        }
        prevMessageId = b.messageId;
        prevRole = b.role;
        prevType = b.type;
        first = false;
    }
};

// Render one document block's rows (and any interactive controls) per its type,
// the TUI analog of the GUI's QML block-delegate switch.
void emitBlock(QVector<RenderLine>& out, QVector<Control>& controls, const be::BlockRecord& b,
               BuildCtx& ctx) {
    const int W = ctx.width;
    switch (b.type) {
    case be::BlockType::Reasoning:
        emitReasoning(out, b, W);
        break;
    case be::BlockType::ToolCall:
        emitToolCall(out, controls, b, ctx);
        break;
    case be::BlockType::Content:
        emitContent(out, b, W);
        break;
    case be::BlockType::CodeFence:
        emitCodeFence(out, b, W);
        break;
    case be::BlockType::Heading: {
        Style s;
        s.fg = tpal::accent();
        s.attr |= ZTextAttribute::Bold;
        emitProse(out, headingText(b), W, s);
        break;
    }
    case be::BlockType::BulletListItem:
    case be::BlockType::OrderedListItem:
    case be::BlockType::TaskListItem:
        emitList(out, b, W);
        break;
    case be::BlockType::Quote:
        emitQuote(out, b, W);
        break;
    case be::BlockType::Table:
        emitTable(out, b, W);
        break;
    case be::BlockType::HorizontalRule: {
        Style s;
        s.fg = tpal::muted();
        out.push_back(RenderLine{mkSpan(QString(W, QChar(0x2500)), s)});
        break;
    }
    case be::BlockType::Image: {
        Style s;
        s.fg = tpal::muted();
        emitMono(out, QObject::tr("[image] ") + b.markdown().trimmed(), W, s);
        break;
    }
    case be::BlockType::Math: {
        Style s;
        s.fg = tpal::muted();
        emitMono(out, QObject::tr("[math] ") + be::fencedBodyOf(b.markdown()).trimmed(), W, s);
        break;
    }
    case be::BlockType::Paragraph:
        emitParagraph(out, b, W);
        break;
    default: {
        const QString raw = b.markdown().trimmed();
        if (!raw.isEmpty()) {
            Style s;
            s.fg = tpal::muted();
            emitMono(out, raw, W, s);
        }
        break;
    }
    }
}

} // namespace

QVariantMap collectClarifyAnswers(const QVariantMap& toolMetadata, const AnswerDraft& draft) {
    const QVariantList questions = clarifyQuestions(toolMetadata);
    QVariantMap out;
    for (int qi = 0; qi < questions.size(); ++qi) {
        const QVariantMap q = questions.at(qi).toMap();
        const QString qid = clarifyQuestionId(q, qi);
        const QStringList choices = q.value(QStringLiteral("choices")).toStringList();
        const bool multi = q.value(QStringLiteral("multiSelect")).toBool();
        const QVariant af = q.value(QStringLiteral("allowFreeform"));
        const bool allowFreeform = af.isValid() ? af.toBool() : choices.isEmpty();

        const QStringList sel = draft.selected.value(qid);
        const QString typed = draft.freeform.value(qid).trimmed();

        if (multi) {
            QStringList list = sel;
            if (allowFreeform && !typed.isEmpty()) {
                list.push_back(typed);
            }
            if (!list.isEmpty()) {
                out.insert(qid, list);
            }
        } else {
            QString value = sel.isEmpty() ? QString() : sel.first();
            if (allowFreeform && !typed.isEmpty() && value.isEmpty()) {
                value = typed;
            }
            if (!value.isEmpty()) {
                out.insert(qid, value);
            }
        }
    }
    return out;
}

LayoutResult TranscriptLayout::build(const be::DocumentStore& doc, int width,
                                     const AnswerDraft& draft, int activeControl,
                                     const QHash<QString, int>& ratings) {
    LayoutResult result;
    QVector<RenderLine>& out = result.lines;
    QVector<Control>& controls = result.controls;
    BuildCtx ctx{qMax(20, width), draft, activeControl, 0};

    FlowState fs;
    const QVector<be::BlockRecord>& blocks = doc.blocks();
    result.blockFirstLine.fill(-1, blocks.size());
    // Tag every emitted row with its source block as we go (cheap parallel append).
    const auto tagLines = [&](int blockIdx) {
        while (result.lineBlock.size() < out.size()) {
            result.lineBlock.push_back(blockIdx);
        }
    };
    for (int bi = 0; bi < blocks.size(); ++bi) {
        const be::BlockRecord& b = blocks.at(bi);
        if (b.tombstoned) {
            continue;
        }

        fs.emitSeparators(result, doc, b, ctx.width);

        // First render row attributable to this block's body (after any header /
        // turn-gap emitted above) - the anchor a find match scrolls to.
        result.blockFirstLine[bi] = static_cast<int>(out.size());

        emitBlock(out, controls, b, ctx);

        tagLines(bi);
    }

    // Thumbs-feedback footer on the last block of a finished assistant message
    // (the TUI analog of the GUI AssistantFooter). Suppressed while an
    // interactive gate is pending (controls non-empty), since the gate owns the
    // keys. The selected glyph is driven by `ratings` so it survives a reload;
    // the 'u' / 'd' key hints are inline (no separate Control, so it never traps
    // the transcript in interactive mode).
    if (controls.isEmpty() && !blocks.isEmpty()) {
        int lastIdx = -1;
        for (int i = static_cast<int>(blocks.size()) - 1; i >= 0; --i) {
            if (!blocks.at(i).tombstoned) {
                lastIdx = i;
                break;
            }
        }
        if (lastIdx >= 0) {
            const be::BlockRecord& last = blocks.at(lastIdx);
            if (last.role == be::MessageRole::Assistant && !last.messageId.isEmpty()) {
                result.feedbackMessageId = last.messageId;
                const int rating = ratings.value(last.messageId, 0);
                Style up;
                up.fg = rating > 0 ? tpal::accent() : tpal::muted();
                if (rating > 0) {
                    up.attr |= ZTextAttribute::Bold;
                }
                Style down;
                down.fg = rating < 0 ? tpal::accent() : tpal::muted();
                if (rating < 0) {
                    down.attr |= ZTextAttribute::Bold;
                }
                out.push_back(RenderLine{
                    mkSpan(QStringLiteral("  "), Style{}),
                    mkSpan(QObject::tr("\u25b2 Good (u)"), up),
                    mkSpan(QStringLiteral("   "), Style{}),
                    mkSpan(QObject::tr("\u25bc Bad (d)"), down),
                });
                tagLines(lastIdx);
            }
        }
    }

    return result;
}
