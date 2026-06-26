#include "line_editor.h"

#include <QtTest>

using lineedit::EditCommand;
using lineedit::KillRing;
using lineedit::LineEditor;

// Exercises the TUI composer's hand-rolled readline/emacs editing layer: the
// pure (text, cursor) command transforms + kill-ring semantics, and the tiny
// ~/.inputrc remap parser. No Tui Widgets / terminal involved - this is the
// toolkit-agnostic core the SubmitInputBox drives.
class TestLineEditor : public QObject {
    Q_OBJECT

private:
    // Apply a command to a (text, cursor) pair, returning the handled flag and
    // mutating the inputs in place. A fresh ring unless the caller supplies one.
    static bool apply(EditCommand cmd, QString& text, int& cursor, KillRing& ring) {
        return LineEditor::applyCommand(cmd, text, cursor, ring);
    }

private slots:
    void movementCommands() {
        QString t = QStringLiteral("hello world");
        int c = 5; // between "hello" and " world"
        KillRing ring;

        QVERIFY(apply(EditCommand::BeginningOfLine, t, c, ring));
        QCOMPARE(c, 0);

        QVERIFY(apply(EditCommand::EndOfLine, t, c, ring));
        QCOMPARE(c, t.size());

        QVERIFY(apply(EditCommand::BackwardChar, t, c, ring));
        QCOMPARE(c, t.size() - 1);
        QVERIFY(apply(EditCommand::ForwardChar, t, c, ring));
        QCOMPARE(c, t.size());

        // backward-word from the end lands at the start of "world".
        QVERIFY(apply(EditCommand::BackwardWord, t, c, ring));
        QCOMPARE(c, 6);
        // backward-word again -> start of "hello".
        QVERIFY(apply(EditCommand::BackwardWord, t, c, ring));
        QCOMPARE(c, 0);
        // forward-word -> end of "hello".
        QVERIFY(apply(EditCommand::ForwardWord, t, c, ring));
        QCOMPARE(c, 5);

        // The text is never modified by movement.
        QCOMPARE(t, QStringLiteral("hello world"));
    }

    void deleteCommands() {
        QString t = QStringLiteral("abc");
        int c = 1;
        KillRing ring;

        QVERIFY(apply(EditCommand::DeleteChar, t, c, ring)); // removes 'b'
        QCOMPARE(t, QStringLiteral("ac"));
        QCOMPARE(c, 1);

        QVERIFY(apply(EditCommand::BackwardDeleteChar, t, c, ring)); // removes 'a'
        QCOMPARE(t, QStringLiteral("c"));
        QCOMPARE(c, 0);

        // delete-char at end-of-line is a no-op (no EOF behavior).
        QString end = QStringLiteral("x");
        int ec = 1;
        QVERIFY(apply(EditCommand::DeleteChar, end, ec, ring));
        QCOMPARE(end, QStringLiteral("x"));
    }

    void killLineAndYank() {
        QString t = QStringLiteral("hello world");
        int c = 6; // before "world"
        KillRing ring;

        QVERIFY(apply(EditCommand::KillLine, t, c, ring));
        QCOMPARE(t, QStringLiteral("hello "));
        QCOMPARE(c, 6);

        // Yank restores the killed tail at the cursor.
        QVERIFY(apply(EditCommand::Yank, t, c, ring));
        QCOMPARE(t, QStringLiteral("hello world"));
        QCOMPARE(c, 11);
    }

    void unixLineDiscard() {
        QString t = QStringLiteral("hello world");
        int c = 6;
        KillRing ring;

        QVERIFY(apply(EditCommand::UnixLineDiscard, t, c, ring));
        QCOMPARE(t, QStringLiteral("world"));
        QCOMPARE(c, 0);

        // The discarded prefix is yankable.
        c = 5;
        QVERIFY(apply(EditCommand::Yank, t, c, ring));
        QCOMPARE(t, QStringLiteral("worldhello "));
    }

    void wordKills() {
        // backward-kill-word (word-char delimited).
        QString t = QStringLiteral("foo bar baz");
        int c = t.size();
        KillRing ring;
        QVERIFY(apply(EditCommand::BackwardKillWord, t, c, ring));
        QCOMPARE(t, QStringLiteral("foo bar "));
        QCOMPARE(c, 8);

        // kill-word (forward) from start removes "foo".
        QString t2 = QStringLiteral("foo bar");
        int c2 = 0;
        KillRing ring2;
        QVERIFY(apply(EditCommand::KillWord, t2, c2, ring2));
        QCOMPARE(t2, QStringLiteral(" bar"));
        QCOMPARE(c2, 0);

        // unix-word-rubout treats only whitespace as the delimiter, so it eats
        // through punctuation that backward-kill-word would stop at.
        QString t3 = QStringLiteral("a/b/c");
        int c3 = t3.size();
        KillRing ring3;
        QVERIFY(apply(EditCommand::UnixWordRubout, t3, c3, ring3));
        QCOMPARE(t3, QString());
        QCOMPARE(c3, 0);
    }

    void consecutiveKillsAccumulate() {
        // Two kill-words in a row accumulate into one kill-ring entry, so a single
        // yank brings back both (readline's kill-sequence behavior).
        QString t = QStringLiteral("alpha beta gamma");
        int c = 0;
        KillRing ring;
        QVERIFY(apply(EditCommand::KillWord, t, c, ring)); // kills "alpha"
        QVERIFY(apply(EditCommand::KillWord, t, c, ring)); // kills " beta"
        QCOMPARE(t, QStringLiteral(" gamma"));

        QVERIFY(apply(EditCommand::Yank, t, c, ring));
        QCOMPARE(t, QStringLiteral("alpha beta gamma"));
    }

    void yankPopRotatesRing() {
        // Build two independent kill entries (separated by a non-kill command so
        // they do not merge).
        QString t = QStringLiteral("one two");
        int c = t.size();
        KillRing ring;
        QVERIFY(apply(EditCommand::BackwardKillWord, t, c, ring)); // kill "two"
        QVERIFY(apply(EditCommand::BackwardChar, t, c, ring));     // break the sequence
        c = t.size();
        QVERIFY(apply(EditCommand::BackwardKillWord, t, c, ring)); // kill "one "
        QCOMPARE(t, QString());

        // Yank brings back the most recent ("one "); yank-pop swaps to "two".
        QVERIFY(apply(EditCommand::Yank, t, c, ring));
        QCOMPARE(t, QStringLiteral("one "));
        QVERIFY(apply(EditCommand::YankPop, t, c, ring));
        QCOMPARE(t, QStringLiteral("two"));
    }

    void transposeChars() {
        QString t = QStringLiteral("ab");
        int c = 2; // at end -> transpose the two preceding chars
        KillRing ring;
        QVERIFY(apply(EditCommand::TransposeChars, t, c, ring));
        QCOMPARE(t, QStringLiteral("ba"));
    }

    void defaultKeymapBindings() {
        QCOMPARE(LineEditor::lookup(Qt::Key_A, Qt::ControlModifier), EditCommand::BeginningOfLine);
        QCOMPARE(LineEditor::lookup(Qt::Key_E, Qt::ControlModifier), EditCommand::EndOfLine);
        QCOMPARE(LineEditor::lookup(Qt::Key_B, Qt::AltModifier), EditCommand::BackwardWord);
        QCOMPARE(LineEditor::lookup(Qt::Key_K, Qt::ControlModifier), EditCommand::KillLine);
        QCOMPARE(LineEditor::lookup(Qt::Key_W, Qt::ControlModifier), EditCommand::UnixWordRubout);
        // A plain letter is not a command (falls through to normal typing).
        QCOMPARE(LineEditor::lookup(Qt::Key_A, Qt::NoModifier), EditCommand::None);
    }

    void lookupEventDerivesLetterFromText() {
        // Tui Widgets delivers Ctrl/Alt+letter as char events: key()==Key_unknown
        // with the letter in text(). lookupEvent must still resolve the binding.
        QCOMPARE(LineEditor::lookupEvent(Qt::Key_unknown, Qt::ControlModifier, QStringLiteral("a")),
                 EditCommand::BeginningOfLine);
        QCOMPARE(LineEditor::lookupEvent(Qt::Key_unknown, Qt::AltModifier, QStringLiteral("b")),
                 EditCommand::BackwardWord);
        // Uppercase (Shift held / caps) still resolves the letter.
        QCOMPARE(LineEditor::lookupEvent(Qt::Key_unknown, Qt::ControlModifier, QStringLiteral("E")),
                 EditCommand::EndOfLine);
        // A plain letter (normal typing) is never a command.
        QCOMPARE(LineEditor::lookupEvent(Qt::Key_unknown, Qt::NoModifier, QStringLiteral("a")),
                 EditCommand::None);
        // A named key still resolves directly (Alt+Backspace -> backward-kill-word).
        QCOMPARE(LineEditor::lookupEvent(Qt::Key_Backspace, Qt::AltModifier, QString()),
                 EditCommand::BackwardKillWord);
    }

    void inputrcRemapQuoted() {
        QHash<quint64, EditCommand> map = LineEditor::defaultKeymap();
        // Rebind Ctrl+X to kill-line and Alt+P (\ep) to backward-word.
        LineEditor::parseInputrcInto(
            map, QStringLiteral("\"\\C-x\": kill-line\n\"\\ep\": backward-word\n"));

        QCOMPARE(map.value(LineEditor::encode(Qt::Key_X, Qt::ControlModifier)),
                 EditCommand::KillLine);
        QCOMPARE(map.value(LineEditor::encode(Qt::Key_P, Qt::AltModifier)),
                 EditCommand::BackwardWord);
    }

    void inputrcRemapSymbolicAndSkips() {
        QHash<quint64, EditCommand> map = LineEditor::defaultKeymap();
        LineEditor::parseInputrcInto(
            map,
            QStringLiteral("# a comment\n"
                           "Control-l: end-of-line\n"
                           "Meta-u: forward-word\n"
                           "\"\\C-x\": \"a macro string\"\n"    // macro -> skipped
                           "\"\\C-g\": some-unknown-function\n" // unknown -> skipped
                           "$if mode=vi\n"
                           "\"\\C-a\": kill-line\n" // inside vi block -> skipped
                           "$endif\n"));

        QCOMPARE(map.value(LineEditor::encode(Qt::Key_L, Qt::ControlModifier)),
                 EditCommand::EndOfLine);
        QCOMPARE(map.value(LineEditor::encode(Qt::Key_U, Qt::AltModifier)),
                 EditCommand::ForwardWord);
        // Macro / unknown bindings did not register.
        QCOMPARE(map.value(LineEditor::encode(Qt::Key_X, Qt::ControlModifier)), EditCommand::None);
        QCOMPARE(map.value(LineEditor::encode(Qt::Key_G, Qt::ControlModifier)), EditCommand::None);
        // The vi-block rebind of Ctrl+A was skipped, so the default survives.
        QCOMPARE(map.value(LineEditor::encode(Qt::Key_A, Qt::ControlModifier)),
                 EditCommand::BeginningOfLine);
    }
};

QTEST_MAIN(TestLineEditor)
#include "tst_line_editor.moc"
