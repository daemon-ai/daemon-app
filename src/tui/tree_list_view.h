// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QPoint>
#include <Tui/ZListView.h>

// ZListView handles Up/Down/Home/End but ignores Left/Right. The sidebar is a
// flattened tree, so map Left/Right to collapse/expand requests (mirroring the
// GUI's Keys.onLeft/RightPressed -> SidebarModel.collapse/expandCurrent).
class TreeListView : public Tui::ZListView {
    Q_OBJECT

public:
    using Tui::ZListView::ZListView;

    // Mouse: act on a click at widget-local point `local`. Selects the row under
    // the cursor (sets the current index); a click on a parent row's disclosure
    // triangle expands/collapses it instead (via the signals below).
    void clickAt(QPoint local);

    // Mouse wheel: move the selection by `delta` rows (negative = up). ZListView
    // scrolls to keep the current index visible, so moving the selection is the
    // public-API way to scroll a flattened tree.
    void scrollByLines(int delta);

signals:
    void collapseRequested();
    void expandRequested();
    // A collapsible section header was clicked: fold/unfold by explicit row (the
    // header is not selectable, so the selection-based collapse/expand requests
    // can't target it).
    void toggleRowRequested(int row);

protected:
    void keyEvent(Tui::ZKeyEvent* event) override;

private:
    // The first visible model row (ZListView's private scroll offset, read via
    // the vendored same-rev ZListViewPrivate). 0 when unavailable.
    [[nodiscard]] int scrollOffset() const;
};
