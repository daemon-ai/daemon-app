#include "conversation_orchestrator.h"

#include "conversation_controller.h"
#include "turn_controller.h"

ConversationOrchestrator::ConversationOrchestrator(QObject* parent)
    : QObject(parent)
    , m_turn(new TurnController(this))
    , m_todos(new TodoListModel(this))
    , m_subagents(new SubagentModel(this))
{
    // busy mirrors the turn's active state.
    connect(m_turn, &TurnController::activeChanged, this, &ConversationOrchestrator::busyChanged);

    // Live subagent rows ride the same event stream the transcript/status bar
    // consume; the model upserts the subagent.* events and ignores the rest.
    connect(m_turn, &TurnController::eventsEmitted, this,
            [this](const QVariantList& events) { m_subagents->applyEvents(events); });

    // Clear the demo todos (and the settled subagent rows) a beat after the turn
    // ends (the QML used a 1.5s single-shot timer restarted on busy->false).
    m_todoClearTimer.setSingleShot(true);
    m_todoClearTimer.setInterval(1500);
    connect(&m_todoClearTimer, &QTimer::timeout, this, [this] {
        m_todos->clear();
        m_subagents->clear();
    });
    connect(m_turn, &TurnController::turnFinished, this, [this] { m_todoClearTimer.start(); });
}

void ConversationOrchestrator::setConversation(ConversationController* conversation)
{
    if (m_conversation == conversation) {
        return;
    }
    m_conversation = conversation;
    emit conversationChanged();
}

bool ConversationOrchestrator::busy() const
{
    return m_turn->active();
}

void ConversationOrchestrator::submit(const QString& text, const QString& refs)
{
    if (m_conversation != nullptr) {
        m_conversation->appendUserText(refs.isEmpty() ? text
                                                       : (refs + QStringLiteral("\n") + text));
    }
    // A fresh turn pre-empts a pending clear of the previous turn's todos.
    m_todoClearTimer.stop();
    m_turn->start(text);
    populateDemoTodos();
}

void ConversationOrchestrator::rerun(const QString& text)
{
    // Interrupt-first: a rewind that lands while a turn is live must pre-empt it so
    // the replayed turn is the only one streaming. No appendUserText here - the
    // user block is already in the document (edit) or intentionally unchanged
    // (regenerate).
    if (m_turn->active()) {
        m_turn->cancel();
    }
    m_todoClearTimer.stop();
    m_turn->start(text);
    populateDemoTodos();
}

void ConversationOrchestrator::steer(const QString& text)
{
    if (m_conversation != nullptr) {
        m_conversation->appendUserText(tr("(steer) ") + text);
    }
}

void ConversationOrchestrator::cancel()
{
    m_turn->cancel();
}

void ConversationOrchestrator::invokeCommand(const QString& command)
{
    if (command == QStringLiteral("new")) {
        if (m_conversation != nullptr) {
            m_conversation->createSession(QString());
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

void ConversationOrchestrator::populateDemoTodos()
{
    // Drop any settled subagent rows from the previous turn so the status stack
    // starts clean; this turn's subagent.* events repopulate it.
    m_subagents->clear();

    // Demo content: no real todo backend exists yet, so a canned plan stands in
    // for the turn (mirrors the former Conversation.qml seam).
    m_todos->setTodos(QVariantList{
        QVariantMap{ { QStringLiteral("text"), tr("Inspect the project") },
                     { QStringLiteral("done"), true } },
        QVariantMap{ { QStringLiteral("text"), tr("Run the checks") },
                     { QStringLiteral("done"), false } },
        QVariantMap{ { QStringLiteral("text"), tr("Summarize the result") },
                     { QStringLiteral("done"), false } },
    });
}
