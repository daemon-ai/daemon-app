#pragma once

#include <Tui/ZWidget.h>

#include <QElapsedTimer>
#include <QString>

namespace files {
class FsExplorerModel;
}

// Custom-painted file tree over the shared FsExplorerModel (the same model the GUI
// FileTree.qml renders). Flattens the visible (expanded) nodes into rows,
// indents by depth, paints expand chevrons + dimmed ignored entries, and lazily
// fetches a directory's children on expand. Activating a file emits fileChosen
// (pinned=true for Enter, false for a single click) so the shell can open it as
// an editor tab. The model lives in the shell; this view owns no fs state.
class FileTreeView : public Tui::ZWidget {
    Q_OBJECT

public:
    explicit FileTreeView(Tui::ZWidget* parent = nullptr);

    void setModel(files::FsExplorerModel* model);

    [[nodiscard]] int rowAt(int localY) const;
    void clickAt(QPoint local);
    void scrollByLines(int delta);

signals:
    // A file was activated. pinned=true for a deliberate open (Enter), false for a
    // transient single-click (preview tab).
    void fileChosen(const QString& rootId, const QString& path, bool pinned);

protected:
    void paintEvent(Tui::ZPaintEvent* event) override;
    void keyEvent(Tui::ZKeyEvent* event) override;
    void resizeEvent(Tui::ZResizeEvent* event) override;
    void focusInEvent(Tui::ZFocusEvent* event) override;
    void focusOutEvent(Tui::ZFocusEvent* event) override;

private:
    void rebuild();
    [[nodiscard]] int visibleRows() const;
    void clampScroll();
    void ensureVisible(int row);
    void toggleExpand(int row, bool expand);
    void activate(int row, bool pinned);

    files::FsExplorerModel* m_model = nullptr;
    int m_current = 0;
    int m_scrollTop = 0;
    int m_lastClickRow = -1;
    QElapsedTimer m_lastClick;
};
