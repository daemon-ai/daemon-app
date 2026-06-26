#include "tree_list_view.h"

#include "sidebar_model.h"

#include <Tui/ZEvent.h>

// Vendored same-rev private header: exposes ZListViewPrivate so we can read the
// sidebar list's private scroll offset (no public accessor exists).
#include <Tui/ZListView_p.h>

#include <QAbstractItemModel>

void TreeListView::keyEvent(Tui::ZKeyEvent* event)
{
    if (event->modifiers() == Qt::NoModifier) {
        if (event->key() == Qt::Key_Left) {
            emit collapseRequested();
            event->accept();
            return;
        }
        if (event->key() == Qt::Key_Right) {
            emit expandRequested();
            event->accept();
            return;
        }
    }
    Tui::ZListView::keyEvent(event);
}

int TreeListView::scrollOffset() const
{
    // ZListView paints model row (scrollPosition + i) at local y == i. The offset
    // lives in the private object with no public accessor; read it through the
    // vendored same-rev ZListViewPrivate (validated by the tui_magic in the base).
    const auto* priv = static_cast<const Tui::ZListViewPrivate*>(
        Tui::ZWidgetPrivate::get(this));
    if (priv == nullptr || priv->tui_magic != tui_magic_v0) {
        return 0;
    }
    return priv->scrollPosition;
}

void TreeListView::scrollByLines(int delta)
{
    QAbstractItemModel* m = model();
    if (m == nullptr) {
        return;
    }
    const int rows = m->rowCount();
    if (rows <= 0) {
        return;
    }
    const int cur = currentIndex().isValid() ? currentIndex().row() : 0;
    const int next = qBound(0, cur + delta, rows - 1);
    if (next != cur) {
        setCurrentIndex(m->index(next, 0));
    }
}

void TreeListView::clickAt(QPoint local)
{
    QAbstractItemModel* m = model();
    if (m == nullptr) {
        return;
    }
    const int rows = m->rowCount();
    if (rows <= 0) {
        return;
    }
    // ZListView paints model row (scrollPosition + i) at local y == i; add the
    // (otherwise-private) scroll offset so clicks map to the right row even when
    // the tree is taller than the viewport and scrolled.
    const int row = local.y() + scrollOffset();
    if (row < 0 || row >= rows) {
        return;
    }
    const QModelIndex idx = m->index(row, 0);

    // A collapsible section header (separator with children) folds/unfolds its whole
    // section. It is not selectable, so route it by explicit row rather than through
    // the selection-based collapse/expand requests.
    if (m->data(idx, SidebarModel::IsSeparatorRole).toBool()
        && m->data(idx, SidebarModel::HasChildrenRole).toBool()) {
        emit toggleRowRequested(row);
        return;
    }

    // Disclosure toggle: a click on a parent row's expand triangle expands or
    // collapses it (via the existing signals, which act on the model's current
    // row - so set the current index first). ZListView lays each row out as
    // "1 margin col + left-decoration + decoration-space + display string", and
    // the display string (DisplayRoleAdapter) begins with depth*2 indent spaces
    // then the triangle, so that is where the triangle sits.
    if (m->data(idx, SidebarModel::HasChildrenRole).toBool()) {
        const int depth = m->data(idx, SidebarModel::DepthRole).toInt();
        const bool hasDecoration = !m->data(idx, Tui::LeftDecorationRole).toString().isEmpty();
        const int decorationSpace = m->data(idx, Tui::LeftDecorationSpaceRole).toInt();
        const int triangleX = 1 + (hasDecoration ? 1 : 0) + decorationSpace + depth * 2;
        if (local.x() <= triangleX + 1) { // +1 tolerance: triangle + trailing space
            setCurrentIndex(idx);
            if (m->data(idx, SidebarModel::ExpandedRole).toBool()) {
                emit collapseRequested();
            } else {
                emit expandRequested();
            }
            return;
        }
    }
    setCurrentIndex(idx);
}
