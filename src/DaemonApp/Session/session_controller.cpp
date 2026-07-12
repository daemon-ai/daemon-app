// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "session_controller.h"

#include "persistence/isession_store.h"

#include <QDateTime>
#include <QFile>

SessionController::SessionController(QObject* parent) : QObject(parent) {}

QObject* SessionController::store() const {
    return m_store;
}

void SessionController::setStore(QObject* store) {
    auto* sessionStore = qobject_cast<persistence::ISessionStore*>(store);
    if (m_store == sessionStore) {
        return;
    }
    if (m_store) {
        m_store->disconnect(this);
    }
    m_store = sessionStore;
    if (m_store) {
        connect(m_store, &persistence::ISessionStore::changed, this, &SessionController::refresh);
        connect(m_store, &persistence::ISessionStore::sessionCreated, this,
                &SessionController::onStoreSessionCreated);
    }
    emit storeChanged();
    refresh();
}

void SessionController::open(const QString& sessionId) {
    if (m_currentId == sessionId) {
        return;
    }
    m_currentId = sessionId;
    emit currentChanged();
    refresh();
    emit sessionChanged();
}

void SessionController::appendUserText(const QString& text) {
    const QString trimmed = text.trimmed();
    if (!m_store || m_currentId.isEmpty() || trimmed.isEmpty()) {
        return;
    }
    // Prefix a message boundary marker (role layer, Strategy C) so the rendered
    // markdown carries the user turn; the BlockEditor parser consumes the marker
    // and tags the text as a user message. A "u<epochMs>" id stays distinct from
    // the runtime's "m<n>" assistant ids, so the two never collide.
    //
    // AD: PRESENTATION-LOCAL adopt only (§8.5 — client-local view state). The store write died
    // with the legacy stores: transcript persistence is the ENGINE's (daemon: the node echoes
    // the user message → the sink persists the block → content() re-projects the authoritative
    // row). In mock the simulator's live turn is editor-local by design (the seeded transcripts
    // render from the mirror; an ad-hoc demo turn is not durable by design).
    const QString id = QStringLiteral("u%1").arg(QDateTime::currentMSecsSinceEpoch());
    const QString marker = QStringLiteral("```msg\n{\"id\":\"%1\",\"role\":\"user\"}\n```").arg(id);

    if (!m_content.isEmpty()) {
        m_content += QStringLiteral("\n\n");
    }
    m_content += marker + QStringLiteral("\n\n") + trimmed;
    emit contentChanged();
}

void SessionController::updateContent(const QString& markdown) {
    if (!m_store || m_currentId.isEmpty() || markdown == m_content) {
        return;
    }
    // Adopt locally first so the store's changed() -> refresh() is a no-op for
    // this controller (no contentChanged), while list models still refresh.
    m_content = markdown;
    m_store->setContent(m_currentId, markdown);
}

void SessionController::moveCurrentToTrash() {
    if (!m_store || m_currentId.isEmpty()) {
        return;
    }
    const QString archivedId = m_currentId;
    // Clear the current selection first so the editor falls back to the empty
    // state; the store's changed() refreshes the lists (the session moves
    // into the Trash scope).
    m_currentId.clear();
    m_content.clear();
    emit currentChanged();
    emit sessionChanged();
    m_store->setArchived(archivedId, true); // emits changed() -> refresh()
}

QString SessionController::createSession(const QString& agentId) {
    if (!m_store) {
        return {};
    }
    // Node-authoritative: nothing is client-minted. Ask the backend to CREATE the session; the
    // node mints the id and the store's sessionCreated -> onStoreSessionCreated adopts it (open +
    // `created`). A synchronous backend (the mock store) emits sessionCreated in-line during the
    // call below, so m_lastCreatedId is set before we return (the mock's callers still get an id);
    // the daemon path returns empty and delivers the id via `created`.
    m_pendingCreate = true;
    m_lastCreatedId.clear();
    m_store->requestNewSession(agentId);
    return m_lastCreatedId;
}

void SessionController::onStoreSessionCreated(const QString& sessionId,
                                              const QString& /*profileId*/) {
    // Only the controller that initiated the create adopts the id; other controllers sharing the
    // store ignore the broadcast.
    if (!m_pendingCreate || sessionId.isEmpty()) {
        return;
    }
    m_pendingCreate = false;
    m_lastCreatedId = sessionId;
    open(sessionId);
    emit created(sessionId);
}

void SessionController::refresh() {
    const QString next =
        (m_store && !m_currentId.isEmpty()) ? m_store->content(m_currentId) : QString{};
    if (next == m_content) {
        return;
    }
    m_content = next;
    emit contentChanged();
}
