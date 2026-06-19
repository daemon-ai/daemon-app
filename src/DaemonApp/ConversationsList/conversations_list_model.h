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
class IChatStore;
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
        FolderNameRole,  // resolved folder name ("" if none)
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

    Q_INVOKABLE void setScope(int nodeType, int nodeId);
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

    persistence::IChatStore* m_store = nullptr;
    domain::ListScope m_scope;
    QString m_search;
    QString m_scopeTitle;
    QList<domain::Conversation> m_all;
    QList<domain::Conversation> m_filtered;
    QHash<int, QString> m_folderNames;
    QHash<int, QPair<QString, QString>> m_tagInfo; // id -> (name, color)
};
