// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "turn_controller.h"

#include <QCoreApplication>
#include <QDateTime>

namespace {

QVariantMap hit(const QString& title, const QString& url, const QString& snippet) {
    return QVariantMap{
        {QStringLiteral("title"), title},
        {QStringLiteral("url"), url},
        {QStringLiteral("snippet"), snippet},
    };
}

// Status-only event maps, shaped like the daemon's usage/context/rate-limit
// stream. They carry no transcript content (the ingest path skips them) and are
// consumed by StatusBarModel; emitting them now pins the event contract.
QVariantMap usageEvent(int tokensIn, int tokensOut, double costUsd) {
    return QVariantMap{
        {QStringLiteral("type"), QStringLiteral("usage")},
        {QStringLiteral("tokensIn"), tokensIn},
        {QStringLiteral("tokensOut"), tokensOut},
        {QStringLiteral("costUsd"), costUsd},
    };
}

QVariantMap contextEvent(int used, int max) {
    return QVariantMap{
        {QStringLiteral("type"), QStringLiteral("context")},
        {QStringLiteral("used"), used},
        {QStringLiteral("max"), max},
    };
}

QVariantMap rateLimitEvent(int remaining, int limit) {
    return QVariantMap{
        {QStringLiteral("type"), QStringLiteral("rateLimit")},
        {QStringLiteral("remaining"), remaining},
        {QStringLiteral("limit"), limit},
    };
}

// A delegation/subagent lifecycle event (the daemon's subagent.* family). One row
// per `id`; `status` is "running" | "done" | "error". `detail` is a short live
// status line. Consumed by SubagentModel; the transcript ingest skips it.
QVariantMap subagentEvent(const QString& id, const QString& title, const QString& status,
                          const QString& detail) {
    return QVariantMap{
        {QStringLiteral("type"), QStringLiteral("subagent")},
        {QStringLiteral("id"), id},
        {QStringLiteral("title"), title},
        {QStringLiteral("status"), status},
        {QStringLiteral("detail"), detail},
    };
}

} // namespace

TurnController::TurnController(QObject* parent) : ITurnEngine(parent) {
    m_stepTimer.setSingleShot(true);
    m_stallTimer.setSingleShot(true);
    m_stallTimer.setInterval(kStallMs);
    m_elapsedTimer.setInterval(1000);

    connect(&m_stepTimer, &QTimer::timeout, this, &TurnController::runStep);
    connect(&m_stallTimer, &QTimer::timeout, this, [this] {
        if (m_active) {
            setTurnState(QStringLiteral("stalled"));
        }
    });
    connect(&m_elapsedTimer, &QTimer::timeout, this, [this] {
        setElapsedMs(static_cast<int>(QDateTime::currentMSecsSinceEpoch() - m_startedAt));
    });
}

void TurnController::setActive(bool active) {
    if (m_active == active) {
        return;
    }
    m_active = active;
    emit activeChanged();
}

void TurnController::setTurnState(const QString& state) {
    if (m_turnState == state) {
        return;
    }
    m_turnState = state;
    emit turnStateChanged();
}

void TurnController::setElapsedMs(int ms) {
    if (m_elapsedMs == ms) {
        return;
    }
    m_elapsedMs = ms;
    emit elapsedMsChanged();
}

void TurnController::setErrorText(const QString& text) {
    if (m_errorText == text) {
        return;
    }
    m_errorText = text;
    emit errorTextChanged();
}

void TurnController::start(const QString& prompt) {
    cancel();
    m_steps = buildScript(prompt);
    m_index = 0;
    m_paused = false;
    m_startedAt = QDateTime::currentMSecsSinceEpoch();
    setElapsedMs(0);
    setErrorText(QString());
    setActive(true);
    setTurnState(QStringLiteral("thinking"));
    m_elapsedTimer.start();
    armStall();
    scheduleNext();
    emit turnStarted();
}

void TurnController::cancel() {
    m_stepTimer.stop();
    m_stallTimer.stop();
    m_elapsedTimer.stop();
    setActive(false);
    m_paused = false;
    if (m_turnState != QStringLiteral("error")) {
        setTurnState(QStringLiteral("idle"));
    }
    m_steps.clear();
    m_index = 0;
}

QString TurnController::stateForEvent(const QString& type) {
    if (type == QStringLiteral("text")) {
        return QStringLiteral("streaming");
    }
    if (type == QStringLiteral("toolStarted") || type == QStringLiteral("toolFinished")) {
        return QStringLiteral("running");
    }
    return QStringLiteral("thinking");
}

void TurnController::scheduleNext() {
    if (m_index >= m_steps.size()) {
        complete();
        return;
    }
    m_stepTimer.setInterval(qMax(1, m_steps.at(m_index).delayMs));
    m_stepTimer.start();
}

void TurnController::runStep() {
    if (m_index >= m_steps.size()) {
        complete();
        return;
    }
    const Step step = m_steps.at(m_index);
    m_index += 1;

    if (!step.event.isEmpty()) {
        emit eventsEmitted(QVariantList{step.event});
    }

    const QString type = step.event.value(QStringLiteral("type")).toString();
    if (step.error) {
        // Terminal failure: keep the error visible after the turn settles.
        setErrorText(!step.errorText.isEmpty()
                         ? step.errorText
                         : tr("The agent hit an error completing this turn."));
    }
    if (type != QStringLiteral("flush")) {
        setTurnState(stateForEvent(type));
        m_resumeState = m_turnState;
        armStall();
    }

    // An approval gate pauses the turn here: stop the stall timer and wait for
    // resume() (driven by the inline approval answer) before scheduling the rest.
    if (step.gate) {
        m_paused = true;
        m_stallTimer.stop();
        // Surface the gate out-of-band so a hidden GUI/inactive TUI can alert.
        emit awaitingInput(step.hostRequestKind.isEmpty() ? QStringLiteral("approval")
                                                          : step.hostRequestKind);
        // A host-input gate additionally raises a masked-prompt request that the
        // front end answers before resume().
        if (!step.hostRequestKind.isEmpty()) {
            emit hostRequested(step.hostRequestKind, step.hostRequestPrompt);
        }
        return;
    }

    scheduleNext();
}

void TurnController::resume() {
    if (!m_active || !m_paused) {
        return;
    }
    m_paused = false;
    armStall();
    scheduleNext();
}

void TurnController::respondApproval(const QString& requestId, bool allow, bool allowPermanent,
                                     const QString& reason) {
    // The simulator holds no durable policy, so "allow permanently" plays out exactly like a
    // one-shot allow; accepted only to satisfy the ITurnEngine seam (wire v28).
    Q_UNUSED(allowPermanent)
    if (!m_active || !m_paused) {
        return;
    }
    // The simulator drives the gated tool's completion itself so no front end
    // fabricates a result: emit this tool's own toolFinished (the same shape the
    // daemon streams as a real ToolFinished), then a decision-appropriate text,
    // then resume() to run the trailing flush that closes the assistant stream.
    const QString callId = requestId.isEmpty() ? QStringLiteral("sim-approve") : requestId;
    QVariantMap finished{
        {QStringLiteral("type"), QStringLiteral("toolFinished")},
        {QStringLiteral("callId"), callId},
        {QStringLiteral("durationMs"), 1400},
        {QStringLiteral("detailKind"), QStringLiteral("ansi-stream")},
    };
    if (allow) {
        finished.insert(QStringLiteral("status"), QStringLiteral("ok"));
        finished.insert(
            QStringLiteral("stdout"),
            QStringLiteral("\u001b[32m\u2713\u001b[0m approved \u2014 command finished\n"));
    } else {
        finished.insert(QStringLiteral("status"), QStringLiteral("error"));
        // [wave2:app-approvals-safety] D3: reflect the operator's deny reason (wire v29) in the
        // simulated refusal so the mock mirrors the daemon's reason-threading behavior.
        finished.insert(QStringLiteral("stderr"),
                        reason.isEmpty() ? QStringLiteral("denied by operator\n")
                                         : QStringLiteral("denied by operator: %1\n").arg(reason));
    }
    const QString followUp = allow
                                 ? QStringLiteral("\n\nThe command finished after your approval.\n")
                                 : QStringLiteral("\n\nThe command was denied, so I skipped it.\n");
    emit eventsEmitted(
        QVariantList{finished, QVariantMap{{QStringLiteral("type"), QStringLiteral("text")},
                                           {QStringLiteral("text"), followUp}}});
    resume();
}

void TurnController::complete() {
    m_stepTimer.stop();
    m_stallTimer.stop();
    m_elapsedTimer.stop();
    setActive(false);
    setTurnState(m_errorText.isEmpty() ? QStringLiteral("idle") : QStringLiteral("error"));
    emit turnFinished();
}

void TurnController::armStall() {
    m_stallTimer.stop();
    m_stallTimer.start();
}

QList<TurnController::Step> TurnController::buildScript(const QString& prompt) {
    // Build a simulator turn. Content is canned but echoes the prompt so the
    // demo UI exercises realistic block shapes; keyword triggers drive specific
    // approval/error/secret branches until a daemon event stream replaces this.
    const QString lower = prompt.toLower();
    const bool wantsError = lower.indexOf(QStringLiteral("fail")) >= 0;
    const bool wantsApproval = lower.indexOf(QStringLiteral("approve")) >= 0;
    // A prompt mentioning sudo / a secret pauses for a masked host input, so the
    // sudo-password and secret/API-key prompt shells are demoable end-to-end.
    const bool wantsSudo = lower.indexOf(QStringLiteral("sudo")) >= 0;
    const bool wantsSecret = lower.indexOf(QStringLiteral("secret")) >= 0 ||
                             lower.indexOf(QStringLiteral("api key")) >= 0 ||
                             lower.indexOf(QStringLiteral("apikey")) >= 0;
    // A prompt mentioning "clarify"/"which" pauses for an inline clarify form (the
    // daemon's HostRequest::Choice seam), so CHA-5 is exercised in the simulator.
    const bool wantsClarify = lower.indexOf(QStringLiteral("clarify")) >= 0 ||
                              lower.indexOf(QStringLiteral("which")) >= 0;
    const QString shortPrompt =
        prompt.length() > 60 ? (prompt.left(57) + QStringLiteral("\u2026")) : prompt;
    QList<Step> steps;

    steps.push_back(
        {250,
         QVariantMap{{QStringLiteral("type"), QStringLiteral("reasoningDelta")},
                     {QStringLiteral("text"),
                      QStringLiteral("Reading the request") +
                          (!shortPrompt.isEmpty() ? (QStringLiteral(" \u2014 \"") + shortPrompt +
                                                     QStringLiteral("\". "))
                                                  : QStringLiteral(". "))}},
         false, QString()});
    steps.push_back(
        {450,
         QVariantMap{
             {QStringLiteral("type"), QStringLiteral("reasoningDelta")},
             {QStringLiteral("text"),
              QStringLiteral("I'll inspect the project and run the checks before answering.")}},
         false, QString()});
    steps.push_back({500,
                     QVariantMap{{QStringLiteral("type"), QStringLiteral("reasoningDone")},
                                 {QStringLiteral("durationMs"), 1200}},
                     false, QString()});

    // Prime the status contract: an initial context fill + the prompt's input
    // token cost. Later steps bump these so the status bar animates over a turn.
    steps.push_back({120, contextEvent(14200, 128000), false, QString()});
    steps.push_back({80, usageEvent(1800, 0, 0.011), false, QString()});

    if (wantsSudo || wantsSecret) {
        // Pause for a masked host input (the daemon's HostRequestKind seam). The
        // step carries no transcript event - it only raises the prompt and gates.
        const QString kind = wantsSudo ? QStringLiteral("password") : QStringLiteral("secret");
        const QString ask = wantsSudo ? QStringLiteral("[sudo] password for deploy")
                                      : QStringLiteral("Paste the API key for the deploy gateway");
        Step gate;
        gate.delayMs = 250;
        gate.gate = true;
        gate.hostRequestKind = kind;
        gate.hostRequestPrompt = ask;
        steps.push_back(gate);
        steps.push_back({250,
                         QVariantMap{{QStringLiteral("type"), QStringLiteral("text")},
                                     {QStringLiteral("text"),
                                      QStringLiteral("\n\nCredential accepted - continuing.\n")}},
                         false, QString()});
        steps.push_back({200, QVariantMap{{QStringLiteral("type"), QStringLiteral("flush")}}, false,
                         QString()});
        return steps;
    }

    if (wantsClarify) {
        // Emit a clarify tool (a single-select question) and gate until the inline
        // ClarifyBlock answer drives resume().
        const QVariantList options{
            QVariantMap{{QStringLiteral("id"), QStringLiteral("staging")},
                        {QStringLiteral("label"), QStringLiteral("Staging")}},
            QVariantMap{{QStringLiteral("id"), QStringLiteral("production")},
                        {QStringLiteral("label"), QStringLiteral("Production")}}};
        const QVariantMap question{
            {QStringLiteral("id"), QStringLiteral("sim-clarify")},
            {QStringLiteral("prompt"), QStringLiteral("Which environment should I target?")},
            {QStringLiteral("kind"), QStringLiteral("single")},
            {QStringLiteral("options"), options}};
        steps.push_back(
            {300,
             QVariantMap{{QStringLiteral("type"), QStringLiteral("toolStarted")},
                         {QStringLiteral("callId"), QStringLiteral("sim-clarify")},
                         {QStringLiteral("name"), QStringLiteral("clarify")},
                         {QStringLiteral("argsSummary"), QStringLiteral("needs a choice")},
                         {QStringLiteral("requestId"), QStringLiteral("sim-clarify")},
                         {QStringLiteral("questions"), QVariantList{question}}},
             false, QString(), /*gate=*/true});
        steps.push_back(
            {250,
             QVariantMap{{QStringLiteral("type"), QStringLiteral("text")},
                         {QStringLiteral("text"),
                          QStringLiteral("\n\nGreat — proceeding with your selection.\n")}},
             false, QString()});
        steps.push_back({200, QVariantMap{{QStringLiteral("type"), QStringLiteral("flush")}}, false,
                         QString()});
        return steps;
    }

    if (wantsApproval) {
        // A dangerous tool pauses the live turn for inline approval: emit an
        // awaiting-approval toolStarted and gate (pause). respondApproval() then
        // emits the gated tool's OWN toolFinished (ok/stdout on allow, error on
        // deny) plus a matching text and resumes — so the simulator produces the
        // tool RESULT itself and the front ends never fabricate one. The trailing
        // flush closes the assistant stream after that scripted completion.
        steps.push_back(
            {300,
             QVariantMap{{QStringLiteral("type"), QStringLiteral("toolStarted")},
                         {QStringLiteral("callId"), QStringLiteral("sim-approve")},
                         {QStringLiteral("name"), QStringLiteral("terminal")},
                         {QStringLiteral("tone"), QStringLiteral("terminal")},
                         {QStringLiteral("argsSummary"), QStringLiteral("rm -rf build-test")},
                         {QStringLiteral("needsApproval"), true},
                         {QStringLiteral("allowPermanent"), true},
                         {QStringLiteral("approvalCommand"),
                          QStringLiteral("rm -rf build-test && cmake --preset test")}},
             false, QString(), /*gate=*/true});
        steps.push_back({200, QVariantMap{{QStringLiteral("type"), QStringLiteral("flush")}}, false,
                         QString()});
        return steps;
    }

    steps.push_back(
        {300,
         QVariantMap{{QStringLiteral("type"), QStringLiteral("toolStarted")},
                     {QStringLiteral("callId"), QStringLiteral("sim-term")},
                     {QStringLiteral("name"), QStringLiteral("terminal")},
                     {QStringLiteral("tone"), QStringLiteral("terminal")},
                     {QStringLiteral("argsSummary"), QStringLiteral("ninja -C build && ctest")},
                     {QStringLiteral("detailKind"), QStringLiteral("ansi-stream")}},
         false, QString()});

    if (wantsError) {
        steps.push_back(
            {900,
             QVariantMap{{QStringLiteral("type"), QStringLiteral("toolFinished")},
                         {QStringLiteral("callId"), QStringLiteral("sim-term")},
                         {QStringLiteral("status"), QStringLiteral("error")},
                         {QStringLiteral("durationMs"), 900},
                         {QStringLiteral("detailKind"), QStringLiteral("ansi-stream")},
                         {QStringLiteral("stderr"),
                          QStringLiteral("\u001b[31merror:\u001b[0m build failed \u2014 1 test "
                                         "did not pass\n")}},
             true, tr("The build failed while completing this turn.")});
        steps.push_back(
            {250,
             QVariantMap{{QStringLiteral("type"), QStringLiteral("text")},
                         {QStringLiteral("text"),
                          QStringLiteral("I hit a build failure before I could finish. See the "
                                         "terminal output above for details.\n")}},
             false, QString()});
        steps.push_back({200, QVariantMap{{QStringLiteral("type"), QStringLiteral("flush")}}, false,
                         QString()});
        return steps;
    }

    steps.push_back(
        {1200,
         QVariantMap{
             {QStringLiteral("type"), QStringLiteral("toolFinished")},
             {QStringLiteral("callId"), QStringLiteral("sim-term")},
             {QStringLiteral("status"), QStringLiteral("ok")},
             {QStringLiteral("durationMs"), 1200},
             {QStringLiteral("detailKind"), QStringLiteral("ansi-stream")},
             {QStringLiteral("stdout"),
              QStringLiteral("\u001b[32mPASS\u001b[0m  88 tests\n\u001b[1mBuild OK\u001b[0m\n")}},
         false, QString()});

    // Tool output grew the context and spent output tokens.
    steps.push_back({80, usageEvent(0, 640, 0.018), false, QString()});
    steps.push_back({80, contextEvent(31800, 128000), false, QString()});

    // Delegate to two subagents (the daemon's delegation concept). They appear in
    // the status stack as live rows, tick progress, then settle done.
    steps.push_back({120,
                     subagentEvent(QStringLiteral("sub-explore"), QStringLiteral("explore"),
                                   QStringLiteral("running"), QStringLiteral("scanning sources")),
                     false, QString()});
    steps.push_back({120,
                     subagentEvent(QStringLiteral("sub-tests"), QStringLiteral("run-tests"),
                                   QStringLiteral("running"), QStringLiteral("starting suite")),
                     false, QString()});

    steps.push_back(
        {300,
         QVariantMap{{QStringLiteral("type"), QStringLiteral("toolStarted")},
                     {QStringLiteral("callId"), QStringLiteral("sim-search")},
                     {QStringLiteral("name"), QStringLiteral("web_search")},
                     {QStringLiteral("tone"), QStringLiteral("web")},
                     {QStringLiteral("argsSummary"),
                      !shortPrompt.isEmpty() ? shortPrompt : QStringLiteral("qml streaming")},
                     {QStringLiteral("detailKind"), QStringLiteral("search-results")}},
         false, QString()});
    // Deliberately long gap (> kStallMs) so the "still thinking" stall shows.
    steps.push_back(
        {2600,
         QVariantMap{
             {QStringLiteral("type"), QStringLiteral("toolFinished")},
             {QStringLiteral("callId"), QStringLiteral("sim-search")},
             {QStringLiteral("status"), QStringLiteral("ok")},
             {QStringLiteral("durationMs"), 2600},
             {QStringLiteral("detailKind"), QStringLiteral("search-results")},
             {QStringLiteral("hits"),
              QVariantList{
                  hit(QStringLiteral("Qt Quick \u2014 Streaming UIs"),
                      QStringLiteral("https://doc.qt.io/qt-6/qtquick-index.html"),
                      QStringLiteral("Build fluid, virtualized views backed by C++ models.")),
                  hit(QStringLiteral("md4qt \u2014 incremental Markdown"),
                      QStringLiteral("https://github.com/igormironchik/md4qt"),
                      QStringLiteral(
                          "A header-only CommonMark parser used for the transcript."))}}},
         false, QString()});

    // Search results pushed context further and consumed the provider window.
    steps.push_back({80, usageEvent(0, 1200, 0.034), false, QString()});
    steps.push_back({80, contextEvent(52600, 128000), false, QString()});
    steps.push_back({80, rateLimitEvent(74, 80), false, QString()});

    // The delegated subagents report progress, then settle.
    steps.push_back({80,
                     subagentEvent(QStringLiteral("sub-explore"), QStringLiteral("explore"),
                                   QStringLiteral("done"), QStringLiteral("42 files")),
                     false, QString()});
    steps.push_back({80,
                     subagentEvent(QStringLiteral("sub-tests"), QStringLiteral("run-tests"),
                                   QStringLiteral("done"), QStringLiteral("88 passed")),
                     false, QString()});

    steps.push_back(
        {350,
         QVariantMap{
             {QStringLiteral("type"), QStringLiteral("text")},
             {QStringLiteral("text"),
              QStringLiteral("Here's the summary: the build is green (88 tests pass) and ")}},
         false, QString()});
    steps.push_back(
        {250,
         QVariantMap{{QStringLiteral("type"), QStringLiteral("text")},
                     {QStringLiteral("text"),
                      QStringLiteral("the references confirm the streaming approach. ")}},
         false, QString()});
    steps.push_back(
        {250,
         QVariantMap{
             {QStringLiteral("type"), QStringLiteral("text")},
             {QStringLiteral("text"),
              QStringLiteral("Let me know if you'd like me to dig into any part further.\n")}},
         false, QString()});
    steps.push_back(
        {200, QVariantMap{{QStringLiteral("type"), QStringLiteral("flush")}}, false, QString()});
    return steps;
}
