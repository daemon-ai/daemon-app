// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "account_management_controller.h"
#include "integrations_tree_model.h"
#include "mirror/mirror_service.h"
#include "mirror/seeder.h"
#include "transports/itransport_registry.h"

#include <QSignalSpy>
#include <QtTest>

using transports::ITransportRegistry;

namespace {

QVariantMap conv(const QString& transport, const QString& id, const QString& title,
                 const QString& kind, const QString& parent) {
    QVariantMap m;
    m[QStringLiteral("transport")] = transport;
    m[QStringLiteral("id")] = id;
    m[QStringLiteral("title")] = title;
    m[QStringLiteral("kind")] = kind;
    m[QStringLiteral("parent")] = parent;
    return m;
}

QVariantMap ops(std::initializer_list<std::pair<QString, bool>> flags) {
    QVariantMap m;
    for (const auto& f : flags) {
        m[f.first] = f.second;
    }
    return m;
}

QVariantMap field(const QString& key, const QString& label, bool required, const QString& kind,
                  const QString& def = {}, const QString& placeholder = {},
                  const QVariantList& choices = {}) {
    QVariantMap m;
    m[QStringLiteral("key")] = key;
    m[QStringLiteral("label")] = label;
    m[QStringLiteral("required")] = required;
    m[QStringLiteral("kind")] = kind;
    m[QStringLiteral("default")] = def;
    m[QStringLiteral("placeholder")] = placeholder;
    m[QStringLiteral("choices")] = choices;
    return m;
}

// A programmable ITransportRegistry: the test drives the exact instances / adapters / conversations
// / settings shapes the integrations tree + account form must render, and records the mutating
// verbs (configure / remove) so masked-field omission and teardown are asserted directly.
class FakeRegistry : public ITransportRegistry {
public:
    using ITransportRegistry::ITransportRegistry;

    QVariantList m_adapters;
    QVariantList m_instances;
    QHash<QString, QVariantList> m_conversations;
    QHash<QString, QVariantMap> m_settings;

    // Recorded writes.
    QString lastConfigureTransport;
    QVariantMap lastConfigureValues;
    int configureCount = 0;
    QString lastRemoved;
    QString lastConnected;
    QHash<QString, bool> enabledCalls;

    [[nodiscard]] QVariantList availableAdapters() const override { return m_adapters; }
    [[nodiscard]] QVariantList instances() const override { return m_instances; }
    [[nodiscard]] QVariantList conversations(const QString& transport) const override {
        return m_conversations.value(transport);
    }
    [[nodiscard]] QVariantMap settings(const QString& transport) const override {
        return m_settings.value(transport);
    }
    void refreshSettings(const QString& transport) override {
        // Emit the stored non-secret values (secrets are never returned).
        emit settingsChanged(transport, m_settings.value(transport));
    }
    void configure(const QString& transport, const QVariantMap& values) override {
        lastConfigureTransport = transport;
        lastConfigureValues = values;
        ++configureCount;
    }
    void remove(const QString& transport) override { lastRemoved = transport; }
    void connectAccount(const QString& transport) override { lastConnected = transport; }
    void setEnabled(const QString& transport, bool enabled) override {
        enabledCalls.insert(transport, enabled);
    }
};

// A programmable IPersonsService: canned person rows with per-transport endpoints.
QVariantMap adapterRow(const QString& family, const QString& displayName, bool rooms, bool dms,
                       bool presence, bool rosterList, bool directory,
                       const QVariantList& schema = {}) {
    QVariantMap caps;
    caps[QStringLiteral("rooms")] = rooms;
    caps[QStringLiteral("directMessages")] = dms;
    caps[QStringLiteral("presence")] = presence;
    QVariantMap m;
    m[QStringLiteral("family")] = family;
    m[QStringLiteral("displayName")] = displayName;
    m[QStringLiteral("capabilities")] = caps;
    m[QStringLiteral("schema")] = schema;
    m[QStringLiteral("rosterOps")] = ops({{QStringLiteral("list"), rosterList}});
    m[QStringLiteral("directory")] = directory;
    return m;
}

QVariantMap instanceRow(const QString& transport, const QString& family, const QString& displayName,
                        bool enabled, const QString& connectionReason) {
    QVariantMap m;
    m[QStringLiteral("transport")] = transport;
    m[QStringLiteral("family")] = family;
    m[QStringLiteral("displayName")] = displayName;
    m[QStringLiteral("label")] = QString();
    m[QStringLiteral("enabled")] = enabled;
    m[QStringLiteral("connectionReason")] = connectionReason;
    m[QStringLiteral("boundProfile")] = QString();
    return m;
}

QVariantMap person(const QString& id, const QString& alias, const QString& transport,
                   const QString& endpointId, const QString& displayName, const QString& presence) {
    QVariantMap ep;
    ep[QStringLiteral("transport")] = transport;
    ep[QStringLiteral("id")] = endpointId;
    ep[QStringLiteral("displayName")] = displayName;
    ep[QStringLiteral("presence")] = presence;
    QVariantMap p;
    p[QStringLiteral("id")] = id;
    p[QStringLiteral("alias")] = alias;
    p[QStringLiteral("endpoints")] = QVariantList{ep};
    return p;
}

// The MIRROR twin of the two-account seed: a Matrix-shaped account (spaces + rooms + DMs +
// persons + directory) alongside a flat IRC-shaped account (rooms only, no DMs, no persons, no
// directory) — enough to exercise every gating rule and the space/parent nesting incl. the
// unknown-parent-as-root fallback. Seeded through the same apply pipeline production feeds (§9).
void seedTwoAccounts(mirror::MirrorService& svc) {
    mirror::SeedSet seed;
    const auto adapter = [](const char* family, const char* name, bool dms, bool directory) {
        mirror::Adapter a;
        a.family = QLatin1String(family);
        a.display_name = QLatin1String(name);
        a.cap_rooms = true;
        a.cap_direct_messages = dms;
        a.directory = directory;
        return a;
    };
    seed.adapters = {adapter("matrix", "Matrix", true, true), adapter("irc", "IRC", false, false)};
    const auto account = [](const char* transport, const char* family, const char* name,
                            bool enabled, const char* reason) {
        mirror::TransportAccount t;
        t.transport = QLatin1String(transport);
        t.family = QLatin1String(family);
        t.display_name = QString::fromUtf8(name);
        t.enabled = enabled;
        t.reason = QLatin1String(reason);
        return t;
    };
    seed.transportAccounts = {
        account("matrix/@you", "matrix", "Matrix (@you)", true, "ready"),
        account("irc/libera", "irc", "IRC (libera)", false, ""),
    };
    const auto conv = [](const char* transport, const char* id, const char* title, const char* kind,
                         const char* parent) {
        mirror::Conversation c;
        c.transport = QLatin1String(transport);
        c.id = QLatin1String(id);
        c.title = QString::fromUtf8(title);
        c.kind = QLatin1String(kind);
        c.parent = QLatin1String(parent);
        return c;
    };
    seed.conversations = {
        conv("matrix/@you", "!space1", "Demo Server", "space", ""),
        conv("matrix/@you", "!general", "general", "channel", "!space1"),
        conv("matrix/@you", "!random", "random", "channel", "!space1"),
        conv("matrix/@you", "!standalone", "standalone", "channel", ""),
        conv("matrix/@you", "!orphan", "orphan", "channel", "!missing"),
        conv("matrix/@you", "!dmAlice", "Alice", "dm", ""),
        conv("matrix/@you", "!grpDesign", "Design", "group_dm", ""),
        conv("irc/libera", "#daemon", "#daemon", "channel", ""),
    };
    mirror::Person bob;
    bob.id = QStringLiteral("p-bob");
    bob.alias = QStringLiteral("Bob");
    bob.endpoint_count = 1;
    seed.persons = {bob};
    mirror::PersonEndpoint bobEp;
    bobEp.person = QStringLiteral("p-bob");
    bobEp.transport = QStringLiteral("matrix/@you");
    bobEp.contact = QStringLiteral("@bob");
    bobEp.display_name = QStringLiteral("Bob");
    bobEp.presence_primitive = QStringLiteral("available");
    seed.personEndpoints = {bobEp};
    mirror::Seeder seeder(svc.store());
    seeder.seed(seed);
}

// The registry-shaped seed the ACCOUNT-CONTROLLER cases still drive (the wizard reads schemas +
// records verbs off the registry seam, which survives as the daemon verb/settings sink).
void seedTwoAccounts(FakeRegistry& reg) {
    reg.m_adapters = QVariantList{
        adapterRow(QStringLiteral("matrix"), QStringLiteral("Matrix"), true, true, true, true,
                   true),
        adapterRow(QStringLiteral("irc"), QStringLiteral("IRC"), true, false, false, false, false),
    };
    reg.m_instances = QVariantList{
        instanceRow(QStringLiteral("matrix/@you"), QStringLiteral("matrix"),
                    QStringLiteral("Matrix (@you)"), true, QStringLiteral("ready")),
        instanceRow(QStringLiteral("irc/libera"), QStringLiteral("irc"),
                    QStringLiteral("IRC (libera)"), false, QString()),
    };
    reg.m_conversations[QStringLiteral("matrix/@you")] = QVariantList{
        conv(QStringLiteral("matrix/@you"), QStringLiteral("!space1"),
             QStringLiteral("Demo Server"), QStringLiteral("space"), QString()),
        conv(QStringLiteral("matrix/@you"), QStringLiteral("!general"), QStringLiteral("general"),
             QStringLiteral("channel"), QStringLiteral("!space1")),
        conv(QStringLiteral("matrix/@you"), QStringLiteral("!random"), QStringLiteral("random"),
             QStringLiteral("channel"), QStringLiteral("!space1")),
        conv(QStringLiteral("matrix/@you"), QStringLiteral("!standalone"),
             QStringLiteral("standalone"), QStringLiteral("channel"), QString()),
        conv(QStringLiteral("matrix/@you"), QStringLiteral("!orphan"), QStringLiteral("orphan"),
             QStringLiteral("channel"), QStringLiteral("!missing")),
        conv(QStringLiteral("matrix/@you"), QStringLiteral("!dmAlice"), QStringLiteral("Alice"),
             QStringLiteral("dm"), QString()),
        conv(QStringLiteral("matrix/@you"), QStringLiteral("!grpDesign"), QStringLiteral("Design"),
             QStringLiteral("groupdm"), QString()),
    };
    reg.m_conversations[QStringLiteral("irc/libera")] = QVariantList{
        conv(QStringLiteral("irc/libera"), QStringLiteral("#daemon"), QStringLiteral("#daemon"),
             QStringLiteral("channel"), QString()),
    };
}

} // namespace

// A2 work package: the integrations tree (protocol-governed account subtree) + the schema-driven
// add/edit/remove account controller. Both are the shared C++ view-model the GUI + TUI render.
class TestIntegrationsTree : public QObject {
    Q_OBJECT

    static int findRow(const IntegrationsTreeModel& m, const QString& label) {
        for (int i = 0; i < m.rowCount(); ++i) {
            if (m.data(m.index(i, 0), IntegrationsTreeModel::LabelRole).toString() == label) {
                return i;
            }
        }
        return -1;
    }
    template <typename T>
    static T roleAt(const IntegrationsTreeModel& m, int row, IntegrationsTreeModel::Role role) {
        return m.data(m.index(row, 0), role).template value<T>();
    }
    static QString kindAt(const IntegrationsTreeModel& m, int row) {
        return roleAt<QString>(m, row, IntegrationsTreeModel::RowKindRole);
    }

private slots:
    // --- Tree composition ----------------------------------------------------

    // Each configured account is a depth-0 ROOT row carrying family + enabled + connection token;
    // the two seeded accounts render in instances() order.
    void accountsAreRoots() {
        mirror::MirrorService svc;
        svc.openInMemory();
        seedTwoAccounts(svc);
        IntegrationsTreeModel model;
        model.setMirror(&svc);

        const int matrix = findRow(model, QStringLiteral("Matrix (@you)"));
        const int irc = findRow(model, QStringLiteral("IRC (libera)"));
        QVERIFY(matrix >= 0 && irc >= 0);
        // AD (1a.3): the mirror projection orders accounts by transport id (deterministic —
        // "irc/libera" < "matrix/@you"), replacing the legacy registry-listing order.
        QVERIFY2(irc < matrix, "accounts render transport-sorted");
        QCOMPARE(roleAt<int>(model, matrix, IntegrationsTreeModel::DepthRole), 0);
        QCOMPARE(kindAt(model, matrix), QStringLiteral("account"));
        QCOMPARE(roleAt<QString>(model, matrix, IntegrationsTreeModel::FamilyRole),
                 QStringLiteral("matrix"));
        QCOMPARE(roleAt<QString>(model, matrix, IntegrationsTreeModel::TransportRole),
                 QStringLiteral("matrix/@you"));
        QVERIFY(roleAt<bool>(model, matrix, IntegrationsTreeModel::EnabledRole));
        QCOMPARE(roleAt<QString>(model, matrix, IntegrationsTreeModel::ConnectionRole),
                 QStringLiteral("ready"));
        QVERIFY(!roleAt<bool>(model, irc, IntegrationsTreeModel::EnabledRole));
        QVERIFY(roleAt<bool>(model, matrix, IntegrationsTreeModel::HasChildrenRole));
    }

    // A space (kind=="space") nests its member conversations (those whose parent == its id) one
    // level deeper; the protocol governs this — flat protocols never emit spaces.
    void spaceNestsChildRooms() {
        mirror::MirrorService svc;
        svc.openInMemory();
        seedTwoAccounts(svc);
        IntegrationsTreeModel model;
        model.setMirror(&svc);

        const int space = findRow(model, QStringLiteral("Demo Server"));
        QVERIFY(space >= 0);
        QCOMPARE(kindAt(model, space), QStringLiteral("space"));
        QCOMPARE(roleAt<QString>(model, space, IntegrationsTreeModel::ConvTypeRole),
                 QStringLiteral("space"));
        const int spaceDepth = roleAt<int>(model, space, IntegrationsTreeModel::DepthRole);

        const int general = findRow(model, QStringLiteral("general"));
        const int random = findRow(model, QStringLiteral("random"));
        QVERIFY(general >= 0 && random >= 0);
        // Child rooms sit directly under the space, one level deeper.
        QCOMPARE(roleAt<int>(model, general, IntegrationsTreeModel::DepthRole), spaceDepth + 1);
        QVERIFY(general > space && random > space);
        QCOMPARE(roleAt<QString>(model, general, IntegrationsTreeModel::ConversationRole),
                 QStringLiteral("!general"));
    }

    // A conversation whose parent points at a non-existent space is treated as a ROOT (not
    // dropped): it lands under the standalone Channels group, next to the genuinely-parentless
    // channel.
    void unknownParentTreatedAsRoot() {
        mirror::MirrorService svc;
        svc.openInMemory();
        seedTwoAccounts(svc);
        IntegrationsTreeModel model;
        model.setMirror(&svc);

        const int channels = findRow(model, QStringLiteral("Channels"));
        QVERIFY(channels >= 0);
        QCOMPARE(kindAt(model, channels), QStringLiteral("section"));
        const int channelsDepth = roleAt<int>(model, channels, IntegrationsTreeModel::DepthRole);

        const int standalone = findRow(model, QStringLiteral("standalone"));
        const int orphan = findRow(model, QStringLiteral("orphan"));
        QVERIFY(standalone >= 0 && orphan >= 0);
        // Both are children of the Channels group (one level deeper), NOT of the space.
        QCOMPARE(roleAt<int>(model, standalone, IntegrationsTreeModel::DepthRole),
                 channelsDepth + 1);
        QCOMPARE(roleAt<int>(model, orphan, IntegrationsTreeModel::DepthRole), channelsDepth + 1);
        // The orphan is NOT nested under Demo Server.
        const int space = findRow(model, QStringLiteral("Demo Server"));
        QVERIFY(orphan > channels && (space < 0 || orphan < space || orphan > channels));
    }

    // Direct messages (dm + groupdm) group under a "Direct Messages" section, gated on the
    // adapter's directMessages capability.
    void directMessagesGroup() {
        mirror::MirrorService svc;
        svc.openInMemory();
        seedTwoAccounts(svc);
        IntegrationsTreeModel model;
        model.setMirror(&svc);

        const int dms = findRow(model, QStringLiteral("Direct Messages"));
        QVERIFY(dms >= 0);
        const int alice = findRow(model, QStringLiteral("Alice"));
        const int design = findRow(model, QStringLiteral("Design"));
        QVERIFY(alice >= 0 && design >= 0);
        QCOMPARE(roleAt<QString>(model, alice, IntegrationsTreeModel::ConvTypeRole),
                 QStringLiteral("dm"));
        QCOMPARE(roleAt<QString>(model, design, IntegrationsTreeModel::ConvTypeRole),
                 QStringLiteral("group_dm")); // the mirror vocabulary (map_conversation)
    }

    // The Persons section renders the endpoints reachable on THIS transport (a cross-transport
    // identity). It appears only for the account whose transport an endpoint names.
    void personsSectionScopedToTransport() {
        mirror::MirrorService svc;
        svc.openInMemory();
        seedTwoAccounts(svc);
        IntegrationsTreeModel model;
        model.setMirror(&svc);

        const int personsHeader = findRow(model, QStringLiteral("Persons"));
        QVERIFY(personsHeader >= 0);
        QCOMPARE(roleAt<QString>(model, personsHeader, IntegrationsTreeModel::TransportRole),
                 QStringLiteral("matrix/@you"));
        const int bob = findRow(model, QStringLiteral("Bob"));
        QVERIFY(bob >= 0);
        QCOMPARE(kindAt(model, bob), QStringLiteral("person"));
        QCOMPARE(roleAt<QString>(model, bob, IntegrationsTreeModel::PersonIdRole),
                 QStringLiteral("p-bob"));
    }

    // Per-protocol section gating: the flat IRC account (no DMs, no persons on it, no directory)
    // shows a Channels group but NO Direct Messages / Persons / Browse sections, while Matrix shows
    // all of them. "Each protocol governs how its tree is rendered."
    void protocolGovernsSections() {
        mirror::MirrorService svc;
        svc.openInMemory();
        seedTwoAccounts(svc);
        IntegrationsTreeModel model;
        model.setMirror(&svc);

        // Collapse Matrix so only IRC's subtree remains for the negative assertions.
        model.toggleExpand(findRow(model, QStringLiteral("Matrix (@you)")));
        QVERIFY(findRow(model, QStringLiteral("#daemon")) >= 0);        // IRC channel present
        QVERIFY(findRow(model, QStringLiteral("Direct Messages")) < 0); // gated off (no DM cap)
        QVERIFY(findRow(model, QStringLiteral("Persons")) < 0);         // no endpoint on irc
        QVERIFY(findRow(model, QStringLiteral("Browse")) < 0);          // no directory cap

        // Re-expand Matrix: it advertises DMs + directory + a person endpoint, so all appear.
        model.toggleExpand(findRow(model, QStringLiteral("Matrix (@you)")));
        QVERIFY(findRow(model, QStringLiteral("Direct Messages")) >= 0);
        QVERIFY(findRow(model, QStringLiteral("Persons")) >= 0);
        QVERIFY(findRow(model, QStringLiteral("Browse")) >= 0);
    }

    // --- Expand / collapse ---------------------------------------------------

    // Collapsing an account hides its ENTIRE subtree (sections, spaces, rooms, DMs); the sibling
    // account is untouched; re-expanding restores it.
    void collapsingAccountHidesSubtree() {
        mirror::MirrorService svc;
        svc.openInMemory();
        seedTwoAccounts(svc);
        IntegrationsTreeModel model;
        model.setMirror(&svc);

        QVERIFY(findRow(model, QStringLiteral("Demo Server")) >= 0);
        model.toggleExpand(findRow(model, QStringLiteral("Matrix (@you)")));
        QVERIFY(findRow(model, QStringLiteral("Demo Server")) < 0);
        QVERIFY(findRow(model, QStringLiteral("general")) < 0);
        QVERIFY(findRow(model, QStringLiteral("Alice")) < 0);
        // The IRC account and its channel remain.
        QVERIFY(findRow(model, QStringLiteral("IRC (libera)")) >= 0);
        QVERIFY(findRow(model, QStringLiteral("#daemon")) >= 0);

        model.toggleExpand(findRow(model, QStringLiteral("Matrix (@you)")));
        QVERIFY(findRow(model, QStringLiteral("Demo Server")) >= 0);
    }

    // Collapsing a space hides only its child rooms (the space row stays); a sibling section is
    // unaffected.
    void collapsingSpaceHidesRooms() {
        mirror::MirrorService svc;
        svc.openInMemory();
        seedTwoAccounts(svc);
        IntegrationsTreeModel model;
        model.setMirror(&svc);

        model.toggleExpand(findRow(model, QStringLiteral("Demo Server")));
        QVERIFY(findRow(model, QStringLiteral("Demo Server")) >= 0);
        QVERIFY(findRow(model, QStringLiteral("general")) < 0);
        QVERIFY(findRow(model, QStringLiteral("random")) < 0);
        // The standalone Channels group is a sibling and stays.
        QVERIFY(findRow(model, QStringLiteral("standalone")) >= 0);
    }

    // expandAll / collapseAll fold every account+space+section; anyExpanded tracks it.
    void expandAllCollapseAll() {
        mirror::MirrorService svc;
        svc.openInMemory();
        seedTwoAccounts(svc);
        IntegrationsTreeModel model;
        model.setMirror(&svc);

        QVERIFY(model.anyExpanded());
        model.collapseAll();
        QVERIFY(!model.anyExpanded());
        // Accounts remain (they are the roots); their subtrees are gone.
        QVERIFY(findRow(model, QStringLiteral("Matrix (@you)")) >= 0);
        QVERIFY(findRow(model, QStringLiteral("Demo Server")) < 0);
        QVERIFY(findRow(model, QStringLiteral("#daemon")) < 0);

        model.expandAll();
        QVERIFY(model.anyExpanded());
        QVERIFY(findRow(model, QStringLiteral("general")) >= 0);
    }

    // --- Activation intents (routed to A4's activation seam) -----------------

    // Activating a conversation node (room / channel / DM) emits conversationActivated with its
    // transport + conversation id — the single "user activated a conversation" seam A4 consumes.
    void activatingConversationEmitsActivated() {
        mirror::MirrorService svc;
        svc.openInMemory();
        seedTwoAccounts(svc);
        IntegrationsTreeModel model;
        model.setMirror(&svc);

        QSignalSpy opened(&model, &IntegrationsTreeModel::conversationActivated);
        model.activate(findRow(model, QStringLiteral("general")));
        QCOMPARE(opened.count(), 1);
        const QList<QVariant> a = opened.takeFirst();
        QCOMPARE(a.at(0).toString(), QStringLiteral("matrix/@you"));
        QCOMPARE(a.at(1).toString(), QStringLiteral("!general"));

        model.activate(findRow(model, QStringLiteral("Alice")));
        QCOMPARE(opened.count(), 1);
        QCOMPARE(opened.takeFirst().at(1).toString(), QStringLiteral("!dmAlice"));
    }

    // Activating an account root emits accountActivated (highlight/scope), not
    // conversationActivated; activating the Browse action emits browseRequested.
    void activatingAccountAndBrowse() {
        mirror::MirrorService svc;
        svc.openInMemory();
        seedTwoAccounts(svc);
        IntegrationsTreeModel model;
        model.setMirror(&svc);

        QSignalSpy account(&model, &IntegrationsTreeModel::accountActivated);
        QSignalSpy conv(&model, &IntegrationsTreeModel::conversationActivated);
        QSignalSpy browse(&model, &IntegrationsTreeModel::browseRequested);

        model.activate(findRow(model, QStringLiteral("Matrix (@you)")));
        QCOMPARE(account.count(), 1);
        QCOMPARE(account.takeFirst().at(0).toString(), QStringLiteral("matrix/@you"));
        QCOMPARE(conv.count(), 0);

        model.activate(findRow(model, QStringLiteral("Browse")));
        QCOMPARE(browse.count(), 1);
        QCOMPARE(browse.takeFirst().at(0).toString(), QStringLiteral("matrix/@you"));
    }

    // A section header (Persons / Channels / Direct Messages) is not selectable and routes nothing.
    void sectionHeadersInert() {
        mirror::MirrorService svc;
        svc.openInMemory();
        seedTwoAccounts(svc);
        IntegrationsTreeModel model;
        model.setMirror(&svc);

        const int channels = findRow(model, QStringLiteral("Channels"));
        QVERIFY(!roleAt<bool>(model, channels, IntegrationsTreeModel::SelectableRole));
        QSignalSpy conv(&model, &IntegrationsTreeModel::conversationActivated);
        model.activate(channels);
        QCOMPARE(conv.count(), 0);
    }

    // The account context intents fan out: requestRemoveAccount -> removeAccountRequested,
    // requestEditAccount -> editAccountRequested, requestAddAccount -> addAccountRequested, while
    // connect/enable go straight to the registry (ControlApi-level).
    void accountContextIntents() {
        mirror::MirrorService svc;
        svc.openInMemory();
        seedTwoAccounts(svc);
        FakeRegistry reg; // the VERB sink (connect/enable route here; reads never do)
        IntegrationsTreeModel model;
        model.setMirror(&svc);
        model.setRegistry(&reg);

        QSignalSpy add(&model, &IntegrationsTreeModel::addAccountRequested);
        QSignalSpy edit(&model, &IntegrationsTreeModel::editAccountRequested);
        QSignalSpy rem(&model, &IntegrationsTreeModel::removeAccountRequested);
        model.requestAddAccount();
        model.requestEditAccount(QStringLiteral("matrix/@you"));
        model.requestRemoveAccount(QStringLiteral("matrix/@you"));
        QCOMPARE(add.count(), 1);
        QCOMPARE(edit.takeFirst().at(0).toString(), QStringLiteral("matrix/@you"));
        QCOMPARE(rem.takeFirst().at(0).toString(), QStringLiteral("matrix/@you"));

        model.connectAccount(QStringLiteral("irc/libera"));
        QCOMPARE(reg.lastConnected, QStringLiteral("irc/libera"));
        model.setAccountEnabled(QStringLiteral("irc/libera"), true);
        QCOMPARE(reg.enabledCalls.value(QStringLiteral("irc/libera")), true);
    }

    // The tree re-composes when a mirror delta lands for a tree-relevant kind (journal-filtered
    // committed wiring) — no manual refresh, identical in daemon and mock (§9).
    void rebuildsOnMirrorDelta() {
        mirror::MirrorService svc;
        svc.openInMemory();
        seedTwoAccounts(svc);
        IntegrationsTreeModel model;
        model.setMirror(&svc);

        QVERIFY(findRow(model, QStringLiteral("#daemon")) >= 0);
        mirror::Conversation fresh;
        fresh.transport = QStringLiteral("irc/libera");
        fresh.id = QStringLiteral("#new");
        fresh.title = QStringLiteral("#new");
        fresh.kind = QStringLiteral("channel");
        mirror::Seeder seeder(svc.store());
        seeder.upsertConversation(fresh);
        QVERIFY(findRow(model, QStringLiteral("#new")) >= 0);
    }

    // --- Schema-driven account management ------------------------------------

    // The protocol picker lists every adapter family (family + displayName).
    void availableFamiliesFromAdapters() {
        FakeRegistry reg;
        seedTwoAccounts(reg);
        AccountManagementController ctrl;
        ctrl.setRegistry(&reg);

        const QVariantList fams = ctrl.availableFamilies();
        QCOMPARE(fams.size(), 2);
        QStringList names;
        for (const QVariant& f : fams) {
            names << f.toMap().value(QStringLiteral("family")).toString();
        }
        QVERIFY(names.contains(QStringLiteral("matrix")));
        QVERIFY(names.contains(QStringLiteral("irc")));
    }

    // beginAdd seeds the schema fields from the family's account_schema with defaults/placeholders;
    // secrets start blank; the mode + family reflect the add flow.
    void beginAddSeedsSchemaFields() {
        FakeRegistry reg;
        reg.m_adapters = QVariantList{adapterRow(
            QStringLiteral("matrix"), QStringLiteral("Matrix"), true, true, true, true, true,
            QVariantList{field(QStringLiteral("homeserver"), QStringLiteral("Homeserver"), true,
                               QStringLiteral("Text"), QStringLiteral("https://matrix.org"),
                               QStringLiteral("https://...")),
                         field(QStringLiteral("token"), QStringLiteral("Access token"), true,
                               QStringLiteral("Password"))})};
        AccountManagementController ctrl;
        ctrl.setRegistry(&reg);

        QSignalSpy state(&ctrl, &AccountManagementController::stateChanged);
        ctrl.beginAdd(QStringLiteral("matrix"));
        QVERIFY(state.count() >= 1);
        QCOMPARE(ctrl.mode(), QStringLiteral("add"));
        QCOMPARE(ctrl.family(), QStringLiteral("matrix"));

        const QVariantList f = ctrl.fields();
        QCOMPARE(f.size(), 2);
        const QVariantMap homeserver = f.at(0).toMap();
        QCOMPARE(homeserver.value(QStringLiteral("key")).toString(), QStringLiteral("homeserver"));
        QCOMPARE(homeserver.value(QStringLiteral("kind")).toString(), QStringLiteral("Text"));
        // Non-secret field prefilled from its default; the secret starts blank.
        QCOMPARE(homeserver.value(QStringLiteral("value")).toString(),
                 QStringLiteral("https://matrix.org"));
        QCOMPARE(f.at(1).toMap().value(QStringLiteral("kind")).toString(),
                 QStringLiteral("Password"));
        QVERIFY(f.at(1).toMap().value(QStringLiteral("value")).toString().isEmpty());
    }

    // submitAdd hands off to the auth flow (A3) with the collected fields and does NOT configure a
    // brand-new account directly (a fresh account is a sign-in).
    void submitAddHandsOffToAuthFlow() {
        FakeRegistry reg;
        reg.m_adapters = QVariantList{adapterRow(
            QStringLiteral("matrix"), QStringLiteral("Matrix"), true, true, true, true, true,
            QVariantList{field(QStringLiteral("homeserver"), QStringLiteral("Homeserver"), true,
                               QStringLiteral("Text"))})};
        AccountManagementController ctrl;
        ctrl.setRegistry(&reg);
        ctrl.beginAdd(QStringLiteral("matrix"));

        QSignalSpy handoff(&ctrl, &AccountManagementController::authFlowRequested);
        QVariantMap values;
        values[QStringLiteral("homeserver")] = QStringLiteral("https://example.org");
        ctrl.submitAdd(values);

        QCOMPARE(handoff.count(), 1);
        const QList<QVariant> a = handoff.takeFirst();
        QCOMPARE(a.at(0).toString(), QStringLiteral("matrix"));
        QCOMPARE(a.at(1).toMap().value(QStringLiteral("homeserver")).toString(),
                 QStringLiteral("https://example.org"));
        QCOMPARE(reg.configureCount, 0); // never configures a fresh account
    }

    // beginEdit seeds the schema fields, then TransportSettings (settingsChanged) patches the
    // current NON-SECRET values in place; the secret field stays blank (secrets never come back).
    void beginEditPatchesCurrentValues() {
        FakeRegistry reg;
        reg.m_adapters = QVariantList{adapterRow(
            QStringLiteral("matrix"), QStringLiteral("Matrix"), true, true, true, true, true,
            QVariantList{field(QStringLiteral("homeserver"), QStringLiteral("Homeserver"), true,
                               QStringLiteral("Text")),
                         field(QStringLiteral("token"), QStringLiteral("Access token"), true,
                               QStringLiteral("Password"))})};
        reg.m_instances = QVariantList{
            instanceRow(QStringLiteral("matrix/@you"), QStringLiteral("matrix"),
                        QStringLiteral("Matrix (@you)"), true, QStringLiteral("ready"))};
        reg.m_settings[QStringLiteral("matrix/@you")] =
            QVariantMap{{QStringLiteral("homeserver"), QStringLiteral("https://hs.example")}};
        AccountManagementController ctrl;
        ctrl.setRegistry(&reg);

        ctrl.beginEdit(QStringLiteral("matrix/@you"));
        QCOMPARE(ctrl.mode(), QStringLiteral("edit"));
        QCOMPARE(ctrl.transport(), QStringLiteral("matrix/@you"));
        QCOMPARE(ctrl.family(), QStringLiteral("matrix"));

        const QVariantList f = ctrl.fields();
        QCOMPARE(f.size(), 2);
        QCOMPARE(f.at(0).toMap().value(QStringLiteral("value")).toString(),
                 QStringLiteral("https://hs.example"));
        // The masked secret is never returned by TransportSettings, so it stays blank.
        QVERIFY(f.at(1).toMap().value(QStringLiteral("value")).toString().isEmpty());
    }

    // THE merge-edit rule: submitEdit OMITS an untouched masked (Password) field from the configure
    // write (secrets never ride these ops; the node keeps the stored secret), while a CHANGED
    // secret is included. Non-secret fields are always sent.
    void submitEditOmitsUntouchedMaskedField() {
        FakeRegistry reg;
        reg.m_adapters = QVariantList{adapterRow(
            QStringLiteral("matrix"), QStringLiteral("Matrix"), true, true, true, true, true,
            QVariantList{field(QStringLiteral("homeserver"), QStringLiteral("Homeserver"), true,
                               QStringLiteral("Text")),
                         field(QStringLiteral("token"), QStringLiteral("Access token"), true,
                               QStringLiteral("Password"))})};
        reg.m_instances = QVariantList{
            instanceRow(QStringLiteral("matrix/@you"), QStringLiteral("matrix"),
                        QStringLiteral("Matrix (@you)"), true, QStringLiteral("ready"))};
        reg.m_settings[QStringLiteral("matrix/@you")] =
            QVariantMap{{QStringLiteral("homeserver"), QStringLiteral("https://hs.example")}};
        AccountManagementController ctrl;
        ctrl.setRegistry(&reg);
        ctrl.beginEdit(QStringLiteral("matrix/@you"));

        QSignalSpy submitted(&ctrl, &AccountManagementController::editSubmitted);
        // The user edits the homeserver and leaves the token field blank (untouched secret).
        QVariantMap values;
        values[QStringLiteral("homeserver")] = QStringLiteral("https://new.example");
        values[QStringLiteral("token")] = QString();
        ctrl.submitEdit(values);

        QCOMPARE(reg.configureCount, 1);
        QCOMPARE(reg.lastConfigureTransport, QStringLiteral("matrix/@you"));
        QVERIFY(reg.lastConfigureValues.contains(QStringLiteral("homeserver")));
        QCOMPARE(reg.lastConfigureValues.value(QStringLiteral("homeserver")).toString(),
                 QStringLiteral("https://new.example"));
        // The untouched masked field is OMITTED entirely (not sent as empty).
        QVERIFY2(!reg.lastConfigureValues.contains(QStringLiteral("token")),
                 "an untouched masked field must be omitted from the configure write");
        QCOMPARE(submitted.count(), 1);

        // A CHANGED secret IS included on the next save.
        reg.lastConfigureValues.clear();
        QVariantMap values2;
        values2[QStringLiteral("homeserver")] = QStringLiteral("https://new.example");
        values2[QStringLiteral("token")] = QStringLiteral("s3cret");
        ctrl.submitEdit(values2);
        QVERIFY(reg.lastConfigureValues.contains(QStringLiteral("token")));
        QCOMPARE(reg.lastConfigureValues.value(QStringLiteral("token")).toString(),
                 QStringLiteral("s3cret"));
    }

    // removeAccount tears the account down through the registry and reports it.
    void removeAccountTearsDown() {
        FakeRegistry reg;
        seedTwoAccounts(reg);
        AccountManagementController ctrl;
        ctrl.setRegistry(&reg);

        QSignalSpy removed(&ctrl, &AccountManagementController::accountRemoved);
        ctrl.removeAccount(QStringLiteral("matrix/@you"));
        QCOMPARE(reg.lastRemoved, QStringLiteral("matrix/@you"));
        QCOMPARE(removed.takeFirst().at(0).toString(), QStringLiteral("matrix/@you"));
    }

    // cancel returns the controller to idle.
    void cancelReturnsToIdle() {
        FakeRegistry reg;
        seedTwoAccounts(reg);
        AccountManagementController ctrl;
        ctrl.setRegistry(&reg);
        ctrl.beginAdd(QStringLiteral("matrix"));
        QCOMPARE(ctrl.mode(), QStringLiteral("add"));
        ctrl.cancel();
        QCOMPARE(ctrl.mode(), QStringLiteral("idle"));
    }
};

QTEST_MAIN(TestIntegrationsTree)
#include "tst_integrations_tree.moc"
