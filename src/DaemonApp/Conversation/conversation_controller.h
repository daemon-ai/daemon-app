#pragma once

#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>

namespace persistence {
class IChatStore;
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

signals:
    void storeChanged();
    void currentChanged();
    void contentChanged();

private:
    void refresh();

    persistence::IChatStore* m_store = nullptr;
    int m_currentId = -1;
    QString m_content;
};
