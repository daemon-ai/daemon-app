// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "domain/participant.h"

#include <QAbstractListModel>
#include <QList>
#include <QString>
#include <QtQml/qqmlregistration.h>

namespace persistence {
class ISessionStore;
}

namespace participants {

// Flat list model for the right sidebar's "Participants" section, mirroring the left sidebar's Tags
// section: a single collapsible separator header ("Participants") followed by one row per chat
// participant, each carrying a colored presence dot. The data is the session store's participant
// roster (copied from the DaemonNet seed). `store` is bound from QML to the shared session store;
// the TUI binds the same C++ model directly. Folding the header hides the participant rows.
class ParticipantsModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QObject* store READ store WRITE setStore NOTIFY storeChanged)

public:
    enum Role {
        LabelRole = Qt::UserRole + 1,
        IsSeparatorRole, // true for the section header row
        HasChildrenRole, // true for the header (drives the twistie)
        ExpandedRole,    // section fold state (header row)
        ColorRole,       // participant dot color (#rrggbb); empty for the header
        PresenceRole,    // PresencePrimitive ("available" -> green); empty for the header
        IsAgentRole,     // true for an agent participant, false for a human contact
    };

    explicit ParticipantsModel(QObject* parent = nullptr);

    [[nodiscard]] QObject* store() const;
    void setStore(QObject* store);

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Fold/unfold the section (no-op unless `row` is the header). Hides/shows the participant rows.
    Q_INVOKABLE void toggleExpand(int row);

signals:
    void storeChanged();

private:
    struct Row {
        QString label;
        bool separator = false;
        bool hasChildren = false;
        QString color;
        QString presence;
        bool isAgent = false;
    };

    void rebuild();

    persistence::ISessionStore* m_store = nullptr;
    QList<Row> m_rows;
    bool m_expanded = true;
};

} // namespace participants
