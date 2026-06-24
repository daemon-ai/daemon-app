#include "code_editor_view.h"

#include "transcript_render.h" // Span / RenderLine
#include "tui_palette.h"

#include "app/code_editor_controller.h"
#include "app/line_model.h"

#include <Tui/ZColor.h>
#include <Tui/ZEvent.h>
#include <Tui/ZPainter.h>

#include <QHash>
#include <QModelIndex>
#include <QRect>

using editor::CodeEditorController;
using editor::LineModel;

namespace {

Tui::ZColor colorFromHex(const QString& hex, const Tui::ZColor& fallback)
{
    if (hex.size() != 7 || !hex.startsWith(QLatin1Char('#')))
        return fallback;
    bool ok = false;
    const int rgb = hex.mid(1).toInt(&ok, 16);
    return ok ? Tui::ZColor((rgb >> 16) & 0xff, (rgb >> 8) & 0xff, rgb & 0xff) : fallback;
}

RenderLine styledLine(const QString& text, const QVariantList& runs, const Tui::ZColor& defaultFg,
                      const Tui::ZColor& bg)
{
    RenderLine spans;
    int pos = 0;
    const auto push = [&](int start, int len, const Tui::ZColor& fg, Tui::ZTextAttributes attrs) {
        if (len <= 0)
            return;
        spans.push_back(Span { text.mid(start, len), fg, bg, attrs });
    };
    for (const QVariant& v : runs) {
        const QVariantMap m = v.toMap();
        const int textSize = static_cast<int>(text.size());
        const int start = qBound(0, m.value(QStringLiteral("start")).toInt(), textSize);
        const int length = qBound(0, m.value(QStringLiteral("length")).toInt(), textSize - start);
        if (start > pos)
            push(pos, start - pos, defaultFg, {});
        Tui::ZTextAttributes attrs {};
        if (m.value(QStringLiteral("bold")).toBool())
            attrs |= Tui::ZTextAttribute::Bold;
        if (m.value(QStringLiteral("italic")).toBool())
            attrs |= Tui::ZTextAttribute::Italic;
        push(start, length,
             colorFromHex(m.value(QStringLiteral("color")).toString(), defaultFg), attrs);
        pos = qMax(pos, start + length);
    }
    const int textSize = static_cast<int>(text.size());
    if (pos < textSize)
        push(pos, textSize - pos, defaultFg, {});
    if (spans.isEmpty() && text.isEmpty())
        spans.push_back(Span { QString(), defaultFg, bg, {} });
    return spans;
}

} // namespace

CodeEditorView::CodeEditorView(Tui::ZWidget* parent) : Tui::ZWidget(parent)
{
    setFocusPolicy(Tui::StrongFocus);
    setSizePolicyV(Tui::SizePolicy::Expanding);
}

void CodeEditorView::setController(CodeEditorController* controller)
{
    if (m_controller != nullptr)
        m_controller->disconnect(this);
    if (m_model != nullptr)
        m_model->disconnect(this);

    m_controller = controller;
    m_model = controller ? controller->lines() : nullptr;
    m_scrollTop = 0;
    m_hScroll = 0;

    if (m_model != nullptr) {
        const auto repaint = [this] { update(); };
        connect(m_model, &QAbstractItemModel::modelReset, this, repaint);
        connect(m_model, &QAbstractItemModel::dataChanged, this, repaint);
        connect(m_model, &QAbstractItemModel::rowsInserted, this, repaint);
        connect(m_model, &QAbstractItemModel::rowsRemoved, this, repaint);
    }
    if (m_controller != nullptr) {
        connect(m_controller, &CodeEditorController::documentChanged, this, [this] { update(); });
        connect(m_controller, &CodeEditorController::cursorChanged, this, [this] {
            ensureCaretVisible();
            update();
        });
    }
    update();
}

int CodeEditorView::gutterWidth() const
{
    const int lines = m_controller ? qMax(1, m_controller->lineCount()) : 1;
    int digits = 1;
    for (int v = lines; v >= 10; v /= 10)
        ++digits;
    return digits + 2; // number + a space on each side
}

int CodeEditorView::visibleRows() const
{
    return qMax(0, geometry().height());
}

void CodeEditorView::clampScroll()
{
    const int rows = m_model ? m_model->rowCount() : 0;
    const int maxTop = qMax(0, rows - visibleRows());
    m_scrollTop = qBound(0, m_scrollTop, maxTop);
    if (m_hScroll < 0)
        m_hScroll = 0;
}

void CodeEditorView::ensureCaretVisible()
{
    if (m_model == nullptr || m_controller == nullptr)
        return;
    const int row = m_model->rowForLine(m_controller->cursorLine());
    if (row >= 0) {
        const int h = visibleRows();
        if (row < m_scrollTop)
            m_scrollTop = row;
        else if (row >= m_scrollTop + h)
            m_scrollTop = row - h + 1;
    }
    const int col = m_controller->cursorColumn();
    const int textW = qMax(1, geometry().width() - gutterWidth());
    if (col < m_hScroll)
        m_hScroll = col;
    else if (col >= m_hScroll + textW)
        m_hScroll = col - textW + 1;
    clampScroll();
}

void CodeEditorView::scrollByLines(int delta)
{
    m_scrollTop += delta;
    clampScroll();
    update();
}

void CodeEditorView::clickAt(QPoint local, Qt::KeyboardModifiers modifiers)
{
    if (m_model == nullptr || m_controller == nullptr)
        return;
    const int row = m_scrollTop + local.y();
    if (row < 0 || row >= m_model->rowCount())
        return;
    const int realLine = m_model->realLine(row);
    if (realLine < 0)
        return;
    const int textX = local.x() - gutterWidth();
    const int col = m_hScroll + qMax(0, textX);
    const bool extend = (modifiers & Qt::ShiftModifier) != 0;
    m_controller->setCursor(realLine, col, extend);
    ensureCaretVisible();
    update();
}

void CodeEditorView::resizeEvent(Tui::ZResizeEvent* event)
{
    Tui::ZWidget::resizeEvent(event);
    clampScroll();
}

void CodeEditorView::paintEvent(Tui::ZPaintEvent* event)
{
    Tui::ZPainter* p = event->painter();
    const Tui::ZColor fg = tpal::fg();
    const Tui::ZColor bg = tpal::bg();
    const Tui::ZColor gutterFg = tpal::muted();
    const Tui::ZColor gutterBg = tpal::codeBg();
    p->clear(fg, bg);

    if (m_model == nullptr || m_controller == nullptr)
        return;

    const int h = geometry().height();
    const int w = geometry().width();
    const int gw = gutterWidth();
    const int textW = qMax(1, w - gw);
    const int caretRow = m_model->rowForLine(m_controller->cursorLine());
    const int caretCol = m_controller->cursorColumn();

    for (int rowY = 0; rowY < h; ++rowY) {
        const int row = m_scrollTop + rowY;
        if (row < 0 || row >= m_model->rowCount())
            continue;
        const QModelIndex idx = m_model->index(row, 0);
        const int realLine = m_model->data(idx, LineModel::RealLineRole).toInt();

        // Gutter: right-aligned 1-based line number.
        const QString num = QString::number(realLine + 1);
        p->clearRect(0, rowY, gw, 1, gutterFg, gutterBg);
        p->writeWithColors(qMax(0, gw - 1 - static_cast<int>(num.size())), rowY, num, gutterFg,
                           gutterBg);

        // Body: render the raw text with structured style runs from LineModel.
        const QString text = m_model->data(idx, LineModel::TextRole).toString();
        const QVariantList runs = m_model->data(idx, LineModel::StyleRunsRole).toList();
        const RenderLine spans = styledLine(text, runs, fg, bg);
        int col = 0; // logical column in the line
        for (const Span& s : spans) {
            for (int k = 0; k < s.text.size(); ++k, ++col) {
                const int sx = gw + (col - m_hScroll);
                if (sx < gw || sx >= w)
                    continue;
                const QString ch = s.text.mid(k, 1);
                if (s.attr != Tui::ZTextAttributes {})
                    p->writeWithAttributes(sx, rowY, ch, s.fg, bg, s.attr);
                else
                    p->writeWithColors(sx, rowY, ch, s.fg, bg);
            }
        }

        // Caret: invert the cell under the caret on its row (when focused).
        if (focus() && row == caretRow) {
            const int sx = gw + (caretCol - m_hScroll);
            if (sx >= gw && sx < w)
                p->writeWithColors(sx, rowY, QStringLiteral(" "), bg, fg);
        }
    }
    Q_UNUSED(textW);
}

void CodeEditorView::keyEvent(Tui::ZKeyEvent* event)
{
    if (m_controller == nullptr) {
        Tui::ZWidget::keyEvent(event);
        return;
    }
    const int key = event->key();
    const Qt::KeyboardModifiers mods = event->modifiers();
    const bool shift = (mods & Qt::ShiftModifier) != 0;
    const bool ctrl = (mods & Qt::ControlModifier) != 0;

    if (ctrl) {
        const QString t = event->text();
        if (t.compare(QStringLiteral("z"), Qt::CaseInsensitive) == 0) {
            m_controller->undo();
            event->accept();
            return;
        }
        if (t.compare(QStringLiteral("y"), Qt::CaseInsensitive) == 0) {
            m_controller->redo();
            event->accept();
            return;
        }
        if (t.compare(QStringLiteral("a"), Qt::CaseInsensitive) == 0) {
            m_controller->selectAll();
            event->accept();
            return;
        }
        if (t.compare(QStringLiteral("s"), Qt::CaseInsensitive) == 0) {
            emit saveRequested();
            event->accept();
            return;
        }
    }

    switch (key) {
    case Qt::Key_Left:
        m_controller->moveLeft(shift);
        event->accept();
        return;
    case Qt::Key_Right:
        m_controller->moveRight(shift);
        event->accept();
        return;
    case Qt::Key_Up:
        m_controller->moveUp(shift);
        event->accept();
        return;
    case Qt::Key_Down:
        m_controller->moveDown(shift);
        event->accept();
        return;
    case Qt::Key_Home:
        m_controller->moveHome(shift);
        event->accept();
        return;
    case Qt::Key_End:
        m_controller->moveEnd(shift);
        event->accept();
        return;
    case Qt::Key_Backspace:
        m_controller->removeBackward();
        event->accept();
        return;
    case Qt::Key_Delete:
        m_controller->removeForward();
        event->accept();
        return;
    case Qt::Key_Enter:
    case Qt::Key_Return:
        m_controller->insertNewline();
        event->accept();
        return;
    case Qt::Key_Tab:
        m_controller->insertText(QStringLiteral("    "));
        event->accept();
        return;
    default:
        break;
    }

    const QString text = event->text();
    if (!text.isEmpty() && text.at(0).isPrint()) {
        m_controller->insertText(text);
        event->accept();
        return;
    }
    Tui::ZWidget::keyEvent(event);
}

void CodeEditorView::focusInEvent(Tui::ZFocusEvent* event)
{
    Tui::ZWidget::focusInEvent(event);
    update();
}

void CodeEditorView::focusOutEvent(Tui::ZFocusEvent* event)
{
    Tui::ZWidget::focusOutEvent(event);
    update();
}
