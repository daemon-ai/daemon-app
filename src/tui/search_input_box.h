#pragma once

#include <Tui/ZInputBox.h>

// A one-line search field above the session list. It is NOT a focus stop:
// the session list owns focus and forwards typed characters here (type-ahead),
// so this box is a passive display of the live query bound to
// SessionsListModel::setSearch. When the list is focused it shows a caret
// after the query to mark where keystrokes land; when empty it shows a placeholder.
class SearchInputBox : public Tui::ZInputBox {
    Q_OBJECT

public:
    using Tui::ZInputBox::ZInputBox;

    // Toggle the typing caret (driven by the session list's focus state).
    void setTypingActive(bool active);

protected:
    void paintEvent(Tui::ZPaintEvent* event) override;

private:
    bool m_typingActive = false;
};

// The one-line in-transcript find field (Ctrl+F). Live text edits drive the
// query; Enter / Down cycle to the next match, Up / Shift+Enter to the previous,
// Esc closes. Everything else edits the query text (so 'n'/'N' type normally).
class TranscriptSearchBox : public Tui::ZInputBox {
    Q_OBJECT

public:
    using Tui::ZInputBox::ZInputBox;

signals:
    void nextRequested();
    void previousRequested();
    void closeRequested();

protected:
    void keyEvent(Tui::ZKeyEvent* event) override;
};
