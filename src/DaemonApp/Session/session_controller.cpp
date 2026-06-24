#include "session_controller.h"

#include "persistence/isession_store.h"

#include <QDateTime>

SessionController::SessionController(QObject* parent)
    : QObject(parent)
{
}

QObject* SessionController::store() const
{
    return m_store;
}

void SessionController::setStore(QObject* store)
{
    auto* sessionStore = qobject_cast<persistence::ISessionStore*>(store);
    if (m_store == sessionStore) {
        return;
    }
    if (m_store) {
        m_store->disconnect(this);
    }
    m_store = sessionStore;
    if (m_store) {
        connect(m_store, &persistence::ISessionStore::changed, this,
                &SessionController::refresh);
    }
    emit storeChanged();
    refresh();
}

void SessionController::open(int sessionId)
{
    if (m_currentId == sessionId) {
        return;
    }
    m_currentId = sessionId;
    emit currentChanged();
    refresh();
    emit sessionChanged();
}

void SessionController::appendUserText(const QString& text)
{
    const QString trimmed = text.trimmed();
    if (!m_store || m_currentId < 0 || trimmed.isEmpty()) {
        return;
    }
    // Prefix a message boundary marker (role layer, Strategy C) so the persisted
    // markdown carries the user turn; the BlockEditor parser consumes the marker
    // and tags the text as a user message. A "u<epochMs>" id stays distinct from
    // the runtime's "m<n>" assistant ids, so the two never collide.
    const QString id = QStringLiteral("u%1").arg(QDateTime::currentMSecsSinceEpoch());
    const QString marker =
        QStringLiteral("```msg\n{\"id\":\"%1\",\"role\":\"user\"}\n```").arg(id);

    QString next = m_content;
    if (!next.isEmpty()) {
        next += QStringLiteral("\n\n");
    }
    next += marker + QStringLiteral("\n\n") + trimmed;
    m_store->setContent(m_currentId, next); // emits changed() -> refresh()
}

void SessionController::updateContent(const QString& markdown)
{
    if (!m_store || m_currentId < 0 || markdown == m_content) {
        return;
    }
    // Adopt locally first so the store's changed() -> refresh() is a no-op for
    // this controller (no contentChanged), while list models still refresh.
    m_content = markdown;
    m_store->setContent(m_currentId, markdown);
}

void SessionController::moveCurrentToTrash()
{
    if (!m_store || m_currentId < 0) {
        return;
    }
    const int archivedId = m_currentId;
    // Clear the current selection first so the editor falls back to the empty
    // state; the store's changed() refreshes the lists (the session moves
    // into the Trash scope).
    m_currentId = -1;
    m_content.clear();
    emit currentChanged();
    emit sessionChanged();
    m_store->setArchived(archivedId, true); // emits changed() -> refresh()
}

int SessionController::createSession(const QString& agentId)
{
    if (!m_store) {
        return -1;
    }
    const int id = m_store->createSession(domain::UnitId(agentId));
    open(id);
    return id;
}

void SessionController::refresh()
{
    const QString next = (m_store && m_currentId >= 0) ? m_store->content(m_currentId) : QString{};
    if (next == m_content) {
        return;
    }
    m_content = next;
    emit contentChanged();
}
