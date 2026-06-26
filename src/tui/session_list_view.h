#pragma once

#include "transcript_render.h" // Span / RenderLine

#include <QVector>
#include <Tui/ZWidget.h>

class SessionsListModel;

// Custom-painted session list replacing the stock single-line Tui::ZListView
// + DisplayRoleAdapter. Renders multi-line "cards" (title + timestamp / snippet /
// agent + tag chips) directly from SessionsListModel, mirroring the GUI's
// SessionCard.qml, with a selection wash, scrolling + right-edge scrollbar,
// and keyboard navigation. Selection lives in the shared model (identity-stable
// across scope/search rebuilds); the view paints CurrentRole as the highlight and
// emits rowActivated() so the shell can open the session.
class SessionListView : public Tui::ZWidget {
    Q_OBJECT

public:
    explicit SessionListView(Tui::ZWidget* parent = nullptr);

    void setModel(SessionsListModel* model);

    // Re-bake the cached card spans from the current theme tokens (the spans
    // capture tpal::* colors at build time, so a theme change needs a rebuild, not
    // just a repaint). Selection + scroll position are preserved.
    void relayout();

    // Mouse helpers. rowAt() maps a widget-local y to the model row whose card
    // covers it (-1 for the gap between cards or out of range). activateAtLocalY()
    // opens that row (same as Enter). scrollByLines() nudges the scroll offset.
    [[nodiscard]] int rowAt(int localY) const;
    void activateAtLocalY(int localY);
    void scrollByLines(int delta);

signals:
    // A row was activated. `pinned` is true for a deliberate open (Enter) that
    // should create a permanent tab, false for a transient open (arrow navigation
    // or a single mouse click) that loads into the VSCode-style preview tab.
    void rowActivated(int row, bool pinned);

    // Type-ahead: the list is the only focus stop in the column, so printable keys
    // build the search query (RootWidget routes these to the passive search box,
    // whose textChanged drives the live filter).
    void searchAppend(const QString& text); // a printable character was typed
    void searchBackspace();                 // delete the last query character
    void searchClear();                     // Esc with a non-empty query
    // Focus gained/lost, so the search box can show/hide its typing caret.
    void focusChanged(bool focused);

    // Session actions on the current row (the shell resolves the session id
    // and runs the store mutation / dialog). Ctrl+R rename, Ctrl+E export, Ctrl+K
    // pin-toggle, Delete delete.
    void renameRequested(int row);
    void exportRequested(int row);
    void pinToggleRequested(int row);
    void deleteRequested(int row);
    // Reorder the row within the list (Alt+Up = -1, Alt+Down = +1); the shell
    // resolves the session id and calls ISessionStore::moveSession.
    void moveRequested(int row, int delta);

protected:
    void paintEvent(Tui::ZPaintEvent* event) override;
    void keyEvent(Tui::ZKeyEvent* event) override;
    void resizeEvent(Tui::ZResizeEvent* event) override;
    void focusInEvent(Tui::ZFocusEvent* event) override;
    void focusOutEvent(Tui::ZFocusEvent* event) override;

private:
    void rebuild();
    void ensureVisible(int row);
    [[nodiscard]] int visibleRows() const;
    [[nodiscard]] int maxScrollTop() const;
    void clampScrollTop();

    SessionsListModel* m_model = nullptr;
    QVector<RenderLine> m_lines; // all rendered rows (cards stacked)
    QVector<int> m_rowOfLine;    // source row for each rendered line (-1 = gap)
    QVector<int> m_lineOfRow;    // first rendered line index for each source row
    int m_scrollTop = 0;
};
