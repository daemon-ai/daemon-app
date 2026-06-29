// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QVector>
#include <Tui/ZWidget.h>

class TabModel;

// The TUI pane tab strip: a one-line, custom-painted row of tab chips bound to
// the shared TabModel (the same model the QML TabBar binds). It renders each open
// tab as " title x ", highlighting the active one, with a trailing " + " add
// affordance. It scrolls horizontally to keep the active tab visible when the
// chips overflow the width. Clicking a chip activates it (or closes it when the
// click lands on the chip's "x"); clicking "+" requests a new tab. The strip acts
// on the model directly for activate/close and surfaces "+" via newTabRequested.
class TabStripView : public Tui::ZWidget {
    Q_OBJECT

public:
    explicit TabStripView(Tui::ZWidget* parent = nullptr);

    void setModel(TabModel* model);

    [[nodiscard]] QSize sizeHint() const override;

    // Mouse: activate / close / new-tab depending on what the local x hits.
    void clickAt(QPoint local);

signals:
    // The trailing "+" affordance was clicked.
    void newTabRequested();

protected:
    void paintEvent(Tui::ZPaintEvent* event) override;
    void resizeEvent(Tui::ZResizeEvent* event) override;
    // The strip joins the Tab focus cycle: Left/Right move between tabs, Enter
    // pins a preview tab, Delete/Backspace closes the current tab. Plain Tab is
    // left unhandled so it bubbles to the focus container for pane cycling.
    void keyEvent(Tui::ZKeyEvent* event) override;
    void focusInEvent(Tui::ZFocusEvent* event) override;
    void focusOutEvent(Tui::ZFocusEvent* event) override;

private:
    struct Segment {
        int index = -1;  // model row, or -1 for the "+" affordance
        int x0 = 0;      // inclusive widget-local start column
        int x1 = 0;      // exclusive end column
        int closeX = -1; // column of the "x" (close) glyph, -1 when not closable
        bool plus = false;
    };

    // Rebuild m_segments for the current width, scrolling m_firstVisible so the
    // active tab stays on screen. Used by both paint and hit-testing.
    void layout();

    TabModel* m_model = nullptr;
    QVector<Segment> m_segments;
    int m_firstVisible = 0; // first model row painted (horizontal scroll anchor)
};
