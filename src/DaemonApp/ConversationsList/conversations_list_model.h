#pragma once

#include "domain/conversation.h"
#include "domain/sidebar_node.h"

#include <QAbstractListModel>
#include <QHash>
#include <QList>
#include <QPair>
#include <QString>
#include <QStringList>
#include <QtQml/qqmlregistration.h>

namespace persistence {
class IConversationStore;
}

// The conversations for the current sidebar scope, filtered by `search`.
class ConversationsListModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QObject* store READ store WRITE setStore NOTIFY storeChanged)
    Q_PROPERTY(QString search READ search WRITE setSearch NOTIFY searchChanged)
    Q_PROPERTY(QString scopeTitle READ scopeTitle NOTIFY scopeChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Role {
        IdRole = Qt::UserRole + 1,
        TitleRole,
        SnippetRole,
        ModifiedRole,
        AgentNameRole,   // resolved owning agent-node name ("" if none)
        AgentKindRole,   // domain::AgentNodeKind of the owning node (cosmetic)
        TagNamesRole,    // QStringList of tag names
        TagColorsRole,   // QStringList of tag colors, parallel to TagNamesRole
    };

    explicit ConversationsListModel(QObject* parent = nullptr);

    [[nodiscard]] QObject* store() const;
    void setStore(QObject* store);

    [[nodiscard]] QString search() const { return m_search; }
    void setSearch(const QString& search);

    [[nodiscard]] QString scopeTitle() const { return m_scopeTitle; }
    [[nodiscard]] int count() const { return static_cast<int>(m_filtered.size()); }

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // `id` is the tag id (Tag scope); `nodeId` is the agent node id (Node scope).
    Q_INVOKABLE void setScope(int nodeType, int id, const QString& nodeId);
    Q_INVOKABLE int idAt(int row) const;

signals:
    void storeChanged();
    void searchChanged();
    void scopeChanged();
    void countChanged();

private:
    void reload();
    void applyFilter();
    void rebuildLookups();
    [[nodiscard]] QString computeScopeTitle() const;

    persistence::IConversationStore* m_store = nullptr;
    domain::ListScope m_scope;
    QString m_search;
    QString m_scopeTitle;
    QList<domain::Conversation> m_all;
    QList<domain::Conversation> m_filtered;
    QHash<QString, QPair<QString, int>> m_nodeInfo; // agent id -> (name, kind)
    QHash<int, QPair<QString, QString>> m_tagInfo;   // tag id -> (name, color)
};
