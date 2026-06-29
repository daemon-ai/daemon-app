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
    // Create a session owned by the given agent node (empty = unassigned); returns its SessionId.
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

private:
    void refresh();

    persistence::ISessionStore* m_store = nullptr;
    QString m_currentId;
    QString m_content;
};
