#include "conversation_controller.h"

#include "persistence/ichat_store.h"

ConversationController::ConversationController(QObject* parent)
    : QObject(parent)
{
}

QObject* ConversationController::store() const
{
    return m_store;
}

void ConversationController::setStore(QObject* store)
{
    auto* chatStore = qobject_cast<persistence::IChatStore*>(store);
    if (m_store == chatStore) {
        return;
    }
    if (m_store) {
        m_store->disconnect(this);
    }
    m_store = chatStore;
    if (m_store) {
        connect(m_store, &persistence::IChatStore::changed, this,
                &ConversationController::refresh);
    }
    emit storeChanged();
    refresh();
}

void ConversationController::open(int conversationId)
{
    if (m_currentId == conversationId) {
        return;
    }
    m_currentId = conversationId;
    emit currentChanged();
    refresh();
}

void ConversationController::appendUserText(const QString& text)
{
    const QString trimmed = text.trimmed();
    if (!m_store || m_currentId < 0 || trimmed.isEmpty()) {
        return;
    }
    QString next = m_content;
    if (!next.isEmpty()) {
        next += QStringLiteral("\n\n");
    }
    next += trimmed;
    m_store->setContent(m_currentId, next); // emits changed() -> refresh()
}

int ConversationController::createConversation(int folderId)
{
    if (!m_store) {
        return -1;
    }
    const int id = m_store->createConversation(folderId);
    open(id);
    return id;
}

void ConversationController::refresh()
{
    const QString next = (m_store && m_currentId >= 0) ? m_store->content(m_currentId) : QString{};
    if (next == m_content) {
        return;
    }
    m_content = next;
    emit contentChanged();
}
