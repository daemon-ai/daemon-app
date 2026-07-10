// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "account_management_controller.h"
#include "integrations_tree_model.h"
#include "transports/ipersons_service.h"
#include "transports/itransport_registry.h"

#include <QSignalSpy>
#include <QtTest>

using transports::IPersonsService;
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
class FakePersons : public IPersonsService {
public:
    using IPersonsService::IPersonsService;
    QVariantList m_persons;
    [[nodiscard]] QVariantList persons() const override { return m_persons; }
    void refresh() override { emit personsChanged(m_persons); }
};

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

// A Matrix-shaped account (spaces + rooms + DMs + persons + directory) alongside a flat IRC-shaped
// account (rooms only, no DMs, no persons, no directory) — enough to exercise every gating rule and
// the space/parent nesting incl. the unknown-parent-as-root fallback.
void seedTwoAccounts(FakeRegistry& reg, FakePersons& persons) {
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
    persons.m_persons = QVariantList{person(QStringLiteral("p-bob"), QStringLiteral("Bob"),
                                            QStringLiteral("matrix/@you"), QStringLiteral("@bob"),
                                            QStringLiteral("Bob"), QStringLiteral("available"))};
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
        FakeRegistry reg;
        FakePersons persons;
        seedTwoAccounts(reg, persons);
        IntegrationsTreeModel model;
        model.setRegistry(&reg);
        model.setPersons(&persons);

        const int matrix = findRow(model, QStringLiteral("Matrix (@you)"));
        const int irc = findRow(model, QStringLiteral("IRC (libera)"));
        QVERIFY(matrix >= 0 && irc >= 0);
        QVERIFY2(matrix < irc, "accounts render in instances() order");
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
        FakeRegistry reg;
        FakePersons persons;
        seedTwoAccounts(reg, persons);
        IntegrationsTreeModel model;
        model.setRegistry(&reg);
        model.setPersons(&persons);

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
        FakeRegistry reg;
        FakePersons persons;
        seedTwoAccounts(reg, persons);
        IntegrationsTreeModel model;
        model.setRegistry(&reg);
        model.setPersons(&persons);

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
        FakeRegistry reg;
        FakePersons persons;
        seedTwoAccounts(reg, persons);
        IntegrationsTreeModel model;
        model.setRegistry(&reg);
        model.setPersons(&persons);

        const int dms = findRow(model, QStringLiteral("Direct Messages"));
        QVERIFY(dms >= 0);
        const int alice = findRow(model, QStringLiteral("Alice"));
        const int design = findRow(model, QStringLiteral("Design"));
        QVERIFY(alice >= 0 && design >= 0);
        QCOMPARE(roleAt<QString>(model, alice, IntegrationsTreeModel::ConvTypeRole),
                 QStringLiteral("dm"));
        QCOMPARE(roleAt<QString>(model, design, IntegrationsTreeModel::ConvTypeRole),
                 QStringLiteral("groupdm"));
    }

    // The Persons section renders the endpoints reachable on THIS transport (a cross-transport
    // identity). It appears only for the account whose transport an endpoint names.
    void personsSectionScopedToTransport() {
        FakeRegistry reg;
        FakePersons persons;
        seedTwoAccounts(reg, persons);
        IntegrationsTreeModel model;
        model.setRegistry(&reg);
        model.setPersons(&persons);

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
        FakeRegistry reg;
        FakePersons persons;
        seedTwoAccounts(reg, persons);
        IntegrationsTreeModel model;
        model.setRegistry(&reg);
        model.setPersons(&persons);

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
        FakeRegistry reg;
        FakePersons persons;
        seedTwoAccounts(reg, persons);
        IntegrationsTreeModel model;
        model.setRegistry(&reg);
        model.setPersons(&persons);

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
        FakeRegistry reg;
        FakePersons persons;
        seedTwoAccounts(reg, persons);
        IntegrationsTreeModel model;
        model.setRegistry(&reg);
        model.setPersons(&persons);

        model.toggleExpand(findRow(model, QStringLiteral("Demo Server")));
        QVERIFY(findRow(model, QStringLiteral("Demo Server")) >= 0);
        QVERIFY(findRow(model, QStringLiteral("general")) < 0);
        QVERIFY(findRow(model, QStringLiteral("random")) < 0);
        // The standalone Channels group is a sibling and stays.
        QVERIFY(findRow(model, QStringLiteral("standalone")) >= 0);
    }

    // expandAll / collapseAll fold every account+space+section; anyExpanded tracks it.
    void expandAllCollapseAll() {
        FakeRegistry reg;
        FakePersons persons;
        seedTwoAccounts(reg, persons);
        IntegrationsTreeModel model;
        model.setRegistry(&reg);
        model.setPersons(&persons);

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
        FakeRegistry reg;
        FakePersons persons;
        seedTwoAccounts(reg, persons);
        IntegrationsTreeModel model;
        model.setRegistry(&reg);
        model.setPersons(&persons);

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
        FakeRegistry reg;
        FakePersons persons;
        seedTwoAccounts(reg, persons);
        IntegrationsTreeModel model;
        model.setRegistry(&reg);
        model.setPersons(&persons);

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
        FakeRegistry reg;
        FakePersons persons;
        seedTwoAccounts(reg, persons);
        IntegrationsTreeModel model;
        model.setRegistry(&reg);
        model.setPersons(&persons);

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
        FakeRegistry reg;
        FakePersons persons;
        seedTwoAccounts(reg, persons);
        IntegrationsTreeModel model;
        model.setRegistry(&reg);
        model.setPersons(&persons);

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

    // The tree re-composes when the registry publishes a change (instancesChanged /
    // conversationsChanged) — no manual refresh.
    void rebuildsOnRegistrySignal() {
        FakeRegistry reg;
        FakePersons persons;
        seedTwoAccounts(reg, persons);
        IntegrationsTreeModel model;
        model.setRegistry(&reg);
        model.setPersons(&persons);

        QVERIFY(findRow(model, QStringLiteral("#daemon")) >= 0);
        reg.m_conversations[QStringLiteral("irc/libera")].append(
            conv(QStringLiteral("irc/libera"), QStringLiteral("#new"), QStringLiteral("#new"),
                 QStringLiteral("channel"), QString()));
        emit reg.conversationsChanged(QStringLiteral("irc/libera"));
        QVERIFY(findRow(model, QStringLiteral("#new")) >= 0);
    }

    // --- Schema-driven account management ------------------------------------

    // The protocol picker lists every adapter family (family + displayName).
    void availableFamiliesFromAdapters() {
        FakeRegistry reg;
        FakePersons persons;
        seedTwoAccounts(reg, persons);
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
        FakePersons persons;
        seedTwoAccounts(reg, persons);
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
        FakePersons persons;
        seedTwoAccounts(reg, persons);
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
