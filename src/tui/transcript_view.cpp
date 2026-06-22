#include "transcript_view.h"

#include "tui_palette.h"

#include "core/block_record.h"
#include "core/document_store.h"

#include <Tui/ZColor.h>
#include <Tui/ZEvent.h>
#include <Tui/ZPainter.h>

#include <QRect>

TranscriptView::TranscriptView(Tui::ZWidget *parent) : Tui::ZWidget(parent)
{
    // Focusable so it joins the Tab cycle and can take scroll keys, but it never
    // consumes Esc (see keyEvent) so the context-sensitive quit chain still works.
    setFocusPolicy(Tui::StrongFocus);
    setSizePolicyV(Tui::SizePolicy::Expanding);
}

void TranscriptView::setDocument(const be::DocumentStore *doc)
{
    m_doc = doc;
    m_scrollTop = 0;
    m_stickToBottom = true;
    rebuild();
}

void TranscriptView::reload()
{
    rebuild();
}

void TranscriptView::scrollByLines(int delta)
{
    m_scrollTop += delta;
    clampScrollTop();
    // Pin to the bottom only when the scroll lands all the way down.
    m_stickToBottom = atBottom();
    update();
}

int TranscriptView::visibleRows() const
{
    return qMax(0, geometry().height());
}

int TranscriptView::maxScrollTop() const
{
    return qMax(0, static_cast<int>(m_lines.size()) - visibleRows());
}

void TranscriptView::clampScrollTop()
{
    m_scrollTop = qBound(0, m_scrollTop, maxScrollTop());
}

void TranscriptView::scrollToBottom()
{
    m_scrollTop = maxScrollTop();
}

bool TranscriptView::atBottom() const
{
    return m_scrollTop >= maxScrollTop();
}

void TranscriptView::rebuild()
{
    const int width = qMax(1, geometry().width());
    if (m_doc != nullptr) {
        const LayoutResult res = TranscriptLayout::build(*m_doc, width, m_draft, m_activeControl);
        m_lines = res.lines;
        m_controls = res.controls;
    } else {
        m_lines.clear();
        m_controls.clear();
    }

    if (interactive()) {
        // An awaiting block exists: focus its first control so the form is
        // immediately answerable, and keep the focused control on screen.
        if (m_activeControl < 0 || m_activeControl >= m_controls.size()) {
            m_activeControl = 0;
        }
        // Re-build once more so the freshly-focused control paints its wash.
        const LayoutResult res = TranscriptLayout::build(*m_doc, width, m_draft, m_activeControl);
        m_lines = res.lines;
        m_controls = res.controls;
        ensureControlVisible();
    } else {
        m_activeControl = -1;
        if (m_stickToBottom) {
            scrollToBottom();
        } else {
            clampScrollTop();
        }
    }
    update();
}

void TranscriptView::clickAt(QPoint local)
{
    if (!interactive()) {
        return; // a plain transcript click only focuses (handled by the shell)
    }
    const int absLine = m_scrollTop + local.y();
    // Controls sitting on the clicked rendered line, in paint (left-to-right) order.
    QVector<int> onLine;
    for (int i = 0; i < m_controls.size(); ++i) {
        if (m_controls.at(i).line == absLine) {
            onLine.push_back(i);
        }
    }
    if (onLine.isEmpty()) {
        return;
    }
    // Approval bars paint several buttons on one row; Control carries no x range,
    // so pick by an even left-to-right partition of the width (single-control rows
    // collapse to that one control).
    int pick = 0;
    if (onLine.size() > 1) {
        const int w = qMax(1, geometry().width());
        pick = qBound(0, local.x() * static_cast<int>(onLine.size()) / w,
                      static_cast<int>(onLine.size()) - 1);
    }
    m_activeControl = onLine.at(pick);
    if (m_controls.at(m_activeControl).kind == Control::Kind::Freeform) {
        rebuild(); // just focus the field; typing fills it
        return;
    }
    activateControl(); // button decides / choice toggles / submit fires
}

void TranscriptView::moveControl(int delta)
{
    if (!interactive()) {
        return;
    }
    const int n = static_cast<int>(m_controls.size());
    m_activeControl = ((m_activeControl + delta) % n + n) % n;
    rebuild();
}

void TranscriptView::ensureControlVisible()
{
    if (m_activeControl < 0 || m_activeControl >= m_controls.size()) {
        return;
    }
    const int line = m_controls.at(m_activeControl).line;
    const int rows = visibleRows();
    if (line < m_scrollTop) {
        m_scrollTop = line;
    } else if (rows > 0 && line >= m_scrollTop + rows) {
        m_scrollTop = line - rows + 1;
    }
    clampScrollTop();
}

QVariantMap TranscriptView::toolMetadataForCallId(const QString &callId) const
{
    if (m_doc == nullptr) {
        return {};
    }
    const QVector<be::BlockRecord> &blocks = m_doc->blocks();
    for (const be::BlockRecord &b : blocks) {
        if (b.type == be::BlockType::ToolCall
            && b.metadata.value(QStringLiteral("callId")).toString() == callId) {
            return b.metadata;
        }
    }
    return {};
}

void TranscriptView::toggleChoice()
{
    if (m_activeControl < 0 || m_activeControl >= m_controls.size()) {
        return;
    }
    const Control &c = m_controls.at(m_activeControl);
    if (c.kind != Control::Kind::Choice) {
        return;
    }
    QStringList sel = m_draft.selected.value(c.questionId);
    if (c.multi) {
        if (sel.contains(c.choiceLabel)) {
            sel.removeAll(c.choiceLabel);
        } else {
            sel.push_back(c.choiceLabel);
        }
    } else {
        // Radio: re-selecting the active choice clears it.
        sel = sel.contains(c.choiceLabel) ? QStringList {} : QStringList { c.choiceLabel };
    }
    m_draft.selected.insert(c.questionId, sel);
    rebuild();
}

void TranscriptView::activateControl()
{
    if (m_activeControl < 0 || m_activeControl >= m_controls.size()) {
        return;
    }
    const Control c = m_controls.at(m_activeControl);
    switch (c.kind) {
    case Control::Kind::Approve:
        emit approvalDecided(c.callId, QStringLiteral("approved"), false);
        break;
    case Control::Kind::Deny:
        emit approvalDecided(c.callId, QStringLiteral("denied"), false);
        break;
    case Control::Kind::Permanent:
        emit approvalDecided(c.callId, QStringLiteral("approved"), true);
        break;
    case Control::Kind::Choice:
        toggleChoice();
        break;
    case Control::Kind::Freeform:
    case Control::Kind::Submit: {
        const QVariantMap meta = toolMetadataForCallId(c.callId);
        const QVariantMap answers = collectClarifyAnswers(meta, m_draft);
        if (!answers.isEmpty()) {
            emit clarifySubmitted(c.callId, c.requestId, answers);
        }
        break;
    }
    }
}

void TranscriptView::resizeEvent(Tui::ZResizeEvent *event)
{
    Tui::ZWidget::resizeEvent(event);
    // Width affects wrapping, height affects the bottom pin; rebuild either way.
    rebuild();
}

void TranscriptView::paintEvent(Tui::ZPaintEvent *event)
{
    Tui::ZPainter *p = event->painter();
    const Tui::ZColor pageFg = tpal::fg();
    const Tui::ZColor pageBg = tpal::bg();
    p->clear(pageFg, pageBg);

    const int h = geometry().height();
    const int w = geometry().width();
    const int lineCount = static_cast<int>(m_lines.size());
    const bool scrollable = lineCount > h;
    // Reserve the last column for the scroll indicator when it is shown.
    const int contentW = scrollable ? qMax(1, w - 1) : w;

    for (int row = 0; row < h; ++row) {
        const int idx = m_scrollTop + row;
        if (idx < 0 || idx >= lineCount) {
            continue;
        }
        int x = 0;
        for (const Span &s : m_lines.at(idx)) {
            if (x >= contentW) {
                break;
            }
            const int len = static_cast<int>(s.text.size());
            QString text = s.text;
            if (x + len > contentW) {
                text = text.left(contentW - x);
            }
            if (!text.isEmpty()) {
                if (s.attr != Tui::ZTextAttributes {}) {
                    p->writeWithAttributes(x, row, text, s.fg, s.bg, s.attr);
                } else {
                    p->writeWithColors(x, row, text, s.fg, s.bg);
                }
            }
            x += len;
        }
    }

    if (scrollable && h > 0) {
        const int total = lineCount;
        int thumbLen = qMax(1, (h * h) / total);
        int thumbTop = (m_scrollTop * h) / total;
        thumbTop = qBound(0, thumbTop, h - thumbLen);
        const int xCol = w - 1;
        for (int row = 0; row < h; ++row) {
            const bool onThumb = row >= thumbTop && row < thumbTop + thumbLen;
            p->writeWithColors(xCol, row,
                               onThumb ? QStringLiteral("\u2503") : QStringLiteral("\u2502"),
                               onThumb ? tpal::accent() : tpal::muted(), pageBg);
        }
    }
}

void TranscriptView::keyEvent(Tui::ZKeyEvent *event)
{
    // Interactive mode: while an awaiting-approval / unanswered-clarify block is
    // present its controls own the cursor. Up/Down/Tab walk the controls, Space
    // toggles a choice, Enter activates a button / submits, and a freeform field
    // takes typed text + Backspace. PageUp/Down/Home/End still scroll; Esc still
    // bubbles (the context-sensitive quit chain).
    if (interactive()) {
        const int key = event->key();
        const Qt::KeyboardModifiers mods = event->modifiers();
        const Control &active = m_controls.at(qBound(0, m_activeControl, m_controls.size() - 1));
        const bool onFreeform = active.kind == Control::Kind::Freeform;

        if (mods == Qt::NoModifier || mods == Qt::ShiftModifier) {
            if (key == Qt::Key_Up || (key == Qt::Key_Tab && mods == Qt::ShiftModifier)
                || key == Qt::Key_Backtab) {
                moveControl(-1);
                event->accept();
                return;
            }
            if (key == Qt::Key_Down || (key == Qt::Key_Tab && mods == Qt::NoModifier)) {
                moveControl(1);
                event->accept();
                return;
            }
            if (key == Qt::Key_Enter || key == Qt::Key_Return) {
                activateControl();
                event->accept();
                return;
            }
            if (key == Qt::Key_Space && !onFreeform) {
                toggleChoice();
                event->accept();
                return;
            }
        }
        if (onFreeform && mods == Qt::NoModifier) {
            if (key == Qt::Key_Backspace) {
                QString text = m_draft.freeform.value(active.questionId);
                text.chop(1);
                m_draft.freeform.insert(active.questionId, text);
                rebuild();
                event->accept();
                return;
            }
        }
        if (onFreeform && !event->text().isEmpty()
            && (mods == Qt::NoModifier || mods == Qt::ShiftModifier)) {
            const QString t = event->text();
            // Ignore control chars (Enter handled above).
            if (t.at(0).isPrint()) {
                m_draft.freeform.insert(active.questionId,
                                        m_draft.freeform.value(active.questionId) + t);
                rebuild();
                event->accept();
                return;
            }
        }
        // Fall through to the scroll keys below (PageUp/Down/Home/End) and Esc.
    }

    if (event->modifiers() == Qt::NoModifier) {
        const int key = event->key();
        const int page = qMax(1, visibleRows() - 1);
        bool handled = true;
        switch (key) {
        case Qt::Key_Up:
            m_scrollTop -= 1;
            break;
        case Qt::Key_Down:
            m_scrollTop += 1;
            break;
        case Qt::Key_PageUp:
            m_scrollTop -= page;
            break;
        case Qt::Key_PageDown:
            m_scrollTop += page;
            break;
        case Qt::Key_Home:
            m_scrollTop = 0;
            break;
        case Qt::Key_End:
            m_scrollTop = maxScrollTop();
            break;
        default:
            handled = false;
            break;
        }
        if (handled) {
            clampScrollTop();
            // Pin to the bottom only when the user scrolled all the way down.
            m_stickToBottom = atBottom();
            update();
            event->accept();
            return;
        }
    }
    // Everything else (notably Esc) bubbles up unhandled.
    Tui::ZWidget::keyEvent(event);
}
