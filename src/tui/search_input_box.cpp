#include "search_input_box.h"

#include "tui_palette.h"

#include <Tui/ZEvent.h>
#include <Tui/ZPainter.h>

#include <QRect>

void SearchInputBox::setTypingActive(bool active)
{
    if (m_typingActive == active) {
        return;
    }
    m_typingActive = active;
    update();
}

void SearchInputBox::paintEvent(Tui::ZPaintEvent* event)
{
    // Fully custom paint: this box never edits text itself (the session list
    // forwards keystrokes), so we draw a search affordance instead of the stock
    // line-edit chrome.
    Tui::ZPainter* p = event->painter();
    const Tui::ZColor bg = tpal::surfaceAlt();
    p->clear(tpal::fg(), bg);
    const int w = geometry().width();
    if (w <= 0) {
        return;
    }
    // Always-visible magnifier marks the row as a search field.
    int x = 0;
    p->writeWithColors(x, 0, QStringLiteral("⌕ "), tpal::muted(), bg);
    x += 2;

    const QString q = text();
    if (q.isEmpty()) {
        if (x < w) {
            p->writeWithColors(x, 0, QStringLiteral("Search sessions").left(w - x),
                               tpal::muted(), bg);
        }
        return;
    }
    // Show the query (its tail if it overflows) and, while the list is focused, an
    // accent bar caret after it to mark where typing lands.
    const int caretReserve = m_typingActive ? 1 : 0;
    const int budget = qMax(0, w - x - caretReserve);
    const QString shown = q.size() > budget ? q.right(budget) : q;
    p->writeWithColors(x, 0, shown, tpal::fg(), bg);
    x += static_cast<int>(shown.size());
    if (m_typingActive && x < w) {
        p->writeWithColors(x, 0, QStringLiteral("▏"), tpal::accent(), bg);
    }
}

void TranscriptSearchBox::keyEvent(Tui::ZKeyEvent* event)
{
    const int key = event->key();
    const bool shift = (event->modifiers() & Qt::ShiftModifier) != 0;
    if (key == Qt::Key_Escape) {
        emit closeRequested();
        event->accept();
        return;
    }
    if (key == Qt::Key_Enter || key == Qt::Key_Return
        || event->text() == QStringLiteral("\r")) {
        if (shift) {
            emit previousRequested();
        } else {
            emit nextRequested();
        }
        event->accept();
        return;
    }
    if (key == Qt::Key_Down) {
        emit nextRequested();
        event->accept();
        return;
    }
    if (key == Qt::Key_Up) {
        emit previousRequested();
        event->accept();
        return;
    }
    Tui::ZInputBox::keyEvent(event);
}
