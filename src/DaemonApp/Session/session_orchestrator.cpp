// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "session_orchestrator.h"

#include "session_controller.h"
#include "turn_controller.h"

#include <QUuid>

SessionOrchestrator::SessionOrchestrator(QObject* parent)
    : QObject(parent), m_turn(new TurnController(this)), m_todos(new TodoListModel(this)),
      m_subagents(new SubagentModel(this)) {
    // Clear simulator todos (and the settled subagent rows) a beat after the turn
    // ends (the QML used a 1.5s single-shot timer restarted on busy->false).
    m_todoClearTimer.setSingleShot(true);
    m_todoClearTimer.setInterval(1500);
    connect(&m_todoClearTimer, &QTimer::timeout, this, [this] {
        m_todos->clear();
        m_subagents->clear();
    });
    wireTurn();
}

void SessionOrchestrator::wireTurn() {
    // busy mirrors the turn's active state.
    connect(m_turn, &ITurnEngine::activeChanged, this, &SessionOrchestrator::busyChanged);
    // Live subagent rows ride the same event stream the transcript/status bar consume; the model
    // upserts the subagent.* events and ignores the rest.
    connect(m_turn, &ITurnEngine::eventsEmitted, this,
            [this](const QVariantList& events) { m_subagents->applyEvents(events); });
    connect(m_turn, &ITurnEngine::turnFinished, this, [this] { m_todoClearTimer.start(); });
}

void SessionOrchestrator::setTurnEngines(ITurnEngineFactory* factory) {
    if (m_turnEngines == factory) {
        return;
    }
    m_turnEngines = factory;
    if (factory == nullptr) {
        return;
    }
    // Swap the default mock simulator for the factory's engine. QML binds `turn` via re-bindable
    // Connections and null-guards it, so replacing the object + emitting turnChanged is safe.
    if (m_turn != nullptr) {
        m_turn->cancel();
        m_turn->deleteLater();
    }
    m_turn = factory->create(this);
    wireTurn();
    if (m_session != nullptr) {
        m_turn->setSessionId(m_session->currentId());
    }
    emit turnChanged();
    emit busyChanged();
}

void SessionOrchestrator::setSession(SessionController* session) {
    if (m_session == session) {
        return;
    }
    m_session = session;
    if (m_turn != nullptr && m_session != nullptr) {
        m_turn->setSessionId(m_session->currentId());
    }
    emit sessionChanged();
}

void SessionOrchestrator::ensureSessionBound() {
    if (m_session == nullptr) {
        return;
    }
    if (m_session->currentId().isEmpty()) {
        // Daemon mode: the node creates the session lazily on Submit, so the client mints the id.
        // (Mock mode already opened a session id when the tab was created.)
        m_session->open(QStringLiteral("s-") + QUuid::createUuid().toString(QUuid::WithoutBraces));
    }
    if (m_turn != nullptr) {
        m_turn->setSessionId(m_session->currentId());
    }
}

bool SessionOrchestrator::busy() const {
    return m_turn->active();
}

void SessionOrchestrator::submit(const QString& text, const QString& refs) {
    ensureSessionBound();
    if (m_session != nullptr) {
        m_session->appendUserText(refs.isEmpty() ? text : (refs + QStringLiteral("\n") + text));
    }
    // A fresh turn pre-empts a pending clear of the previous turn's todos.
    m_todoClearTimer.stop();
    m_turn->start(text);
    populateSimulatorTodos();
}

void SessionOrchestrator::rerun(const QString& text) {
    // Interrupt-first: a rewind that lands while a turn is live must pre-empt it so
    // the replayed turn is the only one streaming. No appendUserText here - the
    // user block is already in the document (edit) or intentionally unchanged
    // (regenerate).
    ensureSessionBound();
    if (m_turn->active()) {
        m_turn->cancel();
    }
    m_todoClearTimer.stop();
    m_turn->start(text);
    populateSimulatorTodos();
}

void SessionOrchestrator::steer(const QString& text) {
    // CHA-6: a live turn takes the steer over the wire (daemon Submit{Steer}); the mock no-ops the
    // engine call. Keep the local note so the steer is visible in either mode.
    if (m_turn != nullptr && m_turn->active()) {
        m_turn->steer(text);
    }
    if (m_session != nullptr) {
        m_session->appendUserText(tr("(steer) ") + text);
    }
}

void SessionOrchestrator::interrupt() {
    // CHA-6 Stop: ask the engine to interrupt. The daemon engine sends Submit{Interrupt} and keeps
    // draining so the transcript settles on TurnFinished(Interrupted); the mock maps it onto
    // cancel.
    if (m_turn != nullptr) {
        m_turn->interrupt(QString());
    }
}

void SessionOrchestrator::cancel() {
    m_turn->cancel();
}

void SessionOrchestrator::invokeCommand(const QString& command) {
    if (command == QStringLiteral("new")) {
        if (m_session != nullptr) {
            m_session->createSession(QString());
        }
        return;
    }
    if (command == QStringLiteral("retry")) {
        emit retryRequested();
        return;
    }
    if (command == QStringLiteral("edit")) {
        emit editRequested();
        return;
    }
    if (command == QStringLiteral("undo")) {
        emit undoRequested();
        return;
    }
    emit commandRequested(command);
}

void SessionOrchestrator::populateSimulatorTodos() {
    // Drop any settled subagent rows from the previous turn so the status stack
    // starts clean; this turn's subagent.* events repopulate it.
    m_subagents->clear();

    // Simulator content: no real todo backend exists yet, so a canned plan stands
    // in for the turn and is cleared shortly after completion.
    m_todos->setTodos(QVariantList{
        QVariantMap{{QStringLiteral("text"), tr("Inspect the project")},
                    {QStringLiteral("done"), true}},
        QVariantMap{{QStringLiteral("text"), tr("Run the checks")},
                    {QStringLiteral("done"), false}},
        QVariantMap{{QStringLiteral("text"), tr("Summarize the result")},
                    {QStringLiteral("done"), false}},
    });
}
