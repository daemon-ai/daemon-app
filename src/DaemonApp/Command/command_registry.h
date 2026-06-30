// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <functional>
#include <QAbstractListModel>
#include <QString>
#include <QVector>

// A shared, front-end-agnostic catalog of command-palette entries (the GUI Mod+K
// overlay and the TUI Ctrl+P overlay bind the same instance). Each entry is a
// stable id + a human title, grouped, with an optional hint and shortcut. The
// model exposes a `search(query)` that narrows the visible rows (case-insensitive
// substring over title/hint/id/group), so both palettes filter identically.
//
// Activation is intent-only: trigger() emits commandTriggered(id); each front end
// routes the id to its existing actions (theme cycle, new session, open
// settings, model picker, mode toggles, or orchestrator slash commands). The
// daemon adapter can later add/route entries without either UI changing.
//
// GUI-free: Qt6::Core only (QAbstractListModel). The GUI binds it as the `Commands`
// context property; the TUI reads `visibleCommands()` to populate its PaletteDialog.
class CommandRegistry : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    struct Command {
        QString id;
        QString title;
        QString group;
        QString hint;
        QString shortcut;
        // Advisory capability gate: when non-empty, the entry is hidden unless the capability
        // provider grants this (snake_case) capability. Used to hide admin/ownership surfaces from
        // principals that lack them (the node still enforces server-side).
        QString requiresCapability;
    };

    enum Roles {
        IdRole = Qt::UserRole + 1,
        TitleRole,
        GroupRole,
        HintRole,
        ShortcutRole,
    };

    explicit CommandRegistry(QObject* parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex& parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] int count() const { return static_cast<int>(m_filtered.size()); }

    // Narrow the visible rows to entries matching `query` (empty = all).
    Q_INVOKABLE void search(const QString& query);
    // Activate the visible row / a specific id (emits commandTriggered).
    Q_INVOKABLE void trigger(int row);
    Q_INVOKABLE void triggerId(const QString& id);
    [[nodiscard]] Q_INVOKABLE QString idAt(int row) const;

    // The currently visible entries, in order (used by the TUI to build its list).
    [[nodiscard]] QVector<Command> visibleCommands() const;

    // Set the capability predicate that gates capability-tagged entries (e.g. the Users & Access
    // admin page). Until set, gated entries are hidden (fail-closed). Re-applies the current
    // filter.
    void setCapabilityProvider(std::function<bool(const QString&)> provider);

signals:
    void commandTriggered(const QString& id);
    void countChanged();

private:
    void applyFilter(const QString& query);
    [[nodiscard]] bool capabilityAllows(const Command& c) const;

    QVector<Command> m_all;
    QVector<int> m_filtered; // visible row -> index into m_all
    QString m_query;
    std::function<bool(const QString&)> m_capabilityProvider;
};
