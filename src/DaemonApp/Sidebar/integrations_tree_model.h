// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QList>
#include <QSet>
#include <QString>
#include <QtQml/qqmlregistration.h>
#include <QVariantList>
#include <QVariantMap>

namespace transports {
class ITransportRegistry;
class IPersonsService;
} // namespace transports

// [integrations wire v38] The integrations tree (work package A2): the sidebar surface where each
// configured account (transport instance) is a ROOT node whose PROTOCOL governs its subtree. The
// node is authoritative; the client hard-codes nothing per protocol — each account's adapter
// capabilities decide which sections appear and the node's ConvList `kind`/`parent` decide the
// hierarchy (spaces/servers -> rooms -> DMs).
//
// Architecture decision (A1 deferred this to A2): this model composes DIRECTLY from
// ITransportRegistry (instances + availableAdapters + conversations) and IPersonsService (persons).
// It is the SOLE integrations tree — the legacy fleet-cross-link sidebar projection was deleted in
// M3. The authoritative live data the daemon adapters fill lives in the registry/persons seams;
// composing directly keeps the protocol-governed rendering in one shared, testable C++ view-model
// (thin-client rule).
//
// GUI + TUI render the SAME model: the GUI binds a flattened ListView (IntegrationsTree.qml), the
// TUI a TreeListView via a DisplayRoleAdapter. All structural logic lives here.
class IntegrationsTreeModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    // The transport-adapter registry (accounts + adapters + conversations). Bound from QML to the
    // shared `Transports` context property. Null => an empty tree.
    Q_PROPERTY(QObject* registry READ registry WRITE setRegistry NOTIFY registryChanged)
    // The cross-transport person registry (endpoints per transport). Bound from QML to the shared
    // `Persons` context property. Null => no Persons sections.
    Q_PROPERTY(QObject* persons READ persons WRITE setPersons NOTIFY personsChanged)
    // True when at least one account/space/section is currently expanded; drives the header's
    // collapse-all/expand-all toggle. Refreshed on rebuild.
    Q_PROPERTY(bool anyExpanded READ anyExpanded NOTIFY treeChanged)

public:
    enum Role {
        LabelRole = Qt::UserRole + 1,
        DepthRole,     // tree depth (0 = account root)
        RowKindRole,   // "account"|"section"|"space"|"conversation"|"person"|"action"
        ConvTypeRole,  // conversation kind: "channel"|"groupdm"|"dm"|"thread"|"space" (else "")
        SectionRole,   // section id for section rows: "persons"|"channels"|"dms"|"browse" (else "")
        TransportRole, // owning transport instance id (account + descendants)
        ConversationRole, // conversation id for conversation/space rows (else "")
        FamilyRole,       // adapter family for account rows (else "")
        PersonIdRole,     // person id for person rows (else "")
        EnabledRole,      // account rows: the node-persisted desired-enabled state
        ConnectionRole,   // account rows: coarse connection token (else "")
        PresenceRole,     // account/person rows: PresencePrimitive (else "")
        CountRole,        // occupant / child badge (-1 = none)
        HasChildrenRole,
        ExpandedRole,
        SelectableRole,
        CurrentRole, // true for the currently-selected row (identity match)
    };
    Q_ENUM(Role)

    explicit IntegrationsTreeModel(QObject* parent = nullptr);

    [[nodiscard]] QObject* registry() const;
    void setRegistry(QObject* registry);
    [[nodiscard]] QObject* persons() const;
    void setPersons(QObject* persons);
    [[nodiscard]] bool anyExpanded() const;

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Select a row: records the selection by identity, routes the row's activation intent, and
    // repaints the highlight. A conversation row emits conversationActivated (routed through the
    // existing activation seam by A4); an account row emits accountActivated; a Browse/Join action
    // emits browseRequested; a person row emits personActivated. No-op for section headers.
    Q_INVOKABLE void activate(int row);
    // Expand/collapse an account / space / section / group row (no-op for leaves).
    Q_INVOKABLE void toggleExpand(int row);
    Q_INVOKABLE void expandAll();
    Q_INVOKABLE void collapseAll();

    // Account context intents (the schema-driven add/edit/remove flows are handled by
    // AccountManagementController, which QML/TUI drive from these signals). connect/enable are
    // ControlApi-level and go straight to the registry.
    Q_INVOKABLE void requestAddAccount();
    Q_INVOKABLE void requestEditAccount(const QString& transport);
    Q_INVOKABLE void requestRemoveAccount(const QString& transport);
    Q_INVOKABLE void connectAccount(const QString& transport);
    Q_INVOKABLE void setAccountEnabled(const QString& transport, bool enabled);

    // Row index of the current selection (-1 if hidden/none).
    [[nodiscard]] Q_INVOKABLE int currentRow() const;

signals:
    void registryChanged();
    void personsChanged();
    void treeChanged();
    // A conversation node (room/channel/DM) was activated: open its native chat tab. Routed through
    // the existing activation seam (A4 owns what activation DOES).
    void conversationActivated(const QString& transport, const QString& conversation);
    // An account root was activated (highlight / scope the surface to this account).
    void accountActivated(const QString& transport);
    // A person node was activated (a cross-transport identity; lazy-DM open is A4's domain).
    void personActivated(const QString& personId);
    // The "Browse/Join" action on an account was activated (open the directory / join flow).
    void browseRequested(const QString& transport);
    // Schema-driven account management intents (handled by AccountManagementController).
    void addAccountRequested();
    void editAccountRequested(const QString& transport);
    void removeAccountRequested(const QString& transport);

private:
    struct Row {
        QString label;
        int depth = 0;
        QString rowKind;      // account|section|space|conversation|person|action
        QString convType;     // conversation kind (else "")
        QString section;      // section id (else "")
        QString transport;    // owning transport instance
        QString conversation; // conversation id (space/conversation rows)
        QString family;       // adapter family (account rows)
        QString personId;     // person id (person rows)
        bool enabled = true;
        QString connection; // coarse connection token (account rows)
        QString presence;   // PresencePrimitive (else "")
        int count = -1;
        bool hasChildren = false;
        bool expanded = false;
        bool selectable = true;
        // Stable fold key (a disjoint id namespace so accounts/spaces/sections share m_collapsed
        // safely). Empty for leaves.
        QString foldKey;
        // Stable selection identity (survives rebuilds). Empty for non-selectable section rows.
        QString selId;
    };

    void rebuild();
    // Append one account and its capability-governed subtree. `adapter` is the matching
    // availableAdapters() row (capabilities/ops), empty if the family is unknown.
    void appendAccount(const QVariantMap& instance, const QVariantMap& adapter);
    // Append a space node + (recursively) its child conversations. `byParent` maps a parent
    // conversation id to its child conversation rows. Guarded against cycles / unknown parents.
    void appendSpace(const QString& transport, const QVariantMap& space,
                     const QHash<QString, QList<QVariantMap>>& byParent, QSet<QString>& visited,
                     int depth);
    void appendConversation(const QString& transport, const QVariantMap& conv, int depth);
    void pushSection(const QString& transport, const QString& sectionId, const QString& label,
                     int depth);

    [[nodiscard]] bool isExpanded(const QString& foldKey) const;
    // A row is hidden when any ancestor fold key is collapsed.
    void collectFoldKeys(QSet<QString>& out) const;
    void setSelectionFromRow(int row);
    [[nodiscard]] bool rowIsCurrent(const Row& r) const;
    void emitCurrentChanged();

    // Look up the availableAdapters() row for a family (capabilities + ops), empty if unknown.
    [[nodiscard]] QVariantMap adapterFor(const QString& family) const;

    transports::ITransportRegistry* m_registry = nullptr;
    transports::IPersonsService* m_persons = nullptr;
    QList<Row> m_rows;
    // Accounts / spaces / sections are expanded by default; only explicitly-collapsed fold keys
    // live here.
    QSet<QString> m_collapsed;

    // Selection, stored by identity (fold key or transport+conversation) so it survives rebuilds.
    QString m_selKey;
};
