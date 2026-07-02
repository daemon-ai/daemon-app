// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemonnet/mock_daemonnet.h"
#include "persistence/in_memory_session_store.h"
#include "profiles/iprofile_store.h"
#include "sidebar_model.h"
#include "uimodels/variant_list_model.h"

#include <QSignalSpy>
#include <QtTest>

using daemonnet::MockDaemonNet;
using persistence::InMemorySessionStore;

namespace {
// A minimal IProfileStore exposing two fixed agent rows (a configured one and an unconfigured
// placeholder), to drive the Fleet membership section deterministically.
class FakeProfileStore : public profiles::IProfileStore {
public:
    FakeProfileStore() : m_model(new uimodels::VariantListModel(this)) {
        QVariantMap configured;
        configured[QStringLiteral("id")] = QStringLiteral("anthropic");
        configured[QStringLiteral("name")] = QStringLiteral("anthropic");
        configured[QStringLiteral("provider")] = QStringLiteral("genai");
        configured[QStringLiteral("model")] = QStringLiteral("claude-opus-4-8");
        configured[QStringLiteral("isDefault")] = true;
        m_model->upsert(configured);
        QVariantMap bare;
        bare[QStringLiteral("id")] = QStringLiteral("default");
        bare[QStringLiteral("name")] = QStringLiteral("default");
        bare[QStringLiteral("provider")] = QStringLiteral("daemon_api");
        bare[QStringLiteral("model")] = QString();
        bare[QStringLiteral("isDefault")] = false;
        m_model->upsert(bare);
    }
    [[nodiscard]] QObject* profiles() const override { return m_model; }
    [[nodiscard]] QString defaultProfileId() const override { return QStringLiteral("anthropic"); }
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
};
} // namespace

// Exercises the flattened agent-tree sidebar model: recursive flattening to
// arbitrary depth, the per-row tree roles, expand/collapse, and scope selection.
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

private slots:
    // The whole fleet-of-fleets is flattened with correct depths; nesting is
    // unbounded (worker sits three levels under its root).
    void flattensTreeWithDepths() {
        InMemorySessionStore store;
        SidebarModel model;
        model.setStore(&store);

        const int acme = findRow(model, QStringLiteral("Acme Platform"));
        const int build = findRow(model, QStringLiteral("Build Fleet"));
        const int coder = findRow(model, QStringLiteral("Coder"));
        const int worker = findRow(model, QStringLiteral("Worker A"));
        QVERIFY(acme >= 0 && build >= 0 && coder >= 0 && worker >= 0);

        QCOMPARE(roleAt<int>(model, acme, SidebarModel::DepthRole), 0);
        QCOMPARE(roleAt<int>(model, build, SidebarModel::DepthRole), 1);
        QCOMPARE(roleAt<int>(model, coder, SidebarModel::DepthRole), 2);
        QCOMPARE(roleAt<int>(model, worker, SidebarModel::DepthRole), 3);
    }

    // Tree roles are cosmetic-kind + structural flags; orchestrators carry
    // children, leaves do not.
    void exposesTreeRoles() {
        InMemorySessionStore store;
        SidebarModel model;
        model.setStore(&store);

        const int acme = findRow(model, QStringLiteral("Acme Platform"));
        const int coder = findRow(model, QStringLiteral("Coder"));

        // kind 2 == Orchestrator, kind 0 == Engine; nodeType 4 == Node.
        QCOMPARE(roleAt<int>(model, acme, SidebarModel::KindRole), 2);
        QCOMPARE(roleAt<int>(model, acme, SidebarModel::NodeTypeRole), 4);
        QVERIFY(roleAt<bool>(model, acme, SidebarModel::HasChildrenRole));
        QCOMPARE(roleAt<QString>(model, acme, SidebarModel::UnitIdRole), QStringLiteral("n-acme"));

        QCOMPARE(roleAt<int>(model, coder, SidebarModel::KindRole), 0);
        QVERIFY(!roleAt<bool>(model, coder, SidebarModel::HasChildrenRole));
    }

    // Collapsing a node hides its whole subtree; expanding restores it.
    void toggleExpandHidesAndRestoresSubtree() {
        InMemorySessionStore store;
        SidebarModel model;
        model.setStore(&store);

        const int before = model.rowCount();
        const int build = findRow(model, QStringLiteral("Build Fleet"));
        QVERIFY(build >= 0);
        QVERIFY(roleAt<bool>(model, build, SidebarModel::ExpandedRole));

        model.toggleExpand(build);
        // Build's subtree (coder, review, deep, worker) is gone.
        QVERIFY(findRow(model, QStringLiteral("Coder")) < 0);
        QVERIFY(findRow(model, QStringLiteral("Worker A")) < 0);
        QCOMPARE(model.rowCount(), before - 4);
        QVERIFY(!roleAt<bool>(model, findRow(model, QStringLiteral("Build Fleet")),
                              SidebarModel::ExpandedRole));

        model.toggleExpand(findRow(model, QStringLiteral("Build Fleet")));
        QVERIFY(findRow(model, QStringLiteral("Coder")) >= 0);
        QCOMPARE(model.rowCount(), before);
    }

    // Activating a node row selects its subtree scope (nodeType Node, the
    // agent id carried through).
    void activateEmitsNodeScope() {
        InMemorySessionStore store;
        SidebarModel model;
        model.setStore(&store);

        QSignalSpy spy(&model, &SidebarModel::scopeSelected);
        const int acme = findRow(model, QStringLiteral("Acme Platform"));
        model.activate(acme);

        QCOMPARE(spy.count(), 1);
        const QList<QVariant> args = spy.takeFirst();
        QCOMPARE(args.at(0).toInt(), 4); // NodeType::Unit
        QCOMPARE(args.at(2).toString(), QStringLiteral("n-acme"));
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

    // The highlight is identity-based: it follows the node across a model reset
    // rather than sticking to a row index. Collapsing a node that does NOT
    // contain the selection rebuilds the list but must leave Coder highlighted.
    void currentRoleTracksIdentityAcrossRebuild() {
        InMemorySessionStore store;
        SidebarModel model;
        model.setStore(&store);

        const int coder = findRow(model, QStringLiteral("Coder"));
        model.activate(coder);
        QVERIFY(roleAt<bool>(model, coder, SidebarModel::CurrentRole));

        // Deep Fleet is a sibling branch under Build, not an ancestor of Coder,
        // so collapsing it triggers a full rebuild without hiding Coder.
        model.toggleExpand(findRow(model, QStringLiteral("Deep Fleet")));
        QVERIFY(findRow(model, QStringLiteral("Worker A")) < 0); // the rebuild happened
        const int coderAfter = findRow(model, QStringLiteral("Coder"));
        QVERIFY(coderAfter >= 0);
        QVERIFY(roleAt<bool>(model, coderAfter, SidebarModel::CurrentRole));
        QCOMPARE(model.currentRow(), coderAfter);
    }

    // Collapsing an ancestor of the selection lifts the selection up to the
    // collapsed node (VSCode behavior) and re-emits its scope.
    void collapsingAncestorMovesSelectionUp() {
        InMemorySessionStore store;
        SidebarModel model;
        model.setStore(&store);

        const int worker = findRow(model, QStringLiteral("Worker A"));
        model.activate(worker);

        QSignalSpy spy(&model, &SidebarModel::scopeSelected);
        const int build = findRow(model, QStringLiteral("Build Fleet"));
        model.toggleExpand(build); // hides Worker A's branch

        QVERIFY(findRow(model, QStringLiteral("Worker A")) < 0);
        const int buildAfter = findRow(model, QStringLiteral("Build Fleet"));
        QVERIFY(roleAt<bool>(model, buildAfter, SidebarModel::CurrentRole));
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.takeFirst().at(2).toString(), QStringLiteral("n-build"));
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

    // Right expands a collapsed node then descends; Left collapses then climbs.
    void arrowKeysExpandCollapseAndTraverse() {
        InMemorySessionStore store;
        SidebarModel model;
        model.setStore(&store);

        const int build = findRow(model, QStringLiteral("Build Fleet"));
        model.activate(build);

        model.collapseCurrent(); // collapse Build (selection stays)
        QVERIFY(!roleAt<bool>(model, findRow(model, QStringLiteral("Build Fleet")),
                              SidebarModel::ExpandedRole));
        QVERIFY(findRow(model, QStringLiteral("Coder")) < 0);

        model.expandCurrent(); // re-expand Build
        QVERIFY(roleAt<bool>(model, findRow(model, QStringLiteral("Build Fleet")),
                             SidebarModel::ExpandedRole));

        model.expandCurrent(); // already expanded -> step to first child
        QCOMPARE(model.currentRow(), findRow(model, QStringLiteral("Coder")));

        model.collapseCurrent(); // leaf -> climb to parent (Build)
        QCOMPARE(model.currentRow(), findRow(model, QStringLiteral("Build Fleet")));
    }

    // Collapse-all hides every subtree (roots remain); expand-all restores them.
    void expandAllAndCollapseAllToggleWholeTree() {
        InMemorySessionStore store;
        SidebarModel model;
        model.setStore(&store);

        QVERIFY(model.anyExpanded());

        model.collapseAll();
        QVERIFY(!model.anyExpanded());
        QVERIFY(findRow(model, QStringLiteral("Coder")) < 0);
        QVERIFY(findRow(model, QStringLiteral("Build Fleet")) < 0);
        QVERIFY(findRow(model, QStringLiteral("Acme Platform")) >= 0); // root stays

        model.expandAll();
        QVERIFY(model.anyExpanded());
        QVERIFY(findRow(model, QStringLiteral("Worker A")) >= 0);
    }

    // collapseAll while a deep node is selected lifts the highlight to its root.
    void collapseAllLiftsSelectionToRoot() {
        InMemorySessionStore store;
        SidebarModel model;
        model.setStore(&store);

        model.activate(findRow(model, QStringLiteral("Worker A")));
        model.collapseAll();

        const int acme = findRow(model, QStringLiteral("Acme Platform"));
        QVERIFY(roleAt<bool>(model, acme, SidebarModel::CurrentRole));
    }

    // Creating a root node adds a top-level row and selects it.
    void createRootNodeAddsAndSelects() {
        InMemorySessionStore store;
        SidebarModel model;
        model.setStore(&store);

        QSignalSpy spy(&model, &SidebarModel::scopeSelected);
        model.createRootUnit();

        const int created = findRow(model, QStringLiteral("New fleet"));
        QVERIFY(created >= 0);
        QCOMPARE(roleAt<int>(model, created, SidebarModel::DepthRole), 0);
        QVERIFY(roleAt<bool>(model, created, SidebarModel::CurrentRole));
        QCOMPARE(spy.takeLast().at(0).toInt(), 4); // NodeType::Unit
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
        FakeProfileStore profiles;
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

    // Folding the Fleet header hides its whole unit body while the header stays,
    // and its ExpandedRole flips; other sections are untouched.
    void fleetSectionHeaderCollapses() {
        InMemorySessionStore store;
        SidebarModel model;
        model.setStore(&store);

        const int fleet = findRow(model, QStringLiteral("Fleet"));
        QVERIFY(fleet >= 0);
        QVERIFY(roleAt<bool>(model, fleet, SidebarModel::IsSeparatorRole));
        QVERIFY(roleAt<bool>(model, fleet, SidebarModel::HasChildrenRole));
        QVERIFY(roleAt<bool>(model, fleet, SidebarModel::ExpandedRole));
        QVERIFY(findRow(model, QStringLiteral("Acme Platform")) >= 0);

        model.toggleExpand(fleet);

        // Body gone, header stays and reads collapsed.
        const int fleetAfter = findRow(model, QStringLiteral("Fleet"));
        QVERIFY(fleetAfter >= 0);
        QVERIFY(!roleAt<bool>(model, fleetAfter, SidebarModel::ExpandedRole));
        QVERIFY(findRow(model, QStringLiteral("Acme Platform")) < 0);
        QVERIFY(findRow(model, QStringLiteral("Coder")) < 0);
        // Other sections remain.
        QVERIFY(findRow(model, QStringLiteral("Tags")) >= 0);
        QVERIFY(findRow(model, QStringLiteral("All Sessions")) >= 0);

        // Re-expand restores the body.
        model.toggleExpand(findRow(model, QStringLiteral("Fleet")));
        QVERIFY(findRow(model, QStringLiteral("Acme Platform")) >= 0);
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
        // Fleet is unaffected.
        QVERIFY(findRow(model, QStringLiteral("Acme Platform")) >= 0);
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
        InMemorySessionStore store;
        MockDaemonNet net;
        SidebarModel model;
        model.setStore(&store);
        model.setDaemonNet(&net);

        // Fold one transport account by hand.
        model.toggleExpand(findRow(model, QStringLiteral("matrix /@bot:hs.org")));
        QVERIFY(findRow(model, QStringLiteral("Channels")) < 0);

        // Fleet expand-all must leave that transport account folded.
        model.expandAll();
        QVERIFY(findRow(model, QStringLiteral("Channels")) < 0);

        // Conversely, collapse a fleet unit then Integrations expand-all keeps it folded.
        model.toggleExpand(findRow(model, QStringLiteral("Build Fleet")));
        QVERIFY(findRow(model, QStringLiteral("Coder")) < 0);
        model.expandAllTransports();
        QVERIFY(findRow(model, QStringLiteral("Coder")) < 0);
    }
};

QTEST_MAIN(TestSidebarModel)
#include "tst_sidebar_model.moc"
