#include "file_tree_view.h"

#include "fs_explorer_model.h"
#include "tui_palette.h"

#include <algorithm>
#include <QPoint>
#include <QRect>
#include <Tui/ZColor.h>
#include <Tui/ZEvent.h>
#include <Tui/ZPainter.h>

using files::FsExplorerModel;

FileTreeView::FileTreeView(Tui::ZWidget* parent) : Tui::ZWidget(parent) {
    setFocusPolicy(Tui::StrongFocus);
    setSizePolicyV(Tui::SizePolicy::Expanding);
}

void FileTreeView::setModel(FsExplorerModel* model) {
    m_model = model;
    if (m_model != nullptr) {
        const auto rebuildSlot = [this] { rebuild(); };
        connect(m_model, &QAbstractItemModel::modelReset, this, rebuildSlot);
        connect(m_model, &QAbstractItemModel::rowsInserted, this, rebuildSlot);
        connect(m_model, &QAbstractItemModel::rowsRemoved, this, rebuildSlot);
        connect(m_model, &QAbstractItemModel::dataChanged, this, [this] { update(); });
        connect(m_model, &QAbstractItemModel::layoutChanged, this, rebuildSlot);
    }
    rebuild();
}

void FileTreeView::rebuild() {
    const int rows = m_model ? m_model->rowCount() : 0;
    if (m_current >= rows)
        m_current = rows - 1;
    m_current = std::max(m_current, 0);
    clampScroll();
    update();
}

int FileTreeView::visibleRows() const {
    return qMax(0, geometry().height());
}

void FileTreeView::clampScroll() {
    const int rows = m_model ? m_model->rowCount() : 0;
    const int maxTop = qMax(0, rows - visibleRows());
    m_scrollTop = qBound(0, m_scrollTop, maxTop);
}

void FileTreeView::ensureVisible(int row) {
    if (row < 0)
        return;
    const int h = visibleRows();
    if (row < m_scrollTop)
        m_scrollTop = row;
    else if (row >= m_scrollTop + h)
        m_scrollTop = row - h + 1;
    clampScroll();
}

int FileTreeView::rowAt(int localY) const {
    const int idx = m_scrollTop + localY;
    const int rows = m_model ? m_model->rowCount() : 0;
    return (idx >= 0 && idx < rows) ? idx : -1;
}

void FileTreeView::toggleExpand(int row, bool expand) {
    if (m_model == nullptr || row < 0 || row >= m_model->rowCount())
        return;
    if (!m_model->data(m_model->index(row, 0), FsExplorerModel::IsDirRole).toBool())
        return;
    const bool expanded =
        m_model->data(m_model->index(row, 0), FsExplorerModel::ExpandedRole).toBool();
    if (expanded != expand)
        m_model->toggleExpand(row);
}

void FileTreeView::activate(int row, bool pinned) {
    if (m_model == nullptr || row < 0 || row >= m_model->rowCount())
        return;
    const QModelIndex idx = m_model->index(row, 0);
    if (m_model->data(idx, FsExplorerModel::IsDirRole).toBool()) {
        m_model->toggleExpand(row);
        return;
    }
    emit fileChosen(m_model->data(idx, FsExplorerModel::RootIdRole).toString(),
                    m_model->data(idx, FsExplorerModel::PathRole).toString(), pinned);
}

void FileTreeView::clickAt(QPoint local) {
    const int row = rowAt(local.y());
    if (row < 0)
        return;
    const bool sameRow =
        row == m_lastClickRow && m_lastClick.isValid() && m_lastClick.elapsed() < 450;
    const bool isDir = m_model != nullptr &&
                       m_model->data(m_model->index(row, 0), FsExplorerModel::IsDirRole).toBool();
    m_current = row;
    activate(row, sameRow && !isDir); // second click on same file = pinned open
    m_lastClickRow = row;
    m_lastClick.restart();
    update();
}

void FileTreeView::scrollByLines(int delta) {
    m_scrollTop += delta;
    clampScroll();
    update();
}

void FileTreeView::resizeEvent(Tui::ZResizeEvent* event) {
    Tui::ZWidget::resizeEvent(event);
    clampScroll();
}

void FileTreeView::paintEvent(Tui::ZPaintEvent* event) {
    Tui::ZPainter* p = event->painter();
    const Tui::ZColor fg = tpal::fg();
    const Tui::ZColor bg = tpal::bg();
    p->clear(fg, bg);

    const int h = geometry().height();
    const int w = geometry().width();

    for (int rowY = 0; rowY < h; ++rowY) {
        const int idx = m_scrollTop + rowY;
        if (m_model == nullptr || idx < 0 || idx >= m_model->rowCount())
            continue;
        const QModelIndex modelIndex = m_model->index(idx, 0);
        const bool isDir = m_model->data(modelIndex, FsExplorerModel::IsDirRole).toBool();
        const bool ignored = m_model->data(modelIndex, FsExplorerModel::IgnoredRole).toBool();
        const bool expanded = m_model->data(modelIndex, FsExplorerModel::ExpandedRole).toBool();
        const int depth = m_model->data(modelIndex, FsExplorerModel::DepthRole).toInt();
        const QString name = m_model->data(modelIndex, FsExplorerModel::LabelRole).toString();
        const bool current = idx == m_current;

        const Tui::ZColor rowBg =
            current ? (focus() ? tpal::selectionBg() : tpal::selectionInactiveBg()) : bg;
        if (current)
            p->clearRect(0, rowY, w, 1, fg, rowBg);

        const QString indent(static_cast<qsizetype>(depth) * 2, QLatin1Char(' '));
        QString chevron;
        if (isDir)
            chevron = expanded ? QStringLiteral("\u25be ") : QStringLiteral("\u25b8 ");
        else
            chevron = QStringLiteral("  ");

        const Tui::ZColor nameFg = ignored ? tpal::muted() : (isDir ? tpal::accent() : fg);
        const QString text = indent + chevron + name;
        p->writeWithColors(0, rowY, text.left(qMax(0, w)), nameFg, rowBg);
    }
}

void FileTreeView::keyEvent(Tui::ZKeyEvent* event) {
    if (m_model == nullptr || m_model->rowCount() == 0) {
        Tui::ZWidget::keyEvent(event);
        return;
    }
    const int key = event->key();
    const Qt::KeyboardModifiers mods = event->modifiers();

    if (mods == Qt::NoModifier) {
        switch (key) {
        case Qt::Key_Up:
            m_current = qMax(0, m_current - 1);
            ensureVisible(m_current);
            update();
            event->accept();
            return;
        case Qt::Key_Down:
            m_current = qMin(m_model->rowCount() - 1, m_current + 1);
            ensureVisible(m_current);
            update();
            event->accept();
            return;
        case Qt::Key_Home:
            m_current = 0;
            ensureVisible(m_current);
            update();
            event->accept();
            return;
        case Qt::Key_End:
            m_current = m_model->rowCount() - 1;
            ensureVisible(m_current);
            update();
            event->accept();
            return;
        case Qt::Key_Right:
            toggleExpand(m_current, true);
            event->accept();
            return;
        case Qt::Key_Left:
            toggleExpand(m_current, false);
            event->accept();
            return;
        case Qt::Key_Enter:
        case Qt::Key_Return:
            activate(m_current, true); // Enter = pinned open
            event->accept();
            return;
        case Qt::Key_Space:
            activate(m_current, false);
            event->accept();
            return;
        default:
            break;
        }
    }
    Tui::ZWidget::keyEvent(event);
}

void FileTreeView::focusInEvent(Tui::ZFocusEvent* event) {
    Tui::ZWidget::focusInEvent(event);
    update();
}

void FileTreeView::focusOutEvent(Tui::ZFocusEvent* event) {
    Tui::ZWidget::focusOutEvent(event);
    update();
}
