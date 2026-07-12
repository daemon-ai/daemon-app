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

namespace mirror {
class Store;
} // namespace mirror
namespace transports {
class ITransportRegistry;
} // namespace transports

// [integrations wire v38] The integrations tree (work package A2 → AD 1a.3): the sidebar surface
// where each configured account (transport instance) is a ROOT node whose PROTOCOL governs its
// subtree. The node is authoritative; the client hard-codes nothing per protocol — each account's
// adapter capabilities decide which sections appear and the node's conversation `kind`/`parent`
// decide the hierarchy (spaces/servers -> rooms -> DMs).
//
// AD (1a.3): this model composes from the MIRROR — `transport_accounts` (accounts), `adapters`
// (capabilities + directory), `conversations` (kind/parent hierarchy) and `persons` ×
// `person_endpoints` (the per-account Persons section) — identical in daemon (ingestor-fed) and
// mock (scenario-seeded) modes (§9). The legacy ITransportRegistry/IPersonsService READ
// composition is gone; the registry stays bound ONLY as the account VERB sink
// (connect/enable — null in mock, where the verbs no-op).
//
// GUI + TUI render the SAME model: the GUI binds a flattened ListView (IntegrationsTree.qml), the
// TUI a TreeListView via a DisplayRoleAdapter. All structural logic lives here.
class IntegrationsTreeModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    // The mirror composition root (`Mirror` context property; live in BOTH modes). Opaque
    // QObject* so QML can assign it without a registered pointer metatype; the .cpp upcasts.
    Q_PROPERTY(QObject* mirror READ mirror WRITE setMirror NOTIFY mirrorChanged)
    // The transport-adapter VERB sink (connectAccount/setEnabled — bound from QML to the shared
    // `Transports` context property; null in mock, the verbs then no-op). NEVER read from.
    Q_PROPERTY(QObject* registry READ registry WRITE setRegistry NOTIFY registryChanged)
    // True when at least one account/space/section is currently expanded; drives the header's
    // collapse-all/expand-all toggle. Refreshed on rebuild.
    Q_PROPERTY(bool anyExpanded READ anyExpanded NOTIFY treeChanged)

public:
    enum Role {
        LabelRole = Qt::UserRole + 1,
        DepthRole,     // tree depth (0 = account root)
        RowKindRole,   // "account"|"section"|"space"|"conversation"|"person"|"action"
        ConvTypeRole,  // conversation kind (mirror vocabulary):
                       // "channel"|"group_dm"|"dm"|"thread"|"space" (else "")
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

    [[nodiscard]] QObject* mirror() const;
    void setMirror(QObject* mirror);
    [[nodiscard]] QObject* registry() const;
    void setRegistry(QObject* registry);
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
    void mirrorChanged();
    void registryChanged();
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
    void onCommitted();
    // Append one account and its capability-governed subtree. `adapter` is the matching mirror
    // adapters row (capabilities/directory as a QVariantMap), empty if the family is unknown.
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

    // --- mirror projections (deterministically ordered) ---------------------------------------
    // The configured accounts (transport-sorted), label overlaid onto displayName.
    [[nodiscard]] QList<QVariantMap> accountRows() const;
    // The adapters row for a family ({capabilities{...}, directory}), empty if unknown.
    [[nodiscard]] QVariantMap adapterFor(const QString& family) const;
    // A transport's conversations (id-sorted): {id, kind, title, parent}.
    [[nodiscard]] QList<QVariantMap> conversationRows(const QString& transport) const;
    // The persons reachable on `transport` (person-id-sorted): {id, label, presence} — the
    // persons × person_endpoints join (alias wins, else the endpoint's display name).
    [[nodiscard]] QList<QVariantMap> personRows(const QString& transport) const;

    QObject* m_mirrorObject = nullptr;
    mirror::Store* m_mirror = nullptr; // the upcast m_mirrorObject (null when unset)
    quint64 m_watermark = 0;
    transports::ITransportRegistry* m_registry = nullptr;
    QList<Row> m_rows;
    // Accounts / spaces / sections are expanded by default; only explicitly-collapsed fold keys
    // live here.
    QSet<QString> m_collapsed;

    // Selection, stored by identity (fold key or transport+conversation) so it survives rebuilds.
    QString m_selKey;
};
