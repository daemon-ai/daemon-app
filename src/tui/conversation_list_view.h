#pragma once

#include "transcript_render.h" // Span / RenderLine

#include <Tui/ZWidget.h>

#include <QVector>

class ConversationsListModel;

// Custom-painted conversation list replacing the stock single-line Tui::ZListView
// + DisplayRoleAdapter. Renders multi-line "cards" (title + timestamp / snippet /
// agent + tag chips) directly from ConversationsListModel, mirroring the GUI's
// ConversationCard.qml, with a selection wash, scrolling + right-edge scrollbar,
// and keyboard navigation. Selection lives in the shared model (identity-stable
// across scope/search rebuilds); the view paints CurrentRole as the highlight and
// emits rowActivated() so the shell can open the conversation.
class ConversationListView : public Tui::ZWidget {
    Q_OBJECT

public:
    explicit ConversationListView(Tui::ZWidget* parent = nullptr);

    void setModel(ConversationsListModel* model);

    // Re-bake the cached card spans from the current theme tokens (the spans
    // capture tpal::* colors at build time, so a theme change needs a rebuild, not
    // just a repaint). Selection + scroll position are preserved.
    void relayout();

signals:
    // Up/Down/Enter on a row: open that conversation (row index in the model).
    void rowActivated(int row);

    // Type-ahead: the list is the only focus stop in the column, so printable keys
    // build the search query (RootWidget routes these to the passive search box,
    // whose textChanged drives the live filter).
    void searchAppend(const QString& text); // a printable character was typed
    void searchBackspace();                 // delete the last query character
    void searchClear();                     // Esc with a non-empty query
    // Focus gained/lost, so the search box can show/hide its typing caret.
    void focusChanged(bool focused);

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

    ConversationsListModel* m_model = nullptr;
    QVector<RenderLine> m_lines;   // all rendered rows (cards stacked)
    QVector<int> m_rowOfLine;      // source row for each rendered line (-1 = gap)
    QVector<int> m_lineOfRow;      // first rendered line index for each source row
    int m_scrollTop = 0;
};
