// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "domain/sidebar_node.h"
#include "domain/unit_node.h"

#include <QAbstractListModel>
#include <QList>
#include <QSet>
#include <QString>
#include <QtQml/qqmlregistration.h>

namespace persistence {
class ISessionStore;
}

namespace profiles {
class IProfileStore;
}

namespace domain {
struct UnitNode;
}

// Flattened supervision-tree sidebar (VSCode AsyncDataTree style): a single flat
// list of rows where the unit tree is rendered as indented rows. Rows are: All
// Sessions / Archived / a "Fleet" header + the recursively flattened unit
// tree (only descending into expanded units) / a "Tags" header + tag rows.
//
// The flatten is uniformly recursive with NO depth limit and NO per-level
// branching: every unit row carries its `depth`, `hasChildren`, `expanded`,
// cosmetic `kind`, and `state`.
//
// The model OWNS the selection (by identity, not row index) so the highlight
// survives expand/collapse rebuilds, and owns all tree navigation/mutation so the
// structural logic lives in one place (and stays unit-testable in C++). `store` is
// bound from QML to the shared session store (a QObject*).
class SidebarModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QObject* store READ store WRITE setStore NOTIFY storeChanged)
    // The profile store (agents == profiles; daemon-supervision-spec §0). The FLEET section renders
    // one agent row per ProfileInfo from here (NOT from the node's tree()). Bound from QML to the
    // shared `Profiles` context property. Null => the node root shows with no agents.
    Q_PROPERTY(QObject* profiles READ profiles WRITE setProfiles NOTIFY profilesChanged)
    // The label for the Fleet's node/connection ROOT row — the client-side representation of this
    // client's link to the (local) node. Bound from QML to the connection target/name. Defaults to
    // a generic local-node label.
    Q_PROPERTY(QString nodeLabel READ nodeLabel WRITE setNodeLabel NOTIFY nodeLabelChanged)
    // True when at least one unit-with-children is currently expanded; drives the
    // Fleet header's collapse-all/expand-all toggle. Refreshed on rebuild.
    Q_PROPERTY(bool anyExpanded READ anyExpanded NOTIFY treeChanged)
    // The Integrations equivalent: true when at least one transport node (account /
    // conversation group) is currently expanded; drives the Integrations header's
    // collapse-all/expand-all toggle.
    Q_PROPERTY(bool anyTransportExpanded READ anyTransportExpanded NOTIFY treeChanged)

public:
    enum Role {
        LabelRole = Qt::UserRole + 1,
        CountRole,
        NodeTypeRole,
        TagIdRole,  // tag id for Tag rows; -1 otherwise
        UnitIdRole, // unit id for Unit rows; empty otherwise
        IsSeparatorRole,
        SelectableRole,
        ColorRole, // tag color for the dot; empty otherwise
        DepthRole, // tree depth (0 = top-level root / non-unit rows)
        HasChildrenRole,
        ExpandedRole,
        KindRole,      // domain::UnitKind (cosmetic) for Unit rows
        StateRole,     // domain::UnitState for Unit rows
        CurrentRole,   // true for the currently-selected row (identity match)
        ProfileRole,   // the unit's profile (ProfileRef == agent identity); empty if none
        SessionIdRole, // the unit's backing session id (UnitNode.session) / a transport leaf's
                       // session
        // Integrations-section rows (NodeType::Transport):
        TxKindRole,   // "account" | "convGroup" | "conversation" | "job" | "caller"
        ConvTypeRole, // conversation type: "channel"|"groupdm"|"dm"|"thread" (else "")
        SubLabelRole, // secondary label: a transport leaf's inline session title (else "")
        PresenceRole, // account rows: PresencePrimitive ("available"/... ; else "")
    };

    explicit SidebarModel(QObject* parent = nullptr);

    [[nodiscard]] QObject* store() const;
    void setStore(QObject* store);

    [[nodiscard]] QObject* profiles() const;
    void setProfiles(QObject* profiles);

    [[nodiscard]] QString nodeLabel() const;
    void setNodeLabel(const QString& label);

    [[nodiscard]] bool anyExpanded() const;
    [[nodiscard]] bool anyTransportExpanded() const;

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Select a row: records the selection by identity, emits scopeSelected, and
    // repaints the highlight. No-op for separators.
    Q_INVOKABLE void activate(int row);
    // Expand/collapse a unit row (no-op for leaves and non-unit rows). If a
    // collapse would hide the current selection, the selection moves up to the
    // collapsed unit (VSCode behavior).
    Q_INVOKABLE void toggleExpand(int row);

    // Keyboard navigation, all operating on the currently-selected row.
    Q_INVOKABLE void selectNext();      // Down: next selectable row
    Q_INVOKABLE void selectPrevious();  // Up: previous selectable row
    Q_INVOKABLE void collapseCurrent(); // Left: collapse, else go to parent
    Q_INVOKABLE void expandCurrent();   // Right: expand, else go to first child
    Q_INVOKABLE void activateCurrent(); // Enter: re-emit the current scope

    // Whole-tree expand/collapse (Fleet header control; unit-scoped).
    Q_INVOKABLE void expandAll();
    Q_INVOKABLE void collapseAll();

    // Whole-tree expand/collapse for the Integrations section (transport-node scoped;
    // the events-IO equivalent of expandAll/collapseAll).
    Q_INVOKABLE void expandAllTransports();
    Q_INVOKABLE void collapseAllTransports();

    // Fleet "+": request the create-AGENT flow (agents == profiles). QML opens a small agent form
    // and calls ProfileRepository::createProfile(spec) (A2). Renamed intent from the earlier
    // "new chat" so the "+" mints an agent, not a bare session.
    Q_INVOKABLE void createRootUnit();
    Q_INVOKABLE void createTag();

    // Row index of the current selection (-1 if it is hidden/none). Lets QML keep
    // the view scrolled to the selection after keyboard navigation.
    [[nodiscard]] Q_INVOKABLE int currentRow() const;

signals:
    void storeChanged();
    void profilesChanged();
    void nodeLabelChanged();
    void treeChanged();
    // nodeType is a domain::NodeType; `tagId` is the tag id (Tag scope); `unitId`
    // is the supervision unit id (Unit scope). For the DaemonNet lens scopes
    // (ByTransport/ByPeer) the `unitId` string slot carries the lens key.
    void scopeSelected(int nodeType, int tagId, const QString& unitId);
    // An Integrations-section session leaf was activated: open its transcript directly
    // (the shared session leaf, cross-linked to the Fleet tree).
    void sessionActivated(const QString& sessionId);
    // An UNPINNED external conversation leaf was activated (B4/EIO-8): browse-only — the shell
    // opens the Channels page (pins stay explicit routing state; no lazy SessionCreate).
    void conversationActivated(const QString& transport, const QString& conversation);
    // "+ New agent/node" (Fleet header): the user decision is that this opens a working chat, not
    // an inert tree unit. The shell routes it to the same open-a-chat path the wizard-finish first
    // chat uses (a fresh transcript bound to the default profile; the session is minted on the
    // first Submit).
    void newChatRequested();
    // Fleet "+" (A2): the user wants to create a new AGENT (a profile). The shell opens a small
    // agent form (provider/model/key) and calls ProfileRepository::createProfile(spec). Distinct
    // from newChatRequested (which opens a session bound to an existing agent).
    void createAgentRequested();

private:
    struct Row {
        QString label;
        int count = -1;
        domain::NodeType type = domain::NodeType::AllSessions;
        int tagId = -1; // tag id
        QString unitId; // unit id (string at the model boundary)
        bool separator = false;
        bool selectable = true;
        QString color; // tag dot color
        int depth = 0;
        bool hasChildren = false;
        bool expanded = false;
        int kind = 0;    // domain::UnitKind
        int state = 0;   // domain::UnitState
        QString profile; // ProfileRef (agent identity) for Unit rows; empty otherwise
        QString session; // backing session id for Unit rows / transport leaf; empty otherwise
        // Transport rows (NodeType::Transport):
        QString txNode;     // transport tree node id (expand/collapse + selection identity)
        QString txKind;     // account|convGroup|conversation|job|caller
        QString convType;   // conversation type (else "")
        QString sublabel;   // transport: inline session title; agent: "provider · model" (else "")
        QString presence;   // account presence (else "")
        QString scopeKey;   // ByTransport/ByPeer key when the row has no session leaf
        int scopeType = -1; // domain::NodeType to emit for scopeKey (-1 = none)
        // Conversation rows (B4): the owning transport instance + conversation id, so an
        // unpinned leaf's activation can open the Channels page scoped to it.
        QString txTransport;
        QString txConversation;
        // Section header rows (Fleet/Tags/Integrations): a stable section id so the
        // whole section folds under its header (else ""). These headers carry
        // hasChildren = true + expanded so GUI/TUI reuse the disclosure plumbing.
        QString sectionKey;
        // Expand/collapse key for the Fleet membership rows (node root / agent), a disjoint id
        // namespace from unit/transport ids so they share m_collapsed safely (else "").
        QString expandKey;
    };

    void rebuild();
    // Build the FLEET membership view (daemon-supervision-spec §0): the node/connection ROOT row,
    // then one agent row per profile, then each expanded agent's sessions (ByProfile). Sourced from
    // the profile store + the session store, NOT from the node's tree()/unitChildren().
    void appendFleetMembership();
    // The agent rows (id/name/model) projected from the bound profile store, or empty when none.
    [[nodiscard]] QList<QVariantMap> agentRows() const;
    // Collect the Fleet membership expand keys currently foldable (the node root + agents that have
    // sessions), for the header's expand-all/collapse-all + anyExpanded.
    void collectFleetExpandKeys(QSet<QString>& out) const;
    // Push a collapsible section header (Fleet/Tags); `sectionKey` keys
    // its fold state in m_sectionCollapsed.
    void pushSectionHeader(const QString& label, domain::NodeType type, const QString& sectionKey);
    [[nodiscard]] bool isExpanded(const QString& id) const;
    // A headered section is expanded unless explicitly collapsed (default expanded).
    [[nodiscard]] bool isSectionExpanded(const QString& sectionKey) const;

    // Selection helpers (identity-based).
    void setSelectionFromRow(int row); // records identity + emits + repaints
    [[nodiscard]] bool rowIsCurrent(const Row& r) const;
    void emitCurrentChanged(); // dataChanged(CurrentRole) for all rows
    [[nodiscard]] int adjacentSelectableRow(int from, int delta) const;
    [[nodiscard]] int parentRow(int row) const;
    // True when the current selection is `rootId` or a descendant of it.
    [[nodiscard]] bool selectionInSubtree(const QString& rootId) const;
    // Collect the ids of every foldable transport node (accounts + conversation groups).
    void collectTransportExpandableIds(QSet<QString>& out) const;

    persistence::ISessionStore* m_store = nullptr;
    profiles::IProfileStore* m_profiles = nullptr;
    QString m_nodeLabel;
    QList<Row> m_rows;
    // Units are expanded by default; only explicitly-collapsed ids live here, so
    // the whole fleet tree is visible on first load (and toggling collapses).
    QSet<QString> m_collapsed;
    // Section headers (Fleet/Tags/Integrations) are expanded by default; only
    // explicitly-collapsed section ids live here. Folding a section hides its body.
    QSet<QString> m_sectionCollapsed;

    // The selection, stored by identity so it is stable across rebuilds.
    domain::NodeType m_selType = domain::NodeType::AllSessions;
    int m_selTag = -1;
    QString m_selUnit;
    QString m_selTxNode; // selected Integrations-section node id (identity-stable)
    QString m_selAgent;  // selected agent row (profile id), when m_selType == Agent
    QString
        m_selSession; // selected agent-session leaf (session id), when m_selType == AgentSession
};
