// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "session_orchestrator.h"

#include "profiles/iprofile_store.h"
#include "session/isession_settings.h"
#include "session_controller.h"
#include "turn_controller.h"

#include <QDateTime>
#include <QFile>

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
    // #region agent log
    {
        QFile dbg(QStringLiteral("/home/j/experiments/daemon/.cursor/debug-96b7ad.log"));
        if (dbg.open(QIODevice::Append | QIODevice::Text))
            dbg.write(QStringLiteral("{\"sessionId\":\"96b7ad\",\"hypothesisId\":\"TURN-BIND\","
                                     "\"location\":\"session_orchestrator.cpp:setTurnEngines\","
                                     "\"message\":\"turn engine installed from factory\",\"data\":"
                                     "{\"engine\":\"%1\"},\"timestamp\":%2}\n")
                          .arg(QString::fromUtf8(m_turn->metaObject()->className()))
                          .arg(QDateTime::currentMSecsSinceEpoch())
                          .toUtf8());
    }
    // #endregion
    wireTurn();
    if (m_session != nullptr) {
        m_turn->setSessionId(m_session->currentId());
    }
    // A freshly-swapped engine (e.g. the daemon engine replacing the mock simulator) must carry the
    // bound session's profile from the first turn.
    syncTurnProfile();
    emit turnChanged();
    emit busyChanged();
}

QObject* SessionOrchestrator::sessionSettings() const {
    return m_settings;
}

void SessionOrchestrator::setSessionSettings(QObject* settings) {
    auto* typed = qobject_cast<session::ISessionSettings*>(settings);
    if (m_settings == typed) {
        return;
    }
    if (m_settings != nullptr) {
        disconnect(m_settings, nullptr, this, nullptr);
    }
    m_settings = typed;
    if (m_settings != nullptr) {
        // A per-session override changed: re-bind, but only when the shared settings object is
        // currently pointed at THIS orchestrator's session (the popover/overlay set its sessionId
        // to the focused chat before editing). Guarding on the active id keeps a change to another
        // tab's overrides from touching this engine and avoids any cross-tab churn. profileFor()
        // below still reads our own id, so the bind is correct regardless.
        connect(m_settings, &session::ISessionSettings::changed, this, [this] {
            if (m_session != nullptr && m_settings != nullptr &&
                m_settings->sessionId() == m_session->currentId()) {
                syncTurnProfile();
            }
        });
    }
    syncTurnProfile();
    emit sessionSettingsChanged();
}

QObject* SessionOrchestrator::profileStore() const {
    return m_profileStore;
}

void SessionOrchestrator::setProfileStore(QObject* store) {
    auto* typed = qobject_cast<profiles::IProfileStore*>(store);
    if (m_profileStore == typed) {
        return;
    }
    m_profileStore = typed;
    syncTurnProfile();
    emit profileStoreChanged();
}

void SessionOrchestrator::syncTurnProfile() {
    if (m_turn == nullptr || m_settings == nullptr || m_session == nullptr) {
        return;
    }
    // Read the override for our OWN session (never the shared active id), then resolve the stored
    // display name to the canonical profile id the node resolves (empty stays empty = node active).
    const QString selection = m_settings->profileFor(m_session->currentId());
    const QString profileId =
        m_profileStore != nullptr ? m_profileStore->resolveProfileRef(selection) : selection;
    m_turn->setProfile(profileId);
}

void SessionOrchestrator::setSession(SessionController* session) {
    if (m_session == session) {
        return;
    }
    m_session = session;
    if (m_turn != nullptr && m_session != nullptr) {
        m_turn->setSessionId(m_session->currentId());
    }
    syncTurnProfile();
    emit sessionChanged();
}

void SessionOrchestrator::ensureSessionBound() {
    if (m_session == nullptr) {
        return;
    }
    // Node-authority: sessions come ONLY from the node (SessionCreate -> SessionCreated), so the
    // composer is gated on an already-bound node id (SessionController::hasSession). There is no
    // client mint here anymore; a still-empty id means there is no session to submit to (the gate
    // should have prevented this), so just bind what we have.
    if (m_turn != nullptr) {
        m_turn->setSessionId(m_session->currentId());
    }
    syncTurnProfile();
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
