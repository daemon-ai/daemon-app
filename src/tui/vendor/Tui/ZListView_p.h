// SPDX-License-Identifier: BSL-1.0

// Vendored verbatim from Tui Widgets at the flake-pinned revision
// 31ccc7af4ff47ec209e8e7b3508988fec5d2e2e9 (src/Tui/ZListView_p.h). Tui Widgets
// does not install its private headers, and ZListView keeps its scroll offset
// (scrollPosition) private with no public accessor. The TUI reads it to map a
// mouse click's y to the correct model row when the sidebar tree is scrolled.
// Keep in lockstep with the tuiwidgets rev pinned in flake.nix.

#ifndef TUIWIDGETS_ZLISTVIEW_P_INCLUDED
#define TUIWIDGETS_ZLISTVIEW_P_INCLUDED

#include <Tui/ZListView.h>

#include <QPointer>

#include <Tui/ZStyledTextLine.h>
#include <Tui/ZWidget_p.h>

#include <Tui/tuiwidgets_internal.h>

TUIWIDGETS_NS_START

class ZListViewPrivate : public ZWidgetPrivate {
public:
    ZListViewPrivate(ZWidget *pub);
    ~ZListViewPrivate() override;

public:
    ZStyledTextLine styledText;
    QAbstractItemModel *model = nullptr;
    QPointer<QItemSelectionModel> selectionModel;
    int lastSelectedRow = 0;
    int scrollPosition = 0;
    QAbstractItemModel *allocatedModel = nullptr;

    TUIWIDGETS_DECLARE_PUBLIC(ZListView)
};

TUIWIDGETS_NS_END

#endif // TUIWIDGETS_ZLISTVIEW_P_INCLUDED
