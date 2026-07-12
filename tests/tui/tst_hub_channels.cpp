// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// Channels hub page tests (Wave 1 → AD 1a.3): the TabModel::Channels projection renders the
// SHARED ChannelsHubModel — the mirror projection both the GUI ChannelsPage and this TUI page
// bind. The fixtures seed a real mirror Store through the Seeder (the same apply pipeline the
// scenario/daemon feeds use), so the page renders exactly what production renders: seeded
// adapters / accounts / rooms / contacts land in the markdown with their connection states,
// an absent projection degrades to an explicit note, and the "channels" manager route opens
// the page tab (the TUI mirror of the GUI cog menu -> Nav.open("channels")).

#include "daemon/channels_hub_model.h"
#include "mirror/mirror_service.h"
#include "mirror/seeder.h"
#include "tab_model.h"
#include "tui_page_hub.h"

#include <QtTest>

using daemonapp::daemon::ChannelsHubModel;

namespace {

constexpr auto kMatrix = "matrix/@bot:hs.org";

// The seeded mirror rows the daemon feed would deliver (map_adapter/map_transport_account
// vocabulary: lowercase connection tokens, ops_json camelCase keys).
void seedChannels(mirror::MirrorService& svc, bool withRows = true) {
    mirror::SeedSet seed;
    if (withRows) {
        mirror::Adapter matrix;
        matrix.family = QStringLiteral("matrix");
        matrix.display_name = QStringLiteral("Matrix");
        matrix.cap_rooms = true;
        matrix.cap_direct_messages = true;
        matrix.ops_json =
            QStringLiteral("{\"rosterOps\":{\"add\":true,\"list\":true,\"remove\":true,"
                           "\"update\":false}}");
        mirror::Adapter rooms;
        rooms.family = QStringLiteral("room");
        rooms.display_name = QStringLiteral("Rooms (internal)");
        rooms.cap_rooms = true;
        seed.adapters = {matrix, rooms};

        mirror::TransportAccount ops;
        ops.transport = QLatin1String(kMatrix);
        ops.family = QStringLiteral("matrix");
        ops.display_name = QStringLiteral("@bot:hs.org");
        ops.label = QStringLiteral("Ops bot"); // wire v35: the label overlays the display name
        ops.enabled = true;
        ops.bound_profile = QStringLiteral("triage");
        ops.connection = QStringLiteral("connected");
        mirror::TransportAccount local;
        local.transport = QStringLiteral("room/local");
        local.family = QStringLiteral("room");
        local.enabled = false; // node-disabled, no display name -> transport id fallback
        local.connection = QStringLiteral("offline");
        seed.transportAccounts = {ops, local};

        mirror::Conversation sec;
        sec.transport = QLatin1String(kMatrix);
        sec.id = QStringLiteral("!sec:hs.org");
        sec.kind = QStringLiteral("channel");
        sec.title = QStringLiteral("#secops");
        mirror::Conversation help;
        help.transport = QLatin1String(kMatrix);
        help.id = QStringLiteral("!help:hs.org");
        help.kind = QStringLiteral("dm"); // untitled -> the id renders
        seed.conversations = {sec, help};

        mirror::Contact alice;
        alice.transport = QLatin1String(kMatrix);
        alice.id = QStringLiteral("@alice:hs.org");
        alice.display_name = QStringLiteral("Alice");
        alice.presence_primitive = QStringLiteral("available");
        mirror::Contact bob;
        bob.transport = QLatin1String(kMatrix);
        bob.id = QStringLiteral("@bob:hs.org");
        bob.presence_primitive = QStringLiteral("offline");
        seed.contacts = {alice, bob};
    }
    mirror::Seeder seeder(svc.store());
    seeder.seed(seed);
}

} // namespace

class TuiHubChannelsTests : public QObject {
    Q_OBJECT

private slots:
    // The seeded adapters, accounts (with connection states + bound profiles) and per-account
    // rooms all land in the Channels page markdown — read from the ONE mirror projection.
    void channelsPageProjectsSeededMirror() {
        mirror::MirrorService svc;
        svc.openInMemory();
        seedChannels(svc);
        ChannelsHubModel hub_model(&svc.store());
        TuiPageHub::Dependencies deps;
        deps.channelsHub = &hub_model;
        const TuiPageHub hub(deps);

        const QString md = hub.pageMarkdownForKind(TabModel::Channels);

        QVERIFY(md.contains(QStringLiteral("# Channels")));
        QVERIFY(md.contains(QStringLiteral("## Accounts")));
        QVERIFY(md.contains(QStringLiteral("## Add channel")));

        // Accounts: label-overlaid display name, family · boundProfile, live connection state.
        QVERIFY(md.contains(QStringLiteral("Ops bot")));
        QVERIFY(md.contains(QStringLiteral("matrix · triage")));
        QVERIFY(md.contains(QStringLiteral("connected")));
        // Fallbacks: no displayName -> transport id; no rooms -> the empty note.
        QVERIFY(md.contains(QStringLiteral("room/local")));
        QVERIFY(md.contains(QStringLiteral("No rooms.")));

        // Rooms: title (or id when untitled) plus the conversation kind.
        QVERIFY(md.contains(QStringLiteral("#secops")));
        QVERIFY(md.contains(QStringLiteral("channel")));
        QVERIFY(md.contains(QStringLiteral("!help:hs.org")));

        // Adapter picker rows.
        QVERIFY(md.contains(QStringLiteral("Matrix")));
        QVERIFY(md.contains(QStringLiteral("Rooms (internal)")));
    }

    // [acct-mgmt] The Contacts group + its contact sub-rows render under the account whose
    // adapter reports rosterOps.list (ops_json); the room/local account (no rosterOps) shows no
    // Contacts group. Contacts come from the mirror `contacts` table.
    void channelsPageProjectsContacts() {
        mirror::MirrorService svc;
        svc.openInMemory();
        seedChannels(svc);
        ChannelsHubModel hub_model(&svc.store());
        TuiPageHub::Dependencies deps;
        deps.channelsHub = &hub_model;
        const TuiPageHub hub(deps);

        const QString md = hub.pageMarkdownForKind(TabModel::Channels);

        // The group header with the count, plus both contacts (aliased name + bare id fallback).
        QVERIFY(md.contains(QStringLiteral("~ Contacts (2)")));
        QVERIFY(md.contains(QStringLiteral("Alice")));
        QVERIFY(md.contains(QStringLiteral("@alice:hs.org")));
        QVERIFY(md.contains(QStringLiteral("@bob:hs.org")));
        // The contact-row key hints are surfaced.
        QVERIFY(md.contains(QStringLiteral("Contact row:")));
    }

    // [acct-mgmt] wire v35: the account row renders the label overlay (displayName) + a marker
    // for a node-disabled account; the enabled account carries no marker. The
    // connect/enable/rename key hints are surfaced on the account-row help line.
    void channelsPageRendersEnabledAndLabel() {
        mirror::MirrorService svc;
        svc.openInMemory();
        seedChannels(svc);
        ChannelsHubModel hub_model(&svc.store());
        TuiPageHub::Dependencies deps;
        deps.channelsHub = &hub_model;
        const TuiPageHub hub(deps);

        const QString md = hub.pageMarkdownForKind(TabModel::Channels);

        // The label overlay renders as the account name; the disabled account is marked.
        QVERIFY(md.contains(QStringLiteral("Ops bot")));
        QVERIFY(md.contains(QStringLiteral("disabled")));
        // The account-row key legend advertises connect / enable-disable / rename.
        QVERIFY(md.contains(QStringLiteral("**e** enable/disable")));
        QVERIFY(md.contains(QStringLiteral("**r** rename")));
    }

    // Absent projection: no hub model renders the explicit unavailable note; a present-but-empty
    // mirror renders the GUI's empty-state copy.
    void channelsPageDegradesWithoutProjection() {
        const TuiPageHub bare{TuiPageHub::Dependencies{}};
        const QString unavailable = bare.pageMarkdownForKind(TabModel::Channels);
        QVERIFY(unavailable.contains(QStringLiteral("# Channels")));
        QVERIFY(unavailable.contains(QStringLiteral("unavailable")));

        mirror::MirrorService svc;
        svc.openInMemory();
        seedChannels(svc, /*withRows=*/false);
        ChannelsHubModel hub_model(&svc.store());
        TuiPageHub::Dependencies deps;
        deps.channelsHub = &hub_model;
        const TuiPageHub hub(deps);
        const QString md = hub.pageMarkdownForKind(TabModel::Channels);
        QVERIFY(md.contains(QStringLiteral("No channels connected.")));
        QVERIFY(md.contains(QStringLiteral("Connect to a daemon")));
    }

    // AD (1a.3): the "new room" affordance is client-local on the shared model — the first
    // conversations commit is the baseline; a later arrival reads back new until marked seen.
    void newRoomBadgeTracksBaselineAndSeen() {
        mirror::MirrorService svc;
        svc.openInMemory();
        seedChannels(svc);
        ChannelsHubModel hub_model(&svc.store());
        const QString t = QLatin1String(kMatrix);
        QVERIFY(!hub_model.isNewConversation(t, QStringLiteral("!sec:hs.org"))); // baseline

        mirror::Conversation fresh;
        fresh.transport = t;
        fresh.id = QStringLiteral("!fresh:hs.org");
        fresh.kind = QStringLiteral("channel");
        fresh.title = QStringLiteral("fresh");
        mirror::Seeder seeder(svc.store());
        seeder.upsertConversation(fresh);
        QVERIFY(hub_model.isNewConversation(t, QStringLiteral("!fresh:hs.org")));

        hub_model.markConversationSeen(t, QStringLiteral("!fresh:hs.org"));
        QVERIFY(!hub_model.isNewConversation(t, QStringLiteral("!fresh:hs.org")));
    }

    // The "channels" manager route (reached via the NavController seam, the
    // TUI counterpart of the GUI cog menu) opens a TabModel::Channels tab.
    void openManagerPageRoutesChannels() {
        TabModel tabs;
        TuiPageHub::Dependencies deps;
        deps.tabModel = &tabs;
        const TuiPageHub hub(deps);

        QVERIFY(hub.openManagerPage(QStringLiteral("channels")));
        const int current = tabs.currentIndex();
        QVERIFY(current >= 0);
        QCOMPARE(tabs.kindAt(current), static_cast<int>(TabModel::Channels));
    }
};

QTEST_GUILESS_MAIN(TuiHubChannelsTests)
#include "tst_hub_channels.moc"
