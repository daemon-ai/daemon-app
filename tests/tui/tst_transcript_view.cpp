// Focus / keyboard tests for the TUI TranscriptView widget (as opposed to the
// pure TranscriptLayout in tst_transcript_render). They drive a real
// TranscriptView, holding an awaiting-approval / clarify block (so it is in
// interactive mode), inside an off-screen Tui terminal whose root is a
// focus-cycling container with one sibling widget. They assert the app-wide rule
// for interactive blocks: arrow keys walk the block's controls (focus stays on
// the transcript), while Tab / Shift+Tab leave to the next pane (focus moves to
// the sibling) instead of being trapped.

#include "transcript_view.h"

#include "core/document_store.h"

#include <Tui/ZRoot.h>
#include <Tui/ZTerminal.h>
#include <Tui/ZTest.h>
#include <Tui/ZWidget.h>
#include <Tui/ZWindow.h>

#include <QtTest>

namespace {

// An interactive turn: a dangerous tool awaiting approval plus a clarify form -
// enough to make TranscriptView::interactive() true (controls are non-empty).
QString interactiveMarkdown()
{
    return QStringLiteral(R"md(```msg
{"id":"m1","role":"assistant"}
```

```tool
{"callId":"c8","name":"terminal","tone":"terminal","status":"running","needsApproval":true,"allowPermanent":true,"approvalCommand":"rm -rf build"}
```

```tool
{"callId":"c7","name":"clarify","requestId":"q1","questions":[{"id":"db","prompt":"Which database?","choices":["PostgreSQL","SQLite"]},{"id":"notes","prompt":"Extra constraints?","allowFreeform":true}]}
```
)md");
}

// A plain, non-interactive transcript with two complete user/assistant turns, so
// the rewind picker has two anchors (the user messages u1 and u2).
QString rewindMarkdown()
{
    return QStringLiteral(R"md(```msg
{"id":"u1","role":"user"}
```

First question.

```msg
{"id":"m1","role":"assistant"}
```

First answer.

```msg
{"id":"u2","role":"user"}
```

Second question.

```msg
{"id":"m2","role":"assistant"}
```

Second answer.
)md");
}

} // namespace

class TranscriptViewTests : public QObject {
    Q_OBJECT

private:
    // Build an off-screen terminal mirroring the app's focus topology: a ZRoot
    // main widget holding a focus-cycling ZWindow (the same FocusContainerMode the
    // shell uses) with an interactive TranscriptView and one sibling focus stop.
    // Tab traversal is a property of the cycling window, so a plain ZWidget root
    // would not move focus. The out-params expose the widgets under test (owned by
    // `root`).
    static void buildScene(Tui::ZTerminal &terminal, Tui::ZRoot &root,
                           be::DocumentStore &doc, TranscriptView *&transcript,
                           Tui::ZWidget *&sibling)
    {
        terminal.setMainWidget(&root);

        auto *window = new Tui::ZWindow(&root);
        window->setFocusMode(Tui::FocusContainerMode::Cycle);
        window->setGeometry({ 0, 0, 80, 24 });

        transcript = new TranscriptView(window);
        transcript->setGeometry({ 0, 0, 80, 20 });

        sibling = new Tui::ZWidget(window);
        sibling->setFocusPolicy(Tui::StrongFocus);
        sibling->setGeometry({ 0, 20, 80, 2 });

        doc.loadMarkdown(interactiveMarkdown());
        transcript->setDocument(&doc);
        transcript->reload();
    }

    // A standalone transcript (no focus-cycle sibling needed) loaded with the
    // plain two-turn document, for the rewind-picker tests.
    static void buildRewindScene(Tui::ZTerminal &terminal, Tui::ZRoot &root,
                                 be::DocumentStore &doc, TranscriptView *&transcript)
    {
        terminal.setMainWidget(&root);
        transcript = new TranscriptView(&root);
        transcript->setGeometry({ 0, 0, 80, 24 });
        doc.loadMarkdown(rewindMarkdown());
        transcript->setDocument(&doc);
        transcript->reload();
        transcript->setFocus();
    }

private slots:
    // Tab is not consumed by an interactive block: it bubbles to the focus
    // container, moving focus to the next pane (the sibling).
    void tabLeavesInteractiveBlock()
    {
        Tui::ZTerminal terminal { Tui::ZTerminal::OffScreen { 80, 24 } };
        Tui::ZRoot root;
        be::DocumentStore doc;
        TranscriptView *transcript = nullptr;
        Tui::ZWidget *sibling = nullptr;
        buildScene(terminal, root, doc, transcript, sibling);

        transcript->setFocus();
        QVERIFY(transcript->focus());

        Tui::ZTest::sendKey(&terminal, Qt::Key_Tab, Qt::NoModifier);
        QVERIFY(!transcript->focus());
        QVERIFY(sibling->focus());
    }

    // Shift+Tab (Backtab) likewise escapes the block rather than walking controls.
    void shiftTabLeavesInteractiveBlock()
    {
        Tui::ZTerminal terminal { Tui::ZTerminal::OffScreen { 80, 24 } };
        Tui::ZRoot root;
        be::DocumentStore doc;
        TranscriptView *transcript = nullptr;
        Tui::ZWidget *sibling = nullptr;
        buildScene(terminal, root, doc, transcript, sibling);

        transcript->setFocus();
        QVERIFY(transcript->focus());

        Tui::ZTest::sendKey(&terminal, Qt::Key_Tab, Qt::ShiftModifier);
        QVERIFY(!transcript->focus());
        QVERIFY(sibling->focus());
    }

    // Arrow keys are consumed as in-block control navigation, so focus stays put.
    void arrowsKeepFocusInBlock()
    {
        Tui::ZTerminal terminal { Tui::ZTerminal::OffScreen { 80, 24 } };
        Tui::ZRoot root;
        be::DocumentStore doc;
        TranscriptView *transcript = nullptr;
        Tui::ZWidget *sibling = nullptr;
        buildScene(terminal, root, doc, transcript, sibling);

        for (Qt::Key key : { Qt::Key_Down, Qt::Key_Up, Qt::Key_Right, Qt::Key_Left }) {
            transcript->setFocus();
            QVERIFY(transcript->focus());
            Tui::ZTest::sendKey(&terminal, key, Qt::NoModifier);
            QVERIFY2(transcript->focus(), "arrow key must not move focus out of the block");
            QVERIFY(!sibling->focus());
        }
    }

    // 'r' opens the rewind picker on the most recent user message; Enter restores
    // it (re-run with its own text), emitting rewindRestoreRequested for that id.
    void rewindPickerRestoresSelectedAnchor()
    {
        Tui::ZTerminal terminal { Tui::ZTerminal::OffScreen { 80, 24 } };
        Tui::ZRoot root;
        be::DocumentStore doc;
        TranscriptView *transcript = nullptr;
        buildRewindScene(terminal, root, doc, transcript);

        QSignalSpy restoreSpy(transcript, &TranscriptView::rewindRestoreRequested);

        // Enter the picker (defaults to the last user message) and restore it.
        Tui::ZTest::sendText(&terminal, QStringLiteral("r"), Qt::NoModifier);
        Tui::ZTest::sendKey(&terminal, Qt::Key_Enter, Qt::NoModifier);
        QCOMPARE(restoreSpy.count(), 1);
        QCOMPARE(restoreSpy.at(0).at(0).toString(), QStringLiteral("u2"));

        // Re-enter, move up to the first user message, and restore that one.
        Tui::ZTest::sendText(&terminal, QStringLiteral("r"), Qt::NoModifier);
        Tui::ZTest::sendKey(&terminal, Qt::Key_Up, Qt::NoModifier);
        Tui::ZTest::sendKey(&terminal, Qt::Key_Enter, Qt::NoModifier);
        QCOMPARE(restoreSpy.count(), 2);
        QCOMPARE(restoreSpy.at(1).at(0).toString(), QStringLiteral("u1"));
    }

    // 'e' in the picker emits rewindEditRequested with the message id and its own
    // text (so the host can seed the composer).
    void rewindPickerEditEmitsText()
    {
        Tui::ZTerminal terminal { Tui::ZTerminal::OffScreen { 80, 24 } };
        Tui::ZRoot root;
        be::DocumentStore doc;
        TranscriptView *transcript = nullptr;
        buildRewindScene(terminal, root, doc, transcript);

        QSignalSpy editSpy(transcript, &TranscriptView::rewindEditRequested);

        Tui::ZTest::sendText(&terminal, QStringLiteral("r"), Qt::NoModifier);
        Tui::ZTest::sendText(&terminal, QStringLiteral("e"), Qt::NoModifier);
        QCOMPARE(editSpy.count(), 1);
        QCOMPARE(editSpy.at(0).at(0).toString(), QStringLiteral("u2"));
        QCOMPARE(editSpy.at(0).at(1).toString(), QStringLiteral("Second question."));
    }

    // Esc cancels the picker without emitting any rewind action.
    void rewindPickerEscCancels()
    {
        Tui::ZTerminal terminal { Tui::ZTerminal::OffScreen { 80, 24 } };
        Tui::ZRoot root;
        be::DocumentStore doc;
        TranscriptView *transcript = nullptr;
        buildRewindScene(terminal, root, doc, transcript);

        QSignalSpy restoreSpy(transcript, &TranscriptView::rewindRestoreRequested);
        QSignalSpy editSpy(transcript, &TranscriptView::rewindEditRequested);

        Tui::ZTest::sendText(&terminal, QStringLiteral("r"), Qt::NoModifier);
        Tui::ZTest::sendKey(&terminal, Qt::Key_Escape, Qt::NoModifier);
        // After cancelling, Enter is a no-op for rewind (not a stale restore).
        Tui::ZTest::sendKey(&terminal, Qt::Key_Enter, Qt::NoModifier);
        QCOMPARE(restoreSpy.count(), 0);
        QCOMPARE(editSpy.count(), 0);
    }
};

QTEST_MAIN(TranscriptViewTests)
#include "tst_transcript_view.moc"
