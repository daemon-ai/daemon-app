// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>

namespace persistence {
class ISessionStore;
}

// Drives the open session: loads its markdown content and appends user
// input. The Transcript renders `content`; the Composer calls appendUserText().
class SessionController : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QObject* store READ store WRITE setStore NOTIFY storeChanged)
    Q_PROPERTY(QString currentId READ currentId NOTIFY currentChanged)
    Q_PROPERTY(QString content READ content NOTIFY contentChanged)
    Q_PROPERTY(bool hasSession READ hasSession NOTIFY currentChanged)

public:
    explicit SessionController(QObject* parent = nullptr);

    [[nodiscard]] QObject* store() const;
    void setStore(QObject* store);

    [[nodiscard]] QString currentId() const { return m_currentId; }
    [[nodiscard]] QString content() const { return m_content; }
    [[nodiscard]] bool hasSession() const { return !m_currentId.isEmpty(); }

    Q_INVOKABLE void open(const QString& sessionId);
    Q_INVOKABLE void appendUserText(const QString& text);
    // Node-authoritative session creation: ask the store's backend to CREATE a session bound to
    // `agentId` (the profile ref; empty = the node's active default). Nothing is client-minted —
    // the node mints the id and the store's sessionCreated drives open() + the `created` signal.
    // Returns the id ONLY for a synchronous backend (the in-memory/mock store); the daemon path
    // returns empty and delivers the id via `created`.
    Q_INVOKABLE QString createSession(const QString& agentId = QString());
    // Persist an in-place edit of the open session from the Transcript.
    // Adopts the markdown locally first so the store's changed() -> refresh()
    // does not echo back as a contentChanged() (which would reload the editor and
    // drop the user's cursor). List snippets still refresh via the store signal.
    Q_INVOKABLE void updateContent(const QString& markdown);
    // Archive (move to trash) the open session and clear the current
    // selection, "Move to Trash" action.
    Q_INVOKABLE void moveCurrentToTrash();

signals:
    void storeChanged();
    void currentChanged();
    void contentChanged();
    // Emitted when a different session becomes current (open/create), so the
    // Transcript reloads. Distinct from contentChanged so the user's own edits do
    // not trigger a reload.
    void sessionChanged();
    // A createSession() this controller initiated resolved to the node-minted `sessionId` — the
    // event-driven hook the shell opens/pins the transcript tab on (node-authority: the id comes
    // from the node, never a client mint).
    void created(const QString& sessionId);

private:
    void refresh();
    // Bound to the store's sessionCreated: when THIS controller initiated the create, adopt the
    // node-minted id (open + emit created). Other controllers ignore it (m_pendingCreate == false).
    void onStoreSessionCreated(const QString& sessionId, const QString& profileId);

    persistence::ISessionStore* m_store = nullptr;
    QString m_currentId;
    QString m_content;
    // Set while a createSession() this controller issued is awaiting the store's sessionCreated.
    bool m_pendingCreate = false;
    // The last node-minted id (returned synchronously for the mock store's in-line create).
    QString m_lastCreatedId;
};
