#include "transcript_view.h"

#include "tui_palette.h"

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
        m_lines = TranscriptLayout::build(*m_doc, width);
    } else {
        m_lines.clear();
    }
    if (m_stickToBottom) {
        scrollToBottom();
    } else {
        clampScrollTop();
    }
    update();
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
