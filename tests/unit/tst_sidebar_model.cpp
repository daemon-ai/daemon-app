// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemonnet/mock_daemonnet.h"
#include "persistence/in_memory_session_store.h"
#include "profiles/iprofile_store.h"
#include "sidebar_model.h"
#include "uimodels/variant_list_model.h"

#include <QSignalSpy>
#include <QtTest>
#include <utility>

using daemonnet::MockDaemonNet;
using persistence::InMemorySessionStore;

namespace {
QVariantMap agentRow(const QString& id, const QString& name, const QString& provider,
                     const QString& model, bool isDefault) {
    QVariantMap m;
    m[QStringLiteral("id")] = id;
    m[QStringLiteral("name")] = name;
    m[QStringLiteral("provider")] = provider;
    m[QStringLiteral("model")] = model;
    m[QStringLiteral("isDefault")] = isDefault;
    return m;
}

// A configured agent + an unconfigured placeholder: drives the provider/model sublabel cases.
QList<QVariantMap> twoAgentRows() {
    return {agentRow(QStringLiteral("anthropic"), QStringLiteral("anthropic"),
                     QStringLiteral("genai"), QStringLiteral("claude-opus-4-8"), true),
            agentRow(QStringLiteral("default"), QStringLiteral("default"),
                     QStringLiteral("daemon_api"), QString(), false)};
}

// A roster mirroring MockDaemonNet's unit->profile binding (its seeded sessions carry
// boundProfile prof-1/2/3), so agent rows resolve real ByProfile session leaves from the store.
// prof-3 deliberately owns no non-archived session (a leaf agent, not foldable).
QList<QVariantMap> rosterRows() {
    return {agentRow(QStringLiteral("prof-1"), QStringLiteral("General Assistant"),
                     QStringLiteral("genai"), QStringLiteral("llama-3.1-8b-instruct"), true),
            agentRow(QStringLiteral("prof-2"), QStringLiteral("Coder"), QStringLiteral("llama_cpp"),
                     QStringLiteral("qwen2.5-coder-32b"), false),
            agentRow(QStringLiteral("prof-3"), QStringLiteral("Researcher"),
                     QStringLiteral("genai"), QStringLiteral("mixtral-8x7b"), false)};
}

// A minimal IProfileStore over fixed agent rows, to drive the Fleet membership section
// deterministically (the FLEET body renders node root -> one agent row per profile).
class FakeProfileStore : public profiles::IProfileStore {
public:
    explicit FakeProfileStore(const QList<QVariantMap>& rows, QString defaultId,
                              QObject* parent = nullptr)
        : profiles::IProfileStore(parent), m_model(new uimodels::VariantListModel(this)),
          m_default(std::move(defaultId)) {
        m_model->setRows(rows);
    }
    [[nodiscard]] QObject* profiles() const override { return m_model; }
    [[nodiscard]] QString defaultProfileId() const override { return m_default; }
    [[nodiscard]] QVariantMap profile(const QString& id) const override {
        const int row = m_model->indexOfId(id);
        return row >= 0 ? m_model->at(row) : QVariantMap{};
    }
    [[nodiscard]] QStringList profileNames() const override { return {}; }
    [[nodiscard]] QVariantList availableSkills() const override { return {}; }
    [[nodiscard]] QVariantList availableTools() const override { return {}; }
    QString createProfile(const QString&) override { return {}; }
    QString cloneProfile(const QString&, const QString&) override { return {}; }
    void updateProfile(const QString&, const QVariantMap&) override {}
    void remove(const QString&) override {}
    void setDefault(const QString&) override {}

private:
    uimodels::VariantListModel* m_model = nullptr;
    QString m_default;
};
} // namespace

// Exercises the sidebar model: the Fleet MEMBERSHIP view (node/connection root -> agent rows ==
// profiles -> per-agent session leaves; daemon-supervision-spec §0), its expand/collapse and
// identity-based selection, the collapsible section headers, and the co-equal Integrations tree.
class TestSidebarModel : public QObject {
    Q_OBJECT

private:
    static int findRow(const SidebarModel& m, const QString& label) {
        for (int i = 0; i < m.rowCount(); ++i) {
            if (m.data(m.index(i, 0), SidebarModel::LabelRole).toString() == label) {
                return i;
            }
        }
        return -1;
    }

    template <typename T>
    static T roleAt(const SidebarModel& m, int row, SidebarModel::Role role) {
        return m.data(m.index(row, 0), role).value<T>();
    }

    // An agent's non-archived session count, straight from the store's ByProfile scope (the same
    // query the model issues), so expectations track the seed instead of magic numbers.
    static int agentSessions(const InMemorySessionStore& store, const QString& profileId) {
        return store.sessionCount({domain::NodeType::Agent, -1, domain::UnitId(), profileId});
    }

private slots:
    // The Fleet membership view is a fixed-shape tree: the node/connection ROOT at depth 0, one
    // agent row per profile at depth 1, and each agent's session leaves at depth 2.
    void fleetMembershipFlattensWithDepths() {
        MockDaemonNet net;
        InMemorySessionStore store(&net);
        FakeProfileStore profiles(rosterRows(), QStringLiteral("prof-1"));
        SidebarModel model;
        model.setStore(&store);
        model.setProfiles(&profiles);

        const int root = findRow(model, QStringLiteral("Local node"));
        const int assistant = findRow(model, QStringLiteral("General Assistant"));
        const int coder = findRow(model, QStringLiteral("Coder"));
        const int leaf = findRow(model, QStringLiteral("Implement endpoint"));
        QVERIFY(root >= 0 && assistant >= 0 && coder >= 0 && leaf >= 0);

        QCOMPARE(roleAt<int>(model, root, SidebarModel::DepthRole), 0);
        QCOMPARE(roleAt<int>(model, assistant, SidebarModel::DepthRole), 1);
        QCOMPARE(roleAt<int>(model, coder, SidebarModel::DepthRole), 1);
        QCOMPARE(roleAt<int>(model, leaf, SidebarModel::DepthRole), 2);
        // Root before agents, agents before their own leaves (store order preserved).
        QVERIFY(root < assistant && assistant < coder && coder < leaf);
    }

    // Membership rows carry the §0 roles: the root is a FleetNode counting its agents; an agent
    // row is NodeType::Agent bound to its profile id, counting (and folding) its sessions; an
    // agent with no sessions is a plain leaf; a session leaf carries profile + session ids.
    void exposesFleetMembershipRoles() {
        MockDaemonNet net;
        InMemorySessionStore store(&net);
        FakeProfileStore profiles(rosterRows(), QStringLiteral("prof-1"));
        SidebarModel model;
        model.setStore(&store);
        model.setProfiles(&profiles);

        const int root = findRow(model, QStringLiteral("Local node"));
        QCOMPARE(roleAt<int>(model, root, SidebarModel::NodeTypeRole), 10); // FleetNode
        QVERIFY(roleAt<bool>(model, root, SidebarModel::HasChildrenRole));
        QCOMPARE(roleAt<int>(model, root, SidebarModel::CountRole), 3); // the roster's agents

        const int coder = findRow(model, QStringLiteral("Coder"));
        QCOMPARE(roleAt<int>(model, coder, SidebarModel::NodeTypeRole), 11); // Agent
        QCOMPARE(roleAt<QString>(model, coder, SidebarModel::ProfileRole),
                 QStringLiteral("prof-2"));
        QVERIFY(roleAt<bool>(model, coder, SidebarModel::HasChildrenRole));
        QVERIFY(agentSessions(store, QStringLiteral("prof-2")) > 0);
        QCOMPARE(roleAt<int>(model, coder, SidebarModel::CountRole),
                 agentSessions(store, QStringLiteral("prof-2")));

        // prof-3 owns no non-archived session: a leaf row, nothing to fold.
        const int researcher = findRow(model, QStringLiteral("Researcher"));
        QCOMPARE(agentSessions(store, QStringLiteral("prof-3")), 0);
        QVERIFY(!roleAt<bool>(model, researcher, SidebarModel::HasChildrenRole));

        const int leaf = findRow(model, QStringLiteral("Implement endpoint"));
        QCOMPARE(roleAt<int>(model, leaf, SidebarModel::NodeTypeRole), 12); // AgentSession
        QCOMPARE(roleAt<QString>(model, leaf, SidebarModel::ProfileRole), QStringLiteral("prof-2"));
        QCOMPARE(roleAt<QString>(model, leaf, SidebarModel::SessionIdRole),
                 QStringLiteral("s-coder-impl"));
    }

    // Collapsing an agent hides exactly its session leaves; collapsing the node root hides the
    // whole membership body (agents included); expanding restores both.
    void toggleExpandFoldsAgentAndRoot() {
        MockDaemonNet net;
        InMemorySessionStore store(&net);
        FakeProfileStore profiles(rosterRows(), QStringLiteral("prof-1"));
        SidebarModel model;
        model.setStore(&store);
        model.setProfiles(&profiles);

        const int before = model.rowCount();
        const int coder = findRow(model, QStringLiteral("Coder"));
        QVERIFY(roleAt<bool>(model, coder, SidebarModel::ExpandedRole));

        model.toggleExpand(coder);
        QVERIFY(findRow(model, QStringLiteral("Implement endpoint")) < 0);
        QCOMPARE(model.rowCount(), before - agentSessions(store, QStringLiteral("prof-2")));
        QVERIFY(!roleAt<bool>(model, findRow(model, QStringLiteral("Coder")),
                              SidebarModel::ExpandedRole));

        model.toggleExpand(findRow(model, QStringLiteral("Coder")));
        QCOMPARE(model.rowCount(), before);
        QVERIFY(findRow(model, QStringLiteral("Implement endpoint")) >= 0);

        // The node root folds the whole membership (all agents + leaves), root row stays.
        model.toggleExpand(findRow(model, QStringLiteral("Local node")));
        QVERIFY(findRow(model, QStringLiteral("Local node")) >= 0);
        QVERIFY(findRow(model, QStringLiteral("Coder")) < 0);
        QVERIFY(findRow(model, QStringLiteral("Implement endpoint")) < 0);
        model.toggleExpand(findRow(model, QStringLiteral("Local node")));
        QCOMPARE(model.rowCount(), before);
    }

    // Activating the node root scopes the list to ALL of this node's sessions.
    void activateNodeRootScopesAllSessions() {
        MockDaemonNet net;
        InMemorySessionStore store(&net);
        FakeProfileStore profiles(rosterRows(), QStringLiteral("prof-1"));
        SidebarModel model;
        model.setStore(&store);
        model.setProfiles(&profiles);

        QSignalSpy spy(&model, &SidebarModel::scopeSelected);
        const int root = findRow(model, QStringLiteral("Local node"));
        model.activate(root);

        QCOMPARE(spy.count(), 1);
        const QList<QVariant> args = spy.takeFirst();
        QCOMPARE(args.at(0).toInt(), 0); // NodeType::AllSessions
        QVERIFY(args.at(2).toString().isEmpty());
        QVERIFY(roleAt<bool>(model, root, SidebarModel::CurrentRole));
    }

    // Activating an agent row scopes the session list to its profile (ByProfile; the profile id
    // rides the string slot as the lens key).
    void activateAgentScopesByProfile() {
        MockDaemonNet net;
        InMemorySessionStore store(&net);
        FakeProfileStore profiles(rosterRows(), QStringLiteral("prof-1"));
        SidebarModel model;
        model.setStore(&store);
        model.setProfiles(&profiles);

        QSignalSpy spy(&model, &SidebarModel::scopeSelected);
        model.activate(findRow(model, QStringLiteral("Coder")));

        QCOMPARE(spy.count(), 1);
        const QList<QVariant> args = spy.takeFirst();
        QCOMPARE(args.at(0).toInt(), 11); // NodeType::Agent
        QCOMPARE(args.at(2).toString(), QStringLiteral("prof-2"));
    }

    // Activating a session leaf under an agent opens its transcript (sessionActivated), not a
    // list scope — the same contract as an Integrations session leaf.
    void activateAgentSessionLeafOpensTranscript() {
        MockDaemonNet net;
        InMemorySessionStore store(&net);
        FakeProfileStore profiles(rosterRows(), QStringLiteral("prof-1"));
        SidebarModel model;
        model.setStore(&store);
        model.setProfiles(&profiles);

        QSignalSpy opened(&model, &SidebarModel::sessionActivated);
        QSignalSpy scoped(&model, &SidebarModel::scopeSelected);
        model.activate(findRow(model, QStringLiteral("Implement endpoint")));

        QCOMPARE(opened.count(), 1);
        QCOMPARE(opened.takeFirst().at(0).toString(), QStringLiteral("s-coder-impl"));
        QCOMPARE(scoped.count(), 0);
    }

    // Separator rows (Fleet / Tags headers) are not selectable and emit nothing.
    void separatorsAreNotSelectable() {
        InMemorySessionStore store;
        SidebarModel model;
        model.setStore(&store);

        const int fleet = findRow(model, QStringLiteral("Fleet"));
        QVERIFY(fleet >= 0);
        QVERIFY(roleAt<bool>(model, fleet, SidebarModel::IsSeparatorRole));

        QSignalSpy spy(&model, &SidebarModel::scopeSelected);
        model.activate(fleet);
        QCOMPARE(spy.count(), 0);
    }

    // The highlight is identity-based: it follows the agent across a model reset rather than
    // sticking to a row index. Folding a SIBLING agent rebuilds the list but must leave the
    // selected agent highlighted.
    void currentRoleTracksIdentityAcrossRebuild() {
        MockDaemonNet net;
        InMemorySessionStore store(&net);
        FakeProfileStore profiles(rosterRows(), QStringLiteral("prof-1"));
        SidebarModel model;
        model.setStore(&store);
        model.setProfiles(&profiles);

        model.activate(findRow(model, QStringLiteral("Coder")));

        // General Assistant is a sibling agent; folding it triggers a full rebuild without
        // hiding Coder ("Scratch ideas" is one of its leaves — its absence proves the rebuild).
        model.toggleExpand(findRow(model, QStringLiteral("General Assistant")));
        QVERIFY(findRow(model, QStringLiteral("Scratch ideas")) < 0);
        const int coderAfter = findRow(model, QStringLiteral("Coder"));
        QVERIFY(coderAfter >= 0);
        QVERIFY(roleAt<bool>(model, coderAfter, SidebarModel::CurrentRole));
        QCOMPARE(model.currentRow(), coderAfter);
    }

    // Folding the agent that contains the selected session leaf hides it (currentRow -1, no
    // silent reassignment); re-expanding restores the SAME leaf as current — the selection is
    // stored by identity, never by row index.
    void selectionIdentitySurvivesFoldAndUnfold() {
        MockDaemonNet net;
        InMemorySessionStore store(&net);
        FakeProfileStore profiles(rosterRows(), QStringLiteral("prof-1"));
        SidebarModel model;
        model.setStore(&store);
        model.setProfiles(&profiles);

        model.activate(findRow(model, QStringLiteral("Implement endpoint")));

        model.toggleExpand(findRow(model, QStringLiteral("Coder")));
        QVERIFY(findRow(model, QStringLiteral("Implement endpoint")) < 0);
        QCOMPARE(model.currentRow(), -1); // hidden, not reassigned

        model.toggleExpand(findRow(model, QStringLiteral("Coder")));
        const int leaf = findRow(model, QStringLiteral("Implement endpoint"));
        QVERIFY(leaf >= 0);
        QVERIFY(roleAt<bool>(model, leaf, SidebarModel::CurrentRole));
        QCOMPARE(model.currentRow(), leaf);
    }

    // Up/Down move the selection between adjacent selectable rows, skipping
    // separators; Enter re-emits the current scope.
    void keyboardNavigationMovesSelection() {
        InMemorySessionStore store;
        SidebarModel model;
        model.setStore(&store);

        model.activate(findRow(model, QStringLiteral("All Sessions")));
        QSignalSpy spy(&model, &SidebarModel::scopeSelected);

        model.selectNext(); // -> Archived
        QCOMPARE(model.currentRow(), findRow(model, QStringLiteral("Archived")));
        QCOMPARE(spy.takeFirst().at(0).toInt(), 1); // NodeType::Archived

        model.selectPrevious(); // -> All Sessions
        QCOMPARE(model.currentRow(), findRow(model, QStringLiteral("All Sessions")));

        spy.clear();
        model.activateCurrent(); // Enter re-emits without moving
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.takeFirst().at(0).toInt(), 0); // NodeType::AllSessions
    }

    // Left collapses the node root in place; Right re-expands it, then descends to its first
    // agent; Left on an expanded agent folds it while the selection stays put.
    void arrowKeysExpandCollapseAndTraverse() {
        MockDaemonNet net;
        InMemorySessionStore store(&net);
        FakeProfileStore profiles(rosterRows(), QStringLiteral("prof-1"));
        SidebarModel model;
        model.setStore(&store);
        model.setProfiles(&profiles);

        model.activate(findRow(model, QStringLiteral("Local node")));

        model.collapseCurrent(); // collapse the root (selection stays)
        QVERIFY(!roleAt<bool>(model, findRow(model, QStringLiteral("Local node")),
                              SidebarModel::ExpandedRole));
        QVERIFY(findRow(model, QStringLiteral("Coder")) < 0);

        model.expandCurrent(); // re-expand the root
        QVERIFY(roleAt<bool>(model, findRow(model, QStringLiteral("Local node")),
                             SidebarModel::ExpandedRole));

        model.expandCurrent(); // already expanded -> step to the first agent
        QCOMPARE(model.currentRow(), findRow(model, QStringLiteral("General Assistant")));

        model.activate(findRow(model, QStringLiteral("Coder")));
        model.collapseCurrent(); // fold the agent in place
        const int coder = findRow(model, QStringLiteral("Coder"));
        QVERIFY(!roleAt<bool>(model, coder, SidebarModel::ExpandedRole));
        QCOMPARE(model.currentRow(), coder);
    }

    // Collapse-all folds the whole membership (root included: agents + leaves hidden, the root
    // row remains); expand-all restores it. anyExpanded drives the header's toggle.
    void expandAllAndCollapseAllToggleWholeTree() {
        MockDaemonNet net;
        InMemorySessionStore store(&net);
        FakeProfileStore profiles(rosterRows(), QStringLiteral("prof-1"));
        SidebarModel model;
        model.setStore(&store);
        model.setProfiles(&profiles);

        QVERIFY(model.anyExpanded());
        QVERIFY(findRow(model, QStringLiteral("Implement endpoint")) >= 0);

        model.collapseAll();
        QVERIFY(!model.anyExpanded());
        QVERIFY(findRow(model, QStringLiteral("Local node")) >= 0); // the root stays
        QVERIFY(findRow(model, QStringLiteral("Coder")) < 0);
        QVERIFY(findRow(model, QStringLiteral("Implement endpoint")) < 0);

        model.expandAll();
        QVERIFY(model.anyExpanded());
        QVERIFY(findRow(model, QStringLiteral("Coder")) >= 0);
        QVERIFY(findRow(model, QStringLiteral("Implement endpoint")) >= 0);
    }

    // collapseAll while an agent is selected keeps the selection identity: the row is hidden
    // (inside the folded root), and expandAll surfaces the SAME agent as current again.
    void collapseAllKeepsSelectionIdentity() {
        MockDaemonNet net;
        InMemorySessionStore store(&net);
        FakeProfileStore profiles(rosterRows(), QStringLiteral("prof-1"));
        SidebarModel model;
        model.setStore(&store);
        model.setProfiles(&profiles);

        model.activate(findRow(model, QStringLiteral("Coder")));
        model.collapseAll();
        QCOMPARE(model.currentRow(), -1); // hidden under the folded root

        model.expandAll();
        const int coder = findRow(model, QStringLiteral("Coder"));
        QVERIFY(coder >= 0);
        QVERIFY(roleAt<bool>(model, coder, SidebarModel::CurrentRole));
    }

    // The Fleet "+" mints an AGENT (a profile) through the create-agent flow: it requests the
    // form and adds NO client-side row (the agent appears via the post-create profile re-list).
    void createRootUnitRequestsAgentCreation() {
        MockDaemonNet net;
        InMemorySessionStore store(&net);
        FakeProfileStore profiles(rosterRows(), QStringLiteral("prof-1"));
        SidebarModel model;
        model.setStore(&store);
        model.setProfiles(&profiles);

        QSignalSpy requested(&model, &SidebarModel::createAgentRequested);
        QSignalSpy scoped(&model, &SidebarModel::scopeSelected);
        const int before = model.rowCount();
        model.createRootUnit();

        QCOMPARE(requested.count(), 1);
        QCOMPARE(model.rowCount(), before); // nothing is client-minted
        QCOMPARE(scoped.count(), 0);        // the selection is untouched
    }

    // Creating a tag adds a tag row and selects it.
    void createTagAddsAndSelects() {
        InMemorySessionStore store;
        SidebarModel model;
        model.setStore(&store);

        QSignalSpy spy(&model, &SidebarModel::scopeSelected);
        model.createTag();

        const int created = findRow(model, QStringLiteral("New tag"));
        QVERIFY(created >= 0);
        QCOMPARE(roleAt<int>(model, created, SidebarModel::NodeTypeRole), 5); // Tag
        QVERIFY(roleAt<bool>(model, created, SidebarModel::CurrentRole));
        QCOMPARE(spy.takeLast().at(0).toInt(), 5);
    }

    // --- The co-equal Integrations section (events-IO axis) -----------------

    // With a DaemonNet source, an "Integrations" header + the capability-driven tree
    // appear alongside the Fleet/Tags sections, each instance expanded to its taxonomy.
    void transportsSectionShape() {
        InMemorySessionStore store;
        MockDaemonNet net;
        SidebarModel model;
        model.setStore(&store);
        model.setDaemonNet(&net);

        // The header (NodeType::TransportSeparator == 8) is a non-selectable separator,
        // presented to the user as "Integrations".
        const int header = findRow(model, QStringLiteral("Integrations"));
        QVERIFY(header >= 0);
        QVERIFY(roleAt<bool>(model, header, SidebarModel::IsSeparatorRole));
        QCOMPARE(roleAt<int>(model, header, SidebarModel::NodeTypeRole), 8);

        // Four transport accounts, two messaging (with presence) + two generic.
        const int matrix = findRow(model, QStringLiteral("matrix /@bot:hs.org"));
        const int internal = findRow(model, QStringLiteral("internal (rooms)"));
        const int cron = findRow(model, QStringLiteral("cron"));
        const int http = findRow(model, QStringLiteral("http /api"));
        QVERIFY(matrix >= 0 && internal >= 0 && cron >= 0 && http >= 0);
        QCOMPARE(roleAt<int>(model, matrix, SidebarModel::NodeTypeRole), 9); // Transport
        QCOMPARE(roleAt<QString>(model, matrix, SidebarModel::TxKindRole),
                 QStringLiteral("account"));
        QCOMPARE(roleAt<QString>(model, matrix, SidebarModel::PresenceRole),
                 QStringLiteral("available"));

        // Matrix groups its conversations under Channels + Direct Messages.
        QVERIFY(findRow(model, QStringLiteral("Channels")) >= 0);
        QVERIFY(findRow(model, QStringLiteral("Direct Messages")) >= 0);

        // #secops is a Channel conversation; @alice a Dm; design-review a GroupDm.
        const int secops = findRow(model, QStringLiteral("#secops"));
        QVERIFY(secops >= 0);
        QCOMPARE(roleAt<QString>(model, secops, SidebarModel::ConvTypeRole),
                 QStringLiteral("channel"));
        QCOMPARE(roleAt<QString>(model, secops, SidebarModel::SessionIdRole),
                 QStringLiteral("s-secops"));
        QCOMPARE(roleAt<QString>(model, secops, SidebarModel::SubLabelRole),
                 QStringLiteral("triage"));
        QCOMPARE(roleAt<QString>(model, findRow(model, QStringLiteral("@alice")),
                                 SidebarModel::ConvTypeRole),
                 QStringLiteral("dm"));

        // Generic transports skip conversation groups: jobs/callers sit directly under.
        QVERIFY(findRow(model, QStringLiteral("nightly-backup")) >= 0);
        QVERIFY(findRow(model, QStringLiteral("key: dashboard")) >= 0);
    }

    // Selecting a conversation/job leaf with a session opens it (sessionActivated),
    // not a list scope.
    void transportLeafOpensSession() {
        InMemorySessionStore store;
        MockDaemonNet net;
        SidebarModel model;
        model.setStore(&store);
        model.setDaemonNet(&net);

        QSignalSpy opened(&model, &SidebarModel::sessionActivated);
        QSignalSpy scoped(&model, &SidebarModel::scopeSelected);
        model.activate(findRow(model, QStringLiteral("#secops")));

        QCOMPARE(opened.count(), 1);
        QCOMPARE(opened.takeFirst().at(0).toString(), QStringLiteral("s-secops"));
        QCOMPARE(scoped.count(), 0);
    }

    // Selecting a transport account scopes the list to its sessions (ByTransport,
    // the lens key in the string slot).
    void transportAccountScopesList() {
        InMemorySessionStore store;
        MockDaemonNet net;
        SidebarModel model;
        model.setStore(&store);
        model.setDaemonNet(&net);

        QSignalSpy scoped(&model, &SidebarModel::scopeSelected);
        QSignalSpy opened(&model, &SidebarModel::sessionActivated);
        model.activate(findRow(model, QStringLiteral("matrix /@bot:hs.org")));

        QCOMPARE(scoped.count(), 1);
        const QList<QVariant> args = scoped.takeFirst();
        QCOMPARE(args.at(0).toInt(), 6); // NodeType::ByTransport
        QCOMPARE(args.at(2).toString(), QStringLiteral("matrix/@bot:hs.org"));
        QCOMPARE(opened.count(), 0);
    }

    // Daemon-mode honesty (W3, plan 2d): a DaemonNet whose transports tree seeds empty (what the
    // daemon service graph constructs until the live projection lands) renders NO Integrations
    // section at all — no header, no rows, nothing foldable — while the co-equal sections render
    // as usual.
    void emptyTransportsTreeRendersNoIntegrationsSection() {
        InMemorySessionStore store;
        MockDaemonNet net(daemonnet::TransportsSeed::Empty);
        SidebarModel model;
        model.setStore(&store);
        model.setDaemonNet(&net);

        QVERIFY(findRow(model, QStringLiteral("Integrations")) < 0);
        QVERIFY(findRow(model, QStringLiteral("matrix /@bot:hs.org")) < 0);
        QVERIFY(findRow(model, QStringLiteral("cron")) < 0);
        QVERIFY(!model.anyTransportExpanded());
        // Fleet/Tags are unaffected.
        QVERIFY(findRow(model, QStringLiteral("Fleet")) >= 0);
        QVERIFY(findRow(model, QStringLiteral("Tags")) >= 0);
    }

    // Collapsing a transport account hides its whole conversation subtree.
    void transportAccountCollapses() {
        InMemorySessionStore store;
        MockDaemonNet net;
        SidebarModel model;
        model.setStore(&store);
        model.setDaemonNet(&net);

        QVERIFY(findRow(model, QStringLiteral("#secops")) >= 0);
        const int matrix = findRow(model, QStringLiteral("matrix /@bot:hs.org"));
        QVERIFY(roleAt<bool>(model, matrix, SidebarModel::HasChildrenRole));
        model.toggleExpand(matrix);

        QVERIFY(findRow(model, QStringLiteral("Channels")) < 0);
        QVERIFY(findRow(model, QStringLiteral("#secops")) < 0);
        // A sibling account is untouched.
        QVERIFY(findRow(model, QStringLiteral("internal (rooms)")) >= 0);
    }

    // --- Fleet membership (agents == profiles) -------------------------------

    // W2 (2b): agent rows carry a "provider · model" secondary label from the profile row
    // fields, so the Fleet shows each agent's actual configuration; an unconfigured agent
    // (empty model) shows just its provider — no dangling separator.
    void agentRowsCarryProviderModelSecondaryLabel() {
        InMemorySessionStore store;
        FakeProfileStore profiles(twoAgentRows(), QStringLiteral("anthropic"));
        SidebarModel model;
        model.setStore(&store);
        model.setProfiles(&profiles);

        const int configured = findRow(model, QStringLiteral("anthropic"));
        QVERIFY(configured >= 0);
        QCOMPARE(roleAt<int>(model, configured, SidebarModel::NodeTypeRole), 11); // Agent
        QCOMPARE(roleAt<QString>(model, configured, SidebarModel::SubLabelRole),
                 QStringLiteral("genai \u00b7 claude-opus-4-8"));

        const int bare = findRow(model, QStringLiteral("default"));
        QVERIFY(bare >= 0);
        QCOMPARE(roleAt<QString>(model, bare, SidebarModel::SubLabelRole),
                 QStringLiteral("daemon_api"));
    }

    // --- Collapsible section headers (Fleet / Tags / Integrations) ----------

    // Folding the Fleet header hides its whole membership body (node root + agents + leaves)
    // while the header stays, and its ExpandedRole flips; other sections are untouched.
    void fleetSectionHeaderCollapses() {
        MockDaemonNet net;
        InMemorySessionStore store(&net);
        FakeProfileStore profiles(rosterRows(), QStringLiteral("prof-1"));
        SidebarModel model;
        model.setStore(&store);
        model.setProfiles(&profiles);

        const int fleet = findRow(model, QStringLiteral("Fleet"));
        QVERIFY(fleet >= 0);
        QVERIFY(roleAt<bool>(model, fleet, SidebarModel::IsSeparatorRole));
        QVERIFY(roleAt<bool>(model, fleet, SidebarModel::HasChildrenRole));
        QVERIFY(roleAt<bool>(model, fleet, SidebarModel::ExpandedRole));
        QVERIFY(findRow(model, QStringLiteral("Local node")) >= 0);
        QVERIFY(findRow(model, QStringLiteral("Coder")) >= 0);

        model.toggleExpand(fleet);

        // Body gone, header stays and reads collapsed.
        const int fleetAfter = findRow(model, QStringLiteral("Fleet"));
        QVERIFY(fleetAfter >= 0);
        QVERIFY(!roleAt<bool>(model, fleetAfter, SidebarModel::ExpandedRole));
        QVERIFY(findRow(model, QStringLiteral("Local node")) < 0);
        QVERIFY(findRow(model, QStringLiteral("Coder")) < 0);
        // Other sections remain.
        QVERIFY(findRow(model, QStringLiteral("Tags")) >= 0);
        QVERIFY(findRow(model, QStringLiteral("All Sessions")) >= 0);

        // Re-expand restores the body.
        model.toggleExpand(findRow(model, QStringLiteral("Fleet")));
        QVERIFY(findRow(model, QStringLiteral("Local node")) >= 0);
    }

    // The Tags section folds independently of Fleet.
    void tagsSectionHeaderCollapses() {
        InMemorySessionStore store;
        SidebarModel model;
        model.setStore(&store);

        // A seeded demo tag exists under the Tags header.
        QVERIFY(!store.tags().isEmpty());
        const QString firstTag = store.tags().first().name;
        QVERIFY(findRow(model, firstTag) >= 0);

        model.toggleExpand(findRow(model, QStringLiteral("Tags")));
        QVERIFY(findRow(model, firstTag) < 0);
        QVERIFY(findRow(model, QStringLiteral("Tags")) >= 0);
        // Fleet is unaffected: its node root row is still rendered.
        QVERIFY(findRow(model, QStringLiteral("Local node")) >= 0);
    }

    // The Integrations section folds its whole transport tree.
    void integrationsSectionHeaderCollapses() {
        InMemorySessionStore store;
        MockDaemonNet net;
        SidebarModel model;
        model.setStore(&store);
        model.setDaemonNet(&net);

        const int header = findRow(model, QStringLiteral("Integrations"));
        QVERIFY(header >= 0);
        QVERIFY(roleAt<bool>(model, header, SidebarModel::HasChildrenRole));
        QVERIFY(findRow(model, QStringLiteral("matrix /@bot:hs.org")) >= 0);

        model.toggleExpand(header);
        QVERIFY(findRow(model, QStringLiteral("Integrations")) >= 0);
        QVERIFY(findRow(model, QStringLiteral("matrix /@bot:hs.org")) < 0);
        QVERIFY(findRow(model, QStringLiteral("#secops")) < 0);

        model.toggleExpand(findRow(model, QStringLiteral("Integrations")));
        QVERIFY(findRow(model, QStringLiteral("matrix /@bot:hs.org")) >= 0);
    }

    // Tags section is ordered above the Fleet section in the flattened list.
    void tagsSectionIsAboveFleet() {
        InMemorySessionStore store;
        SidebarModel model;
        model.setStore(&store);

        const int tags = findRow(model, QStringLiteral("Tags"));
        const int fleet = findRow(model, QStringLiteral("Fleet"));
        QVERIFY(tags >= 0 && fleet >= 0);
        QVERIFY2(tags < fleet, "Tags section must render above the Fleet section");
    }

    // The Integrations header's expand-all/collapse-all folds/unfolds the whole
    // transport tree (the events-IO equivalent of Fleet's control).
    void integrationsExpandAllCollapseAll() {
        InMemorySessionStore store;
        MockDaemonNet net;
        SidebarModel model;
        model.setStore(&store);
        model.setDaemonNet(&net);

        QVERIFY(model.anyTransportExpanded()); // expanded by default

        model.collapseAllTransports();
        QVERIFY(!model.anyTransportExpanded());
        // Accounts stay (top of the section); their children are folded away.
        QVERIFY(findRow(model, QStringLiteral("matrix /@bot:hs.org")) >= 0);
        QVERIFY(findRow(model, QStringLiteral("Channels")) < 0);
        QVERIFY(findRow(model, QStringLiteral("#secops")) < 0);

        model.expandAllTransports();
        QVERIFY(model.anyTransportExpanded());
        QVERIFY(findRow(model, QStringLiteral("Channels")) >= 0);
        QVERIFY(findRow(model, QStringLiteral("#secops")) >= 0);
    }

    // Fleet and Integrations expand/collapse-all are independent: one must not
    // clobber the other's fold state (they share m_collapsed across id namespaces).
    void fleetAndIntegrationsExpandAllAreIndependent() {
        MockDaemonNet net;
        InMemorySessionStore store(&net);
        FakeProfileStore profiles(rosterRows(), QStringLiteral("prof-1"));
        SidebarModel model;
        model.setStore(&store);
        model.setProfiles(&profiles);
        model.setDaemonNet(&net);

        // Fold one transport account by hand.
        model.toggleExpand(findRow(model, QStringLiteral("matrix /@bot:hs.org")));
        QVERIFY(findRow(model, QStringLiteral("Channels")) < 0);

        // Fleet expand-all must leave that transport account folded.
        model.expandAll();
        QVERIFY(findRow(model, QStringLiteral("Channels")) < 0);

        // Conversely, fold an agent, then Integrations expand-all keeps it folded.
        model.toggleExpand(findRow(model, QStringLiteral("Coder")));
        QVERIFY(findRow(model, QStringLiteral("Implement endpoint")) < 0);
        model.expandAllTransports();
        QVERIFY(findRow(model, QStringLiteral("Implement endpoint")) < 0);
    }
};

QTEST_MAIN(TestSidebarModel)
#include "tst_sidebar_model.moc"
