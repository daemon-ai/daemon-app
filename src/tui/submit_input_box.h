// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "line_editor.h"

#include <Tui/ZTextEdit.h>
#include <Tui/ZTextMetrics.h>

class ComposerSessionController;

// The composer input: a multiline editor (ZTextEdit) matching the GUI's TextArea.
// Plain Enter submits, Shift+Enter inserts a newline, Ctrl+Enter steers; it grows
// with content up to a row cap, supports readline-style editing (executed against
// the document cursor via the shared lineedit keymap), and drives the shared
// session's draft + completion + reverse-search state.
class SubmitInputBox : public Tui::ZTextEdit {
    Q_OBJECT

public:
    explicit SubmitInputBox(const Tui::ZTextMetrics& metrics, Tui::ZWidget* parent = nullptr);

    // The shared composer session. When set, the input box routes completion
    // navigation (Up/Down/Enter/Tab/Esc while completionActive) to it and refreshes
    // the slash/@ trigger after every key (text/caret change).
    void setSession(ComposerSessionController* session) { m_session = session; }

    // Flat offset into text() (counting embedded '\n') <-> document Position, so the
    // completion FSM and the controller's cursorRequested keep using one linear
    // caret offset across both front ends.
    [[nodiscard]] int linearCursor() const;
    void setLinearCursor(int linear);
    // Place the caret at the end of the document (draftReset's caret-to-end contract).
    void moveCursorToEnd();

signals:
    void submitted(const QString& text);
    // Esc on an empty composer: ask the shell to move focus back to the panes.
    void leaveRequested();
    // Up/Down at the first/last line with the caret idle: walk the shared history.
    void historyPrevious();
    void historyNext();
    // Ctrl+O: open the workspace attachment picker, mirroring the GUI's "+"
    // attachment menu (Files).
    void attachRequested();

protected:
    void keyEvent(Tui::ZKeyEvent* event) override;
    // Draw a dim placeholder when the draft is empty (over the base paint, so the
    // editor's own caret still shows).
    void paintEvent(Tui::ZPaintEvent* event) override;
    [[nodiscard]] QSize sizeHint() const override;

private:
    // Push the live text + caret into the shared session (draft + completion
    // trigger). The controller guards against echoing the value back.
    void syncToSession();
    // Re-evaluate the desired height (1..kMaxRows) from the document line count and
    // relayout if it changed.
    void recomputeHeight();
    // Execute a readline EditCommand against the current line via the tested
    // lineedit transform (line-relative motions/kills with kill-ring); line-boundary
    // deletes fall through to the base editor. Returns true if consumed.
    bool applyReadline(lineedit::EditCommand cmd);
    // Route a key to the active reverse-search FSM. Returns true if the key was
    // consumed by the search; false means the FSM accepted the match and the caller
    // should keep processing the key.
    bool handleReverseSearch(Tui::ZKeyEvent* event);

    [[nodiscard]] Tui::ZTextEdit::Position positionForLinear(int linear) const;
    [[nodiscard]] int linearForPosition(Tui::ZTextEdit::Position pos) const;

    ComposerSessionController* m_session = nullptr;
    // Kill buffer for the readline-style edit commands (Ctrl+K/U/W -> Ctrl+Y).
    lineedit::KillRing m_killRing;
    // Current laid-out height in rows (drives sizeHint); grows with the document.
    int m_rows = 1;

    static constexpr int kMaxRows = 6;
};
