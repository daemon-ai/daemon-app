#pragma once

#include "conversation_controller.h"
#include "todo_list_model.h"
#include "turn_controller.h"

#include <QObject>
#include <QString>
#include <QTimer>
#include <QtQml/qqmlregistration.h>

// The submit pipeline, extracted from Conversation.qml (the per-front-end glue at
// lines 331-357) so both the QML GUI and the TUI drive submit -> turn identically.
// It owns the TurnController (the assistant-turn FSM) and a TodoListModel (the
// status-stack todos), and references the front-end's ConversationController for
// the persisted content. The view only routes input to the invokables and renders
// `turn`/`todos`; it performs no submit orchestration itself.
//
// Slash commands: the shared "new" is handled here (creates a conversation); any
// other command (e.g. "theme"/"distraction") is a front-end UI action and is
// surfaced via commandRequested for the front end to perform.
class ConversationOrchestrator : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(ConversationController* conversation READ conversation WRITE setConversation NOTIFY
                   conversationChanged)
    Q_PROPERTY(TurnController* turn READ turn CONSTANT)
    Q_PROPERTY(TodoListModel* todos READ todos CONSTANT)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)

public:
    explicit ConversationOrchestrator(QObject* parent = nullptr);

    [[nodiscard]] ConversationController* conversation() const { return m_conversation; }
    void setConversation(ConversationController* conversation);

    [[nodiscard]] TurnController* turn() const { return m_turn; }
    [[nodiscard]] TodoListModel* todos() const { return m_todos; }
    [[nodiscard]] bool busy() const;

    // Persist the user's text (attachment refs ride on the front), start the
    // simulated assistant turn, and populate the demo todos for the turn.
    Q_INVOKABLE void submit(const QString& text, const QString& refs = QString());
    // Mid-turn steer: logged as a user note until a real gateway carries steers.
    Q_INVOKABLE void steer(const QString& text);
    // Interrupt the running turn (Stop / Esc).
    Q_INVOKABLE void cancel();
    // Client-side slash command: "new" creates a conversation here; others are
    // emitted via commandRequested for the front end to route.
    Q_INVOKABLE void invokeCommand(const QString& command);

signals:
    void conversationChanged();
    void busyChanged();
    // A non-shared slash command the front end must perform (e.g. open settings,
    // toggle distraction-free).
    void commandRequested(const QString& command);

private:
    void populateDemoTodos();

    ConversationController* m_conversation = nullptr;
    TurnController* m_turn = nullptr;
    TodoListModel* m_todos = nullptr;
    // Clears the demo todos a short beat after the turn settles (replaces the QML
    // todoClearTimer).
    QTimer m_todoClearTimer;
};
