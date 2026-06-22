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
};

QTEST_MAIN(TranscriptViewTests)
#include "tst_transcript_view.moc"
