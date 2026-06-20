import QtQuick

// Swappable, scripted stand-in for the daemon runtime. Given a user prompt it
// plays a believable assistant turn (reasoning -> tool running/done -> streamed
// text -> flush) by pushing event maps through editor.ingestEvents([...]) on a
// re-armed timer. A real gateway later replaces this object by emitting the same
// event shapes; the chrome only reads turnState/elapsedMs/errorText.
//
// turnState: "idle" | "thinking" | "running" | "streaming" | "stalled" | "error"
Item {
    id: root

    // The EditorController whose ingest path the script drives.
    property var editor: null

    property bool active: false
    property string turnState: "idle"
    property int elapsedMs: 0
    property string errorText: ""

    // Stall threshold: if no scripted step lands within this window mid-turn, the
    // chrome shows "still thinking". One script gap is longer than this on
    // purpose so the indicator is exercised.
    readonly property int stallMs: 2000

    // Internal queue of { delayMs, event } actions and the cursor into it.
    property var _steps: []
    property int _index: 0
    property double _startedAt: 0
    // The turnState to restore when a stalled gap finally resolves.
    property string _resumeState: "thinking"

    signal turnStarted()
    signal turnFinished()

    function start(prompt) {
        cancel()
        _steps = _buildScript(String(prompt || ""))
        _index = 0
        _startedAt = Date.now()
        elapsedMs = 0
        errorText = ""
        active = true
        turnState = "thinking"
        elapsedTimer.start()
        _armStall()
        _scheduleNext()
        turnStarted()
    }

    function cancel() {
        stepTimer.stop()
        stallTimer.stop()
        elapsedTimer.stop()
        active = false
        if (turnState !== "error")
            turnState = "idle"
        _steps = []
        _index = 0
    }

    function _stateForEvent(type) {
        if (type === "text")
            return "streaming"
        if (type === "toolStarted" || type === "toolFinished")
            return "running"
        return "thinking"
    }

    function _scheduleNext() {
        if (_index >= _steps.length) {
            _complete()
            return
        }
        stepTimer.interval = Math.max(1, _steps[_index].delayMs)
        stepTimer.start()
    }

    function _runStep() {
        if (_index >= _steps.length) {
            _complete()
            return
        }
        const step = _steps[_index]
        _index += 1

        if (root.editor && step.event)
            root.editor.ingestEvents([step.event])

        const type = step.event ? step.event.type : ""
        if (step.error) {
            // Terminal failure: keep the error visible after the turn settles.
            errorText = step.errorText && step.errorText.length > 0
                ? step.errorText
                : qsTr("The agent hit an error completing this turn.")
        }
        if (type !== "flush") {
            turnState = _stateForEvent(type)
            _resumeState = turnState
            _armStall()
        }
        _scheduleNext()
    }

    function _complete() {
        stepTimer.stop()
        stallTimer.stop()
        elapsedTimer.stop()
        active = false
        turnState = errorText.length > 0 ? "error" : "idle"
        turnFinished()
    }

    function _armStall() {
        stallTimer.stop()
        stallTimer.start()
    }

    // Build a scripted turn. Content is canned but echoes the prompt so it reads
    // as a response; a prompt containing "fail" drives the error branch (N31).
    function _buildScript(prompt) {
        const wantsError = prompt.toLowerCase().indexOf("fail") >= 0
        const shortPrompt = prompt.length > 60 ? prompt.slice(0, 57) + "…" : prompt
        const steps = []

        steps.push({ delayMs: 250, event: { type: "reasoningDelta",
            text: "Reading the request" + (shortPrompt.length > 0 ? " — \"" + shortPrompt + "\". " : ". ") } })
        steps.push({ delayMs: 450, event: { type: "reasoningDelta",
            text: "I'll inspect the project and run the checks before answering." } })
        steps.push({ delayMs: 500, event: { type: "reasoningDone", durationMs: 1200 } })

        steps.push({ delayMs: 300, event: { type: "toolStarted", callId: "sim-term",
            name: "terminal", tone: "terminal", argsSummary: "ninja -C build && ctest",
            detailKind: "ansi-stream" } })

        if (wantsError) {
            steps.push({ delayMs: 900, event: { type: "toolFinished", callId: "sim-term",
                status: "error", durationMs: 900, detailKind: "ansi-stream",
                stderr: "\u001b[31merror:\u001b[0m build failed — 1 test did not pass\n" },
                error: true, errorText: qsTr("The build failed while completing this turn.") })
            steps.push({ delayMs: 250, event: { type: "text",
                text: "I hit a build failure before I could finish. See the terminal output above for details.\n" } })
            steps.push({ delayMs: 200, event: { type: "flush" } })
            return steps
        }

        steps.push({ delayMs: 1200, event: { type: "toolFinished", callId: "sim-term",
            status: "ok", durationMs: 1200, detailKind: "ansi-stream",
            stdout: "\u001b[32mPASS\u001b[0m  88 tests\n\u001b[1mBuild OK\u001b[0m\n" } })

        steps.push({ delayMs: 300, event: { type: "toolStarted", callId: "sim-search",
            name: "web_search", tone: "web", argsSummary: shortPrompt.length > 0 ? shortPrompt : "qml streaming",
            detailKind: "search-results" } })
        // Deliberately long gap (> stallMs) so the "still thinking" stall shows.
        steps.push({ delayMs: 2600, event: { type: "toolFinished", callId: "sim-search",
            status: "ok", durationMs: 2600, detailKind: "search-results",
            hits: [
                { title: "Qt Quick — Streaming UIs", url: "https://doc.qt.io/qt-6/qtquick-index.html",
                  snippet: "Build fluid, virtualized views backed by C++ models." },
                { title: "md4qt — incremental Markdown", url: "https://github.com/igormironchik/md4qt",
                  snippet: "A header-only CommonMark parser used for the transcript." }
            ] } })

        steps.push({ delayMs: 350, event: { type: "text",
            text: "Here's the summary: the build is green (88 tests pass) and " } })
        steps.push({ delayMs: 250, event: { type: "text",
            text: "the references confirm the streaming approach. " } })
        steps.push({ delayMs: 250, event: { type: "text",
            text: "Let me know if you'd like me to dig into any part further.\n" } })
        steps.push({ delayMs: 200, event: { type: "flush" } })
        return steps
    }

    Timer {
        id: stepTimer
        repeat: false
        onTriggered: root._runStep()
    }

    Timer {
        id: stallTimer
        interval: root.stallMs
        repeat: false
        onTriggered: {
            if (root.active)
                root.turnState = "stalled"
        }
    }

    Timer {
        id: elapsedTimer
        interval: 1000
        repeat: true
        onTriggered: root.elapsedMs = Date.now() - root._startedAt
    }
}
