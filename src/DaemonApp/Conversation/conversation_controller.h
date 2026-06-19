#pragma once

#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>

namespace persistence {
class IConversationStore;
}

// Drives the open conversation: loads its markdown content and appends user
// input. The Transcript renders `content`; the Composer calls appendUserText().
class ConversationController : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QObject* store READ store WRITE setStore NOTIFY storeChanged)
    Q_PROPERTY(int currentId READ currentId NOTIFY currentChanged)
    Q_PROPERTY(QString content READ content NOTIFY contentChanged)
    Q_PROPERTY(bool hasConversation READ hasConversation NOTIFY currentChanged)

public:
    explicit ConversationController(QObject* parent = nullptr);

    [[nodiscard]] QObject* store() const;
    void setStore(QObject* store);

    [[nodiscard]] int currentId() const { return m_currentId; }
    [[nodiscard]] QString content() const { return m_content; }
    [[nodiscard]] bool hasConversation() const { return m_currentId >= 0; }

    Q_INVOKABLE void open(int conversationId);
    Q_INVOKABLE void appendUserText(const QString& text);
    Q_INVOKABLE int createConversation(int folderId = -1);
    // Persist an in-place edit of the open conversation from the Transcript.
    // Adopts the markdown locally first so the store's changed() -> refresh()
    // does not echo back as a contentChanged() (which would reload the editor and
    // drop the user's cursor). List snippets still refresh via the store signal.
    Q_INVOKABLE void updateContent(const QString& markdown);
    // Archive (move to trash) the open conversation and clear the current
    // selection, "Move to Trash" action.
    Q_INVOKABLE void moveCurrentToTrash();

signals:
    void storeChanged();
    void currentChanged();
    void contentChanged();
    // Emitted when a different conversation becomes current (open/create), so the
    // Transcript reloads. Distinct from contentChanged so the user's own edits do
    // not trigger a reload.
    void conversationChanged();

private:
    void refresh();

    persistence::IConversationStore* m_store = nullptr;
    int m_currentId = -1;
    QString m_content;
};
