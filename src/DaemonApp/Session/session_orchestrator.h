#pragma once

#include "session_controller.h"
#include "subagent_model.h"
#include "todo_list_model.h"
#include "turn_controller.h"

#include <QObject>
#include <QString>
#include <QTimer>
#include <QtQml/qqmlregistration.h>

// The submit pipeline, extracted from Session.qml (the per-front-end glue at
// lines 331-357) so both the QML GUI and the TUI drive submit -> turn identically.
// It owns the TurnController (the assistant-turn FSM) and a TodoListModel (the
// status-stack todos), and references the front-end's SessionController for
// the persisted content. The view only routes input to the invokables and renders
// `turn`/`todos`; it performs no submit orchestration itself.
//
// Slash commands: the shared "new" is handled here (creates a session); any
// other command (e.g. "theme"/"distraction") is a front-end UI action and is
// surfaced via commandRequested for the front end to perform.
class SessionOrchestrator : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(SessionController* session READ session WRITE setSession NOTIFY
                   sessionChanged)
    Q_PROPERTY(TurnController* turn READ turn CONSTANT)
    Q_PROPERTY(TodoListModel* todos READ todos CONSTANT)
    Q_PROPERTY(SubagentModel* subagents READ subagents CONSTANT)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)

public:
    explicit SessionOrchestrator(QObject* parent = nullptr);

    [[nodiscard]] SessionController* session() const { return m_session; }
    void setSession(SessionController* session);

    [[nodiscard]] TurnController* turn() const { return m_turn; }
    [[nodiscard]] TodoListModel* todos() const { return m_todos; }
    [[nodiscard]] SubagentModel* subagents() const { return m_subagents; }
    [[nodiscard]] bool busy() const;

    // Persist the user's text (attachment refs ride on the front), start the
    // simulated assistant turn, and populate the demo todos for the turn.
    Q_INVOKABLE void submit(const QString& text, const QString& refs = QString());
    // Re-run the assistant turn for `text` WITHOUT appending a new user message:
    // the regenerate path (no new prompt) and the GUI edit path (the edited user
    // block already exists in the document). Interrupts a live turn first. This is
    // the single turn-execution seam every rewind path funnels through; swapping
    // the scripted TurnController for a daemon NodeApi adapter happens here alone.
    Q_INVOKABLE void rerun(const QString& text);
    // Mid-turn steer: logged as a user note until a real gateway carries steers.
    Q_INVOKABLE void steer(const QString& text);
    // Interrupt the running turn (Stop / Esc).
    Q_INVOKABLE void cancel();
    // Client-side slash command: "new" creates a session here; the rewind
    // commands ("retry"/"edit"/"undo") surface as the rewind*Requested signals for
    // the front end to apply against its document owner (EditorController / the TUI
    // DocumentStore); all others are emitted via commandRequested.
    Q_INVOKABLE void invokeCommand(const QString& command);

signals:
    void sessionChanged();
    void busyChanged();
    // A non-shared slash command the front end must perform (e.g. open settings,
    // toggle distraction-free).
    void commandRequested(const QString& command);
    // Rewind slash commands. The front end owns the document, so it resolves the
    // last user message and truncates via the shared DocumentStore primitives,
    // then re-runs through rerun()/submit(): retry re-runs the last user message
    // with its own text; edit truncates it and seeds the composer with its text
    // (the "/edit prompt into composer" path); undo drops the last exchange.
    void retryRequested();
    void editRequested();
    void undoRequested();

private:
    void populateDemoTodos();

    SessionController* m_session = nullptr;
    TurnController* m_turn = nullptr;
    TodoListModel* m_todos = nullptr;
    SubagentModel* m_subagents = nullptr;
    // Clears the demo todos a short beat after the turn settles (replaces the QML
    // todoClearTimer).
    QTimer m_todoClearTimer;
};
