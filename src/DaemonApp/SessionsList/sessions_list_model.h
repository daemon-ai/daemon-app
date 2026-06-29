// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "domain/ids.h"
#include "domain/session.h"
#include "domain/sidebar_node.h"

#include <QAbstractListModel>
#include <QHash>
#include <QList>
#include <QPair>
#include <QString>
#include <QStringList>
#include <QtQml/qqmlregistration.h>

namespace persistence {
class ISessionStore;
}

// The sessions for the current sidebar scope, filtered by `search`.
class SessionsListModel : public QAbstractListModel {
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
        UnitNameRole,  // resolved owning unit name ("" if none)
        UnitKindRole,  // domain::UnitKind of the owning unit (cosmetic)
        TagNamesRole,  // QStringList of tag names
        TagColorsRole, // QStringList of tag colors, parallel to TagNamesRole
        CurrentRole,   // true for the currently-selected row (identity match)
        PinnedRole,    // true when the session is pinned (floats to top)
    };

    explicit SessionsListModel(QObject* parent = nullptr);

    [[nodiscard]] QObject* store() const;
    void setStore(QObject* store);

    [[nodiscard]] QString search() const { return m_search; }
    void setSearch(const QString& search);

    [[nodiscard]] QString scopeTitle() const { return m_scopeTitle; }
    [[nodiscard]] int count() const { return static_cast<int>(m_filtered.size()); }

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // `tagId` is the tag id (Tag scope); `unitId` is the unit id (Unit scope).
    Q_INVOKABLE void setScope(int nodeType, int tagId, const QString& unitId);
    // The authoritative string SessionId at `row` ("" if out of range).
    Q_INVOKABLE QString idAt(int row) const;

    // Identity-based selection (mirrors SidebarModel): the selection is stored by
    // session id so it survives search/scope rebuilds; the highlight is
    // driven by CurrentRole rather than a view-local row index.
    // Select a row: records the selection by id, emits selectionChanged, and
    // repaints the highlight.
    Q_INVOKABLE void activate(int row);
    Q_INVOKABLE void selectNext();     // next row (clamped)
    Q_INVOKABLE void selectPrevious(); // previous row (clamped)
    // Row index of the current selection (-1 if none / filtered out).
    [[nodiscard]] Q_INVOKABLE int currentRow() const;

signals:
    void storeChanged();
    void searchChanged();
    void scopeChanged();
    void countChanged();
    // Emitted whenever the current selection changes (the string SessionId, "" when cleared).
    void selectionChanged(const QString& sessionId);

private:
    void reload();
    void applyFilter();
    void rebuildLookups();
    [[nodiscard]] QString computeScopeTitle() const;
    void setCurrentId(const QString& id); // records id + emits + repaints
    void emitCurrentChanged();            // dataChanged(CurrentRole) for all rows

    persistence::ISessionStore* m_store = nullptr;
    domain::ListScope m_scope;
    QString m_search;
    QString m_scopeTitle;
    QList<domain::Session> m_all;
    QList<domain::Session> m_filtered;
    QHash<QString, QPair<QString, int>> m_unitInfo; // unit id -> (name, kind)
    QHash<int, QPair<QString, QString>> m_tagInfo;  // tag id -> (name, color)
    QString m_currentId; // selected session id (identity-stable), "" = none
};
