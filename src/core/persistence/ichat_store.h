#pragma once

#include "domain/conversation.h"
#include "domain/folder.h"
#include "domain/sidebar_node.h"
#include "domain/tag.h"

#include <QList>
#include <QObject>
#include <QString>

namespace persistence {

// The data seam. View models depend on this interface, never on a concrete
// store. The in-memory implementation is used now; a SQLite-backed DbManager
// implementation can be dropped in later without touching the UI.
class IChatStore : public QObject {
    Q_OBJECT

public:
    using QObject::QObject;
    ~IChatStore() override = default;

    [[nodiscard]] virtual QList<domain::Folder> folders() const = 0;
    [[nodiscard]] virtual QList<domain::Tag> tags() const = 0;

    // Conversations matching the given sidebar scope (metadata + content).
    [[nodiscard]] virtual QList<domain::Conversation>
    conversations(const domain::ListScope& scope) const = 0;

    [[nodiscard]] virtual int conversationCount(const domain::ListScope& scope) const = 0;

    [[nodiscard]] virtual QString content(int conversationId) const = 0;

    // Mutations. Each emits changed() so models can refresh.
    virtual int createConversation(int folderId) = 0;
    virtual void setContent(int conversationId, const QString& markdown) = 0;
    virtual void setArchived(int conversationId, bool archived) = 0;

signals:
    void changed();
};

} // namespace persistence
