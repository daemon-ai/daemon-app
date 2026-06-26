#pragma once

// A small, dependency-free readline/emacs-style line-editing layer for the TUI
// composer. ZInputBox exposes only text()/setText()/cursorPosition()/
// setCursorPosition(), so every command here is a pure transform over a
// (text, cursor) pair plus a kill-ring; the widget re-applies the result.
//
// We deliberately do NOT link GNU Readline (GPLv3). Instead this implements the
// common emacs bindings and a tiny ~/.inputrc / $INPUTRC parser that remaps key
// sequences onto the same command set, so a user's existing rebindings of the
// functions we support are honored. Unknown functions / multi-key sequences /
// macros are skipped (the key then falls through to the stock input box).

#include <QHash>
#include <QString>
#include <QStringList>
#include <Qt> // Qt::Key, Qt::KeyboardModifiers

namespace lineedit {

// The subset of readline functions we implement. Names mirror readline's so the
// inputrc remap can resolve them directly.
enum class EditCommand {
    None,
    BeginningOfLine,    // C-a
    EndOfLine,          // C-e
    ForwardChar,        // C-f
    BackwardChar,       // C-b
    ForwardWord,        // M-f
    BackwardWord,       // M-b
    DeleteChar,         // C-d (under cursor; never EOF/quit here)
    BackwardDeleteChar, // rubout
    KillLine,           // C-k  (cursor -> end)
    UnixLineDiscard,    // C-u  (start -> cursor)
    KillWord,           // M-d  (word forward)
    BackwardKillWord,   // M-Backspace (word-char delimited, backward)
    UnixWordRubout,     // C-w  (whitespace delimited, backward)
    Yank,               // C-y
    YankPop,            // M-y
    TransposeChars,     // C-t
};

// Most-recent-first kill buffer with the bookkeeping readline needs: consecutive
// kills accumulate into one entry, and yank/yank-pop rotate through the ring.
class KillRing {
public:
    // Record killed text. `backward` prepends (kills toward the start) so the
    // pieces of an accumulating kill stay in logical order; `merge` folds into the
    // current top entry (set when the previous command was also a kill).
    void pushKill(const QString& killed, bool backward, bool merge);

    [[nodiscard]] bool isEmpty() const { return m_ring.isEmpty(); }
    [[nodiscard]] int size() const { return static_cast<int>(m_ring.size()); }

    // Start a fresh yank at the ring top.
    QString beginYank();
    // Advance to the next ring entry (yank-pop) and return it.
    QString rotate();

    void setYankSpan(int pos, int len) {
        m_yankPos = pos;
        m_yankLen = len;
    }
    [[nodiscard]] int yankPos() const { return m_yankPos; }
    [[nodiscard]] int yankLen() const { return m_yankLen; }

    [[nodiscard]] bool lastWasKill() const { return m_lastWasKill; }
    [[nodiscard]] bool lastWasYank() const { return m_lastWasYank; }
    // Called once per command to roll the kill/yank "sequence" flags forward.
    void endCommand(bool didKill, bool didYank);

private:
    QStringList m_ring;
    int m_yankIndex = 0;
    int m_yankPos = -1;
    int m_yankLen = 0;
    bool m_lastWasKill = false;
    bool m_lastWasYank = false;
};

class LineEditor {
public:
    // Apply `cmd` to (text, cursor), updating `ring`. Returns true when the key is
    // a recognized command the caller should consume (movement included); false
    // for EditCommand::None or a no-op yank-pop, so the caller can fall through.
    static bool applyCommand(EditCommand cmd, QString& text, int& cursor, KillRing& ring);

    // Resolve a key chord to a command using the cached (default + environment
    // inputrc) keymap. EditCommand::None means "not bound".
    static EditCommand lookup(int key, Qt::KeyboardModifiers mods);

    // Same, but resilient to how Tui Widgets delivers Ctrl/Alt+letter chords: they
    // arrive as a char event with key()==Qt::Key_unknown and the letter in text(),
    // not as Qt::Key_A+modifier. When the raw key is unbound and `text` is a single
    // ASCII letter/digit, retry with the key derived from it (so Ctrl+A etc. map).
    static EditCommand lookupEvent(int key, Qt::KeyboardModifiers mods, const QString& text);

    // --- Building blocks (exposed for unit tests) -------------------------------

    // Pack a chord into a stable hash key (only Ctrl/Alt/Shift/Meta are kept).
    static quint64 encode(int key, Qt::KeyboardModifiers mods);

    // The built-in emacs keymap (no inputrc applied).
    static QHash<quint64, EditCommand> defaultKeymap();

    // Overlay `~/.inputrc`-style bindings parsed from `contents` onto `map`.
    static void parseInputrcInto(QHash<quint64, EditCommand>& map, const QString& contents);

    // Map a readline function-name to a command (None if unsupported).
    static EditCommand commandForFunction(const QString& name);

private:
    static const QHash<quint64, EditCommand>& keymap();
};

} // namespace lineedit
