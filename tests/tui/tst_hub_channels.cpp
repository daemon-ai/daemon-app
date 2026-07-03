// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// Channels hub page tests (Wave 1): the TabModel::Channels projection renders
// the transports seams - seeded adapters / accounts / rooms land in the page
// markdown with their presence states, absent seams degrade to an explicit
// note instead of an empty page, and the "channels" manager route opens the
// page tab (the TUI mirror of the GUI cog menu -> Nav.open("channels")).

#include "tab_model.h"
#include "transports/ipresence_service.h"
#include "transports/itransport_registry.h"
#include "tui_page_hub.h"

#include <QtTest>

namespace {

// Seeded stand-ins for the transports seams (the shipped MockTransportRegistry
// reports no instances, so seed the account/room shapes the daemon adapter
// emits - the field names match ITransportRegistry's row contract).
class SeededTransportRegistry : public transports::ITransportRegistry {
public:
    using ITransportRegistry::ITransportRegistry;

    [[nodiscard]] QVariantList availableAdapters() const override {
        return {
            QVariantMap{{QStringLiteral("family"), QStringLiteral("matrix")},
                        {QStringLiteral("displayName"), QStringLiteral("Matrix")}},
            QVariantMap{{QStringLiteral("family"), QStringLiteral("room")},
                        {QStringLiteral("displayName"), QStringLiteral("Rooms (internal)")}},
        };
    }

    [[nodiscard]] QVariantList instances() const override {
        return {
            QVariantMap{{QStringLiteral("transport"), QStringLiteral("matrix/@bot:hs.org")},
                        {QStringLiteral("family"), QStringLiteral("matrix")},
                        {QStringLiteral("displayName"), QStringLiteral("Ops bot")},
                        {QStringLiteral("boundProfile"), QStringLiteral("triage")}},
            // No display name + no bound profile: the row falls back to the
            // transport id and the bare family.
            QVariantMap{{QStringLiteral("transport"), QStringLiteral("room/local")},
                        {QStringLiteral("family"), QStringLiteral("room")},
                        {QStringLiteral("displayName"), QString()},
                        {QStringLiteral("boundProfile"), QString()}},
        };
    }

    [[nodiscard]] QVariantList conversations(const QString& transport) const override {
        if (transport != QLatin1String("matrix/@bot:hs.org")) {
            return {};
        }
        return {
            QVariantMap{{QStringLiteral("transport"), transport},
                        {QStringLiteral("id"), QStringLiteral("!sec:hs.org")},
                        {QStringLiteral("kind"), QStringLiteral("channel")},
                        {QStringLiteral("title"), QStringLiteral("#secops")}},
            QVariantMap{{QStringLiteral("transport"), transport},
                        {QStringLiteral("id"), QStringLiteral("!help:hs.org")},
                        {QStringLiteral("kind"), QStringLiteral("dm")},
                        {QStringLiteral("title"), QString()}},
        };
    }
};

// One connected account, everything else offline (the mock-presence default).
class SeededPresenceService : public transports::IPresenceService {
public:
    using IPresenceService::IPresenceService;

    [[nodiscard]] QString connectionState(const QString& transport) const override {
        return transport == QLatin1String("matrix/@bot:hs.org") ? QStringLiteral("connected")
                                                                : QStringLiteral("offline");
    }
    [[nodiscard]] QString presence(const QString&) const override {
        return QStringLiteral("unknown");
    }
};

// An empty registry: connected-nothing, adapters need a live daemon.
class EmptyTransportRegistry : public transports::ITransportRegistry {
public:
    using ITransportRegistry::ITransportRegistry;

    [[nodiscard]] QVariantList availableAdapters() const override { return {}; }
    [[nodiscard]] QVariantList instances() const override { return {}; }
};

} // namespace

class TuiHubChannelsTests : public QObject {
    Q_OBJECT

private slots:
    // The seeded adapters, accounts (with presence states + bound profiles)
    // and per-account rooms all land in the Channels page markdown.
    void channelsPageProjectsSeededSeams() {
        SeededTransportRegistry registry;
        SeededPresenceService presence;
        TuiPageHub::Dependencies deps;
        deps.transportRegistry = &registry;
        deps.presence = &presence;
        const TuiPageHub hub(deps);

        const QString md = hub.pageMarkdownForKind(TabModel::Channels);

        QVERIFY(md.contains(QStringLiteral("# Channels")));
        QVERIFY(md.contains(QStringLiteral("## Accounts")));
        QVERIFY(md.contains(QStringLiteral("## Add channel")));

        // Accounts: display name, family · boundProfile, live connection state.
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

    // Absent seams: no transports registry renders the explicit unavailable
    // note; a present-but-empty registry renders the GUI's empty-state copy
    // (and a missing presence seam falls back to offline without crashing).
    void channelsPageDegradesWithoutSeams() {
        const TuiPageHub bare{TuiPageHub::Dependencies{}};
        const QString unavailable = bare.pageMarkdownForKind(TabModel::Channels);
        QVERIFY(unavailable.contains(QStringLiteral("# Channels")));
        QVERIFY(unavailable.contains(QStringLiteral("unavailable")));

        EmptyTransportRegistry registry;
        TuiPageHub::Dependencies deps;
        deps.transportRegistry = &registry; // presence stays null
        const TuiPageHub hub(deps);
        const QString md = hub.pageMarkdownForKind(TabModel::Channels);
        QVERIFY(md.contains(QStringLiteral("No channels connected.")));
        QVERIFY(md.contains(QStringLiteral("Connect to a daemon")));
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
