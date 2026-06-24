import QtQuick
import QtTest
import DaemonApp.Composer

// Behavioral coverage for the ported composer: enter-to-send, shift+enter
// newline, queue-while-busy with idle auto-drain, per-conversation draft swap,
// and sent-message history recall. Logic paths are driven through the composer's
// public functions/signals; the newline case exercises the real key handler.
TestCase {
    id: tc
    name: "Composer"
    width: 640
    height: 400
    when: windowShown

    Composer {
        id: composer
        width: 640
        sessionId: 1
    }

    SignalSpy {
        id: submitSpy
        target: composer
        signalName: "submitted"
    }

    function init() {
        // Reset to a clean, idle state on a fresh conversation each test.
        composer.busy = false;
        composer.sessionId = 900;
        composer.draftText = "";
        submitSpy.clear();
    }

    function test_enter_sends() {
        composer.draftText = "hello world";
        composer.submitDraft();

        compare(submitSpy.count, 1, "one submit");
        compare(submitSpy.signalArguments[0][0], "hello world", "submitted text");
        compare(composer.draftText, "", "draft cleared after send");
    }

    function test_empty_does_not_send() {
        composer.draftText = "   ";
        composer.submitDraft();
        compare(submitSpy.count, 0, "whitespace-only draft does not send");
    }

    function test_shift_enter_inserts_newline() {
        composer.draftText = "line one";
        composer.focusInput();
        composer.draftText = "line one"; // ensure caret at end via re-set
        keyClick(Qt.Key_Return, Qt.ShiftModifier);

        verify(composer.draftText.indexOf("\n") >= 0, "shift+enter inserts a newline");
        compare(submitSpy.count, 0, "shift+enter must not submit");
    }

    function test_queue_while_busy_then_autodrain() {
        composer.busy = true;
        composer.draftText = "queued turn";
        composer.submitDraft();

        compare(submitSpy.count, 0, "busy submit does not send immediately");
        compare(composer.queueCount, 1, "draft was queued");
        compare(composer.draftText, "", "composer cleared after queueing");

        // Going idle drains the head.
        composer.busy = false;
        tryCompare(submitSpy, "count", 1, 1000);
        compare(submitSpy.signalArguments[0][0], "queued turn", "drained text");
        tryCompare(composer, "queueCount", 0, 1000);
    }

    function test_draft_swaps_per_conversation() {
        composer.sessionId = 10;
        composer.draftText = "draft-A";

        composer.sessionId = 11;
        compare(composer.draftText, "", "new conversation starts empty");
        composer.draftText = "draft-B";

        composer.sessionId = 10;
        compare(composer.draftText, "draft-A", "conversation 10 draft restored");

        composer.sessionId = 11;
        compare(composer.draftText, "draft-B", "conversation 11 draft restored");
    }

    function test_history_recall() {
        composer.sessionId = 20;
        composer.draftText = "first";
        composer.submitDraft();
        composer.draftText = "second";
        composer.submitDraft();
        compare(composer.draftText, "", "cleared after sends");

        composer.browseUp();
        compare(composer.draftText, "second", "most recent first");
        composer.browseUp();
        compare(composer.draftText, "first", "older next");
        composer.browseUp();
        compare(composer.draftText, "first", "stops at oldest");

        composer.browseDown();
        compare(composer.draftText, "second", "back toward present");
        composer.browseDown();
        compare(composer.draftText, "", "restores the live (empty) draft");
    }
}
