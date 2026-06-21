#include "turn_controller.h"

#include <QCoreApplication>
#include <QDateTime>

namespace {

QVariantMap hit(const QString& title, const QString& url, const QString& snippet)
{
    return QVariantMap{
        { QStringLiteral("title"), title },
        { QStringLiteral("url"), url },
        { QStringLiteral("snippet"), snippet },
    };
}

} // namespace

TurnController::TurnController(QObject* parent)
    : QObject(parent)
{
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

void TurnController::setActive(bool active)
{
    if (m_active == active) {
        return;
    }
    m_active = active;
    emit activeChanged();
}

void TurnController::setTurnState(const QString& state)
{
    if (m_turnState == state) {
        return;
    }
    m_turnState = state;
    emit turnStateChanged();
}

void TurnController::setElapsedMs(int ms)
{
    if (m_elapsedMs == ms) {
        return;
    }
    m_elapsedMs = ms;
    emit elapsedMsChanged();
}

void TurnController::setErrorText(const QString& text)
{
    if (m_errorText == text) {
        return;
    }
    m_errorText = text;
    emit errorTextChanged();
}

void TurnController::start(const QString& prompt)
{
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

void TurnController::cancel()
{
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

QString TurnController::stateForEvent(const QString& type)
{
    if (type == QStringLiteral("text")) {
        return QStringLiteral("streaming");
    }
    if (type == QStringLiteral("toolStarted") || type == QStringLiteral("toolFinished")) {
        return QStringLiteral("running");
    }
    return QStringLiteral("thinking");
}

void TurnController::scheduleNext()
{
    if (m_index >= m_steps.size()) {
        complete();
        return;
    }
    m_stepTimer.setInterval(qMax(1, m_steps.at(m_index).delayMs));
    m_stepTimer.start();
}

void TurnController::runStep()
{
    if (m_index >= m_steps.size()) {
        complete();
        return;
    }
    const Step step = m_steps.at(m_index);
    m_index += 1;

    if (!step.event.isEmpty()) {
        emit eventsEmitted(QVariantList{ step.event });
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
        return;
    }

    scheduleNext();
}

void TurnController::resume()
{
    if (!m_active || !m_paused) {
        return;
    }
    m_paused = false;
    armStall();
    scheduleNext();
}

void TurnController::complete()
{
    m_stepTimer.stop();
    m_stallTimer.stop();
    m_elapsedTimer.stop();
    setActive(false);
    setTurnState(m_errorText.isEmpty() ? QStringLiteral("idle") : QStringLiteral("error"));
    emit turnFinished();
}

void TurnController::armStall()
{
    m_stallTimer.stop();
    m_stallTimer.start();
}

QList<TurnController::Step> TurnController::buildScript(const QString& prompt)
{
    // Build a scripted turn. Content is canned but echoes the prompt so it reads
    // as a response; a prompt containing "fail" drives the error branch.
    const bool wantsError = prompt.toLower().indexOf(QStringLiteral("fail")) >= 0;
    const bool wantsApproval = prompt.toLower().indexOf(QStringLiteral("approve")) >= 0;
    const QString shortPrompt
        = prompt.length() > 60 ? (prompt.left(57) + QStringLiteral("\u2026")) : prompt;
    QList<Step> steps;

    steps.push_back({ 250,
                      QVariantMap{ { QStringLiteral("type"), QStringLiteral("reasoningDelta") },
                                   { QStringLiteral("text"),
                                     QStringLiteral("Reading the request")
                                         + (!shortPrompt.isEmpty()
                                                ? (QStringLiteral(" \u2014 \"") + shortPrompt
                                                   + QStringLiteral("\". "))
                                                : QStringLiteral(". ")) } },
                      false, QString() });
    steps.push_back(
        { 450,
          QVariantMap{ { QStringLiteral("type"), QStringLiteral("reasoningDelta") },
                       { QStringLiteral("text"),
                         QStringLiteral(
                             "I'll inspect the project and run the checks before answering.") } },
          false, QString() });
    steps.push_back({ 500,
                      QVariantMap{ { QStringLiteral("type"), QStringLiteral("reasoningDone") },
                                   { QStringLiteral("durationMs"), 1200 } },
                      false, QString() });

    if (wantsApproval) {
        // A dangerous tool pauses the live turn for inline approval: emit an
        // awaiting-approval toolStarted, gate (pause) until resume() fires after
        // the user answers, then the host drives the tool to ok and we close out.
        steps.push_back(
            { 300,
              QVariantMap{ { QStringLiteral("type"), QStringLiteral("toolStarted") },
                           { QStringLiteral("callId"), QStringLiteral("sim-approve") },
                           { QStringLiteral("name"), QStringLiteral("terminal") },
                           { QStringLiteral("tone"), QStringLiteral("terminal") },
                           { QStringLiteral("argsSummary"), QStringLiteral("rm -rf build-test") },
                           { QStringLiteral("needsApproval"), true },
                           { QStringLiteral("allowPermanent"), true },
                           { QStringLiteral("approvalCommand"),
                             QStringLiteral("rm -rf build-test && cmake --preset test") } },
              false, QString(), /*gate=*/true });
        steps.push_back(
            { 250,
              QVariantMap{ { QStringLiteral("type"), QStringLiteral("text") },
                           { QStringLiteral("text"),
                             QStringLiteral("\n\nThe command finished after your approval.\n") } },
              false, QString() });
        steps.push_back({ 200,
                          QVariantMap{ { QStringLiteral("type"), QStringLiteral("flush") } },
                          false, QString() });
        return steps;
    }

    steps.push_back({ 300,
                      QVariantMap{ { QStringLiteral("type"), QStringLiteral("toolStarted") },
                                   { QStringLiteral("callId"), QStringLiteral("sim-term") },
                                   { QStringLiteral("name"), QStringLiteral("terminal") },
                                   { QStringLiteral("tone"), QStringLiteral("terminal") },
                                   { QStringLiteral("argsSummary"),
                                     QStringLiteral("ninja -C build && ctest") },
                                   { QStringLiteral("detailKind"),
                                     QStringLiteral("ansi-stream") } },
                      false, QString() });

    if (wantsError) {
        steps.push_back(
            { 900,
              QVariantMap{ { QStringLiteral("type"), QStringLiteral("toolFinished") },
                           { QStringLiteral("callId"), QStringLiteral("sim-term") },
                           { QStringLiteral("status"), QStringLiteral("error") },
                           { QStringLiteral("durationMs"), 900 },
                           { QStringLiteral("detailKind"), QStringLiteral("ansi-stream") },
                           { QStringLiteral("stderr"),
                             QStringLiteral("\u001b[31merror:\u001b[0m build failed \u2014 1 test "
                                            "did not pass\n") } },
              true, tr("The build failed while completing this turn.") });
        steps.push_back(
            { 250,
              QVariantMap{ { QStringLiteral("type"), QStringLiteral("text") },
                           { QStringLiteral("text"),
                             QStringLiteral("I hit a build failure before I could finish. See the "
                                            "terminal output above for details.\n") } },
              false, QString() });
        steps.push_back({ 200,
                          QVariantMap{ { QStringLiteral("type"), QStringLiteral("flush") } },
                          false, QString() });
        return steps;
    }

    steps.push_back(
        { 1200,
          QVariantMap{ { QStringLiteral("type"), QStringLiteral("toolFinished") },
                       { QStringLiteral("callId"), QStringLiteral("sim-term") },
                       { QStringLiteral("status"), QStringLiteral("ok") },
                       { QStringLiteral("durationMs"), 1200 },
                       { QStringLiteral("detailKind"), QStringLiteral("ansi-stream") },
                       { QStringLiteral("stdout"),
                         QStringLiteral(
                             "\u001b[32mPASS\u001b[0m  88 tests\n\u001b[1mBuild OK\u001b[0m\n") } },
          false, QString() });

    steps.push_back(
        { 300,
          QVariantMap{ { QStringLiteral("type"), QStringLiteral("toolStarted") },
                       { QStringLiteral("callId"), QStringLiteral("sim-search") },
                       { QStringLiteral("name"), QStringLiteral("web_search") },
                       { QStringLiteral("tone"), QStringLiteral("web") },
                       { QStringLiteral("argsSummary"),
                         !shortPrompt.isEmpty() ? shortPrompt : QStringLiteral("qml streaming") },
                       { QStringLiteral("detailKind"), QStringLiteral("search-results") } },
          false, QString() });
    // Deliberately long gap (> kStallMs) so the "still thinking" stall shows.
    steps.push_back(
        { 2600,
          QVariantMap{
              { QStringLiteral("type"), QStringLiteral("toolFinished") },
              { QStringLiteral("callId"), QStringLiteral("sim-search") },
              { QStringLiteral("status"), QStringLiteral("ok") },
              { QStringLiteral("durationMs"), 2600 },
              { QStringLiteral("detailKind"), QStringLiteral("search-results") },
              { QStringLiteral("hits"),
                QVariantList{
                    hit(QStringLiteral("Qt Quick \u2014 Streaming UIs"),
                        QStringLiteral("https://doc.qt.io/qt-6/qtquick-index.html"),
                        QStringLiteral("Build fluid, virtualized views backed by C++ models.")),
                    hit(QStringLiteral("md4qt \u2014 incremental Markdown"),
                        QStringLiteral("https://github.com/igormironchik/md4qt"),
                        QStringLiteral(
                            "A header-only CommonMark parser used for the transcript.")) } } },
          false, QString() });

    steps.push_back(
        { 350,
          QVariantMap{ { QStringLiteral("type"), QStringLiteral("text") },
                       { QStringLiteral("text"),
                         QStringLiteral(
                             "Here's the summary: the build is green (88 tests pass) and ") } },
          false, QString() });
    steps.push_back(
        { 250,
          QVariantMap{ { QStringLiteral("type"), QStringLiteral("text") },
                       { QStringLiteral("text"),
                         QStringLiteral("the references confirm the streaming approach. ") } },
          false, QString() });
    steps.push_back(
        { 250,
          QVariantMap{ { QStringLiteral("type"), QStringLiteral("text") },
                       { QStringLiteral("text"),
                         QStringLiteral(
                             "Let me know if you'd like me to dig into any part further.\n") } },
          false, QString() });
    steps.push_back({ 200,
                      QVariantMap{ { QStringLiteral("type"), QStringLiteral("flush") } },
                      false, QString() });
    return steps;
}
