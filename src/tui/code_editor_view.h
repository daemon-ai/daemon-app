#pragma once

#include <Tui/ZWidget.h>

#include <QString>

namespace editor {
class CodeEditorController;
class LineModel;
}

// Custom-painted code editor over the shared de_core engine: it binds a
// CodeEditorController (which owns the TextDocument + incremental
// KSyntaxHighlighting + LineModel) and renders the LineModel's raw text plus
// structured style runs as colored terminal spans. Editing is routed through the
// controller (the document is the source of truth). Monospace layout, gutter line
// numbers, an inverted-cell caret, and vertical + horizontal scroll to keep the
// caret visible.
class CodeEditorView : public Tui::ZWidget {
    Q_OBJECT

public:
    explicit CodeEditorView(Tui::ZWidget* parent = nullptr);

    void setController(editor::CodeEditorController* controller);
    [[nodiscard]] editor::CodeEditorController* controller() const { return m_controller; }

    void scrollByLines(int delta);
    void clickAt(QPoint local, Qt::KeyboardModifiers modifiers);

signals:
    void saveRequested();

protected:
    void paintEvent(Tui::ZPaintEvent* event) override;
    void keyEvent(Tui::ZKeyEvent* event) override;
    void resizeEvent(Tui::ZResizeEvent* event) override;
    void focusInEvent(Tui::ZFocusEvent* event) override;
    void focusOutEvent(Tui::ZFocusEvent* event) override;

private:
    [[nodiscard]] int gutterWidth() const;
    [[nodiscard]] int visibleRows() const;
    void clampScroll();
    void ensureCaretVisible();

    editor::CodeEditorController* m_controller = nullptr;
    editor::LineModel* m_model = nullptr;
    int m_scrollTop = 0;
    int m_hScroll = 0;
};
