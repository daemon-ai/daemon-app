#include "queue_strip_view.h"

#include "tui_palette.h"

#include "composer_queue_model.h"
#include "composer_session_controller.h"

#include <Tui/ZColor.h>
#include <Tui/ZEvent.h>
#include <Tui/ZPainter.h>

#include <Tui/ZTerminal.h>

#include <QRect>
#include <QSize>

namespace {

Span mkSpan(const QString& text, const Tui::ZColor& fg, const Tui::ZColor& bg,
            Tui::ZTextAttributes attr = {})
{
    return Span { text, fg, bg, attr };
}

QString elide(const QString& text, int width)
{
    if (width <= 0) {
        return QString();
    }
    if (text.size() <= width) {
        return text;
    }
    return text.left(width - 1) + QStringLiteral("\u2026");
}

} // namespace

QueueStripView::QueueStripView(Tui::ZWidget* parent) : Tui::ZWidget(parent)
{
    setFocusPolicy(Tui::NoFocus); // gains focus only when it has entries
    setSizePolicyH(Tui::SizePolicy::Expanding);
    setSizePolicyV(Tui::SizePolicy::Fixed);
    setMaximumSize(Tui::tuiMaxSize, 0);
}

int QueueStripView::count() const
{
    return m_controller != nullptr ? m_controller->queueCount() : 0;
}

void QueueStripView::setController(ComposerSessionController* controller)
{
    m_controller = controller;
    if (m_controller != nullptr) {
        const auto repaint = [this] { rebuild(); };
        connect(m_controller, &ComposerSessionController::queueCountChanged, this, repaint);
        connect(m_controller, &ComposerSessionController::editingIndexChanged, this, repaint);
        if (ComposerQueueModel* q = m_controller->queue()) {
            connect(q, &QAbstractItemModel::modelReset, this, repaint);
            connect(q, &QAbstractItemModel::dataChanged, this, repaint);
            connect(q, &QAbstractItemModel::rowsInserted, this, repaint);
            connect(q, &QAbstractItemModel::rowsRemoved, this, repaint);
        }
    }
    rebuild();
}

QSize QueueStripView::sizeHint() const
{
    return { 40, m_height };
}

void QueueStripView::layoutLines()
{
    m_lines.clear();
    const int n = count();
    const int editing = m_controller != nullptr ? m_controller->editingIndex() : -1;
    m_sel = qBound(0, m_sel, qMax(0, n - 1));

    const int w = qMax(1, geometry().width());
    ComposerQueueModel* q = m_controller != nullptr ? m_controller->queue() : nullptr;
    for (int i = 0; i < n && q != nullptr; ++i) {
        const bool sel = i == m_sel;
        const bool isEditing = i == editing;
        const QString text = q->textAt(i);
        const Tui::ZColor bg = sel ? tpal::selectionBg() : tpal::bg();
        const Tui::ZColor idxFg = sel ? tpal::accent() : tpal::muted();

        RenderLine line;
        const QString marker = isEditing ? QStringLiteral("\u270e ")  // pencil while editing
                             : sel        ? QStringLiteral("\u25b8 ")  // arrow on selection
                                          : QStringLiteral("  ");
        line.push_back(mkSpan(marker, idxFg, bg));
        line.push_back(mkSpan(QString::number(i + 1) + QStringLiteral(". "), idxFg, bg));
        line.push_back(mkSpan(elide(text, qMax(1, w - 6)), tpal::fg(), bg,
                              sel ? Tui::ZTextAttribute::Bold : Tui::ZTextAttributes {}));
        m_lines.push_back(line);
    }
    update();
}

void QueueStripView::rebuild()
{
    const int n = count();
    m_height = qMin(n, kMaxRows);
    setFocusPolicy(n > 0 ? Tui::StrongFocus : Tui::NoFocus);
    // Changing our fixed height must re-flow the *parent* column: setMinimumSize
    // only requests layout for this widget's own children, so explicitly ask the
    // terminal to relayout the parent (which repositions/resizes us within it).
    // The follow-up resizeEvent re-runs layoutLines() with our real width, so the
    // entry text elides correctly; build once here for the no-resize case too.
    setMinimumSize(0, m_height);
    setMaximumSize(Tui::tuiMaxSize, m_height);
    if (Tui::ZWidget* par = parentWidget()) {
        if (Tui::ZTerminal* term = terminal()) {
            term->requestLayout(par);
        }
    }
    layoutLines();
}

void QueueStripView::clickAt(QPoint local)
{
    const int n = count();
    if (n <= 0) {
        return;
    }
    const int row = local.y();
    if (row < 0 || row >= qMin(n, kMaxRows)) {
        return; // below the last visible entry
    }
    if (row != m_sel) {
        m_sel = row;
        rebuild();
    }
}

void QueueStripView::resizeEvent(Tui::ZResizeEvent* event)
{
    Tui::ZWidget::resizeEvent(event);
    layoutLines();
}

void QueueStripView::paintEvent(Tui::ZPaintEvent* event)
{
    Tui::ZPainter* p = event->painter();
    p->clear(tpal::fg(), tpal::bg());

    const int h = geometry().height();
    const int w = geometry().width();
    for (int row = 0; row < h && row < static_cast<int>(m_lines.size()); ++row) {
        int x = 0;
        for (const Span& s : m_lines.at(row)) {
            if (x >= w) {
                break;
            }
            QString text = s.text;
            if (x + static_cast<int>(text.size()) > w) {
                text = text.left(w - x);
            }
            if (s.attr != Tui::ZTextAttributes {}) {
                p->writeWithAttributes(x, row, text, s.fg, s.bg, s.attr);
            } else {
                p->writeWithColors(x, row, text, s.fg, s.bg);
            }
            x += static_cast<int>(s.text.size());
        }
    }
}

void QueueStripView::keyEvent(Tui::ZKeyEvent* event)
{
    const int n = count();
    if (m_controller != nullptr && n > 0 && event->modifiers() == Qt::NoModifier) {
        const int key = event->key();
        bool handled = true;
        switch (key) {
        case Qt::Key_Up:
            m_sel = qMax(0, m_sel - 1);
            rebuild();
            break;
        case Qt::Key_Down:
            m_sel = qMin(n - 1, m_sel + 1);
            rebuild();
            break;
        case Qt::Key_Enter:
        case Qt::Key_Return:
            m_controller->sendNowEntry(m_sel);
            break;
        case Qt::Key_Delete:
        case Qt::Key_Backspace:
            m_controller->deleteEntry(m_sel);
            break;
        default:
            if (event->text() == QStringLiteral("s")) {
                m_controller->sendNowEntry(m_sel);
            } else if (event->text() == QStringLiteral("d")) {
                m_controller->deleteEntry(m_sel);
            } else if (event->text() == QStringLiteral("e")) {
                m_controller->beginEdit(m_sel);
                emit editRequested(m_sel);
            } else {
                handled = false;
            }
            break;
        }
        if (handled) {
            event->accept();
            return;
        }
    }
    Tui::ZWidget::keyEvent(event);
}
