#pragma once

#include "transcript_render.h" // Span / RenderLine

#include <Tui/ZWidget.h>

#include <QVector>

class ComposerSessionController;

// Custom-painted queued-prompt strip above the composer, bound to
// ComposerSessionController's queue (ComposerQueueModel) + queueCount +
// editingIndex. Mirrors the GUI QueuePanel: one row per queued entry (index +
// elided text, edit highlight on editingIndex). Keyboard routes the row actions
// straight to the shared controller: Up/Down select; Enter / 's' send-now; 'e'
// edit; 'd' / Delete remove. The strip auto-sizes to its content (0 height when
// empty, dropping out of the focus cycle).
class QueueStripView : public Tui::ZWidget {
    Q_OBJECT

public:
    explicit QueueStripView(Tui::ZWidget* parent = nullptr);

    void setController(ComposerSessionController* controller);

    [[nodiscard]] QSize sizeHint() const override;

signals:
    // beginEdit was invoked for `index`; the shell should focus the composer so
    // the loaded draft can be edited and saved on Enter.
    void editRequested(int index);

protected:
    void paintEvent(Tui::ZPaintEvent* event) override;
    void keyEvent(Tui::ZKeyEvent* event) override;
    void resizeEvent(Tui::ZResizeEvent* event) override;

private:
    void rebuild();      // height + parent relayout on data change
    void layoutLines();  // rebuild span lines for the current width
    [[nodiscard]] int count() const;

    ComposerSessionController* m_controller = nullptr;
    QVector<RenderLine> m_lines;
    int m_sel = 0;
    int m_height = 0;

    static constexpr int kMaxRows = 5;
};
