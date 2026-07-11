// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QCoreApplication>
#include <QHash>
#include <QList>
#include <QString>
#include <QVariantMap>
#include <Tui/ZEvent.h>

namespace accounts {
class IAccountsService;
}
namespace automation {
class ICronStore;
} // namespace automation
namespace config {
class IDaemonConfig;
}
namespace connection {
class IConnectionService;
}
namespace feedback {
class IFeedback;
}
namespace daemonapp::daemon {
class PrincipalModel;
class CapsRepository;    // [wave2:app-delegation] F7/DEL-7
class GatewayRepository; // [wave2:app-gateway] Phase F: node OpenAI-gateway control
class EngineIdentity;    // [wave2:integration] C5 approval origin attribution
} // namespace daemonapp::daemon
namespace daemonnet {
class IRoutingActions;
}
namespace mirror {
class MirrorService;
}
namespace fleet {
class IApprovalsInbox;
class IDashboard;
class IFleetTree;
class ISessionRoster;
} // namespace fleet
namespace memory {
class IMemoryService;
}
namespace memoryui {
class MemoryGraphModel;
class MemoryListModel;
class MemoryStatsModel;
class MemoryTimelineModel;
} // namespace memoryui
namespace models {
class IModelCatalog;
}
namespace profiles {
class IProfileStore;
}
namespace settings {
class ISettingsStore;
}
namespace tools {
class IToolInventory; // [wave2:app-approvals-safety] D2
}
namespace transports {
class IContactsService;
class IPresenceService;
class ITransportRegistry;
} // namespace transports
namespace update {
class UpdateManager;
}

class TabModel;

class TuiPageHub {
    Q_DECLARE_TR_FUNCTIONS(TuiPageHub)

public:
    struct Dependencies {
        TabModel* tabModel = nullptr;
        config::IDaemonConfig* daemonConfig = nullptr;
        connection::IConnectionService* connection = nullptr;
        // Node-owned telemetry-consent seam backing the Advanced "Send anonymous
        // telemetry" row (parity with the GUI AdvancedSection binding).
        feedback::IFeedback* feedback = nullptr;
        models::IModelCatalog* modelCatalog = nullptr;
        accounts::IAccountsService* accounts = nullptr;
        profiles::IProfileStore* profiles = nullptr;
        fleet::ISessionRoster* roster = nullptr;
        fleet::IFleetTree* fleetTree = nullptr;
        fleet::IApprovalsInbox* approvals = nullptr;
        tools::IToolInventory* tools = nullptr; // [wave2:app-approvals-safety] D2
        fleet::IDashboard* dashboard = nullptr;
        automation::ICronStore* cron = nullptr;
        // The Routing page's source (M3): the mirror store's origin->session pin table
        // (route_pins), shared with the GUI routing manager. Mutations (unbind) go through the
        // node-authoritative routingActions seam. Both null in mock (the page renders empty).
        mirror::MirrorService* mirror = nullptr;
        daemonnet::IRoutingActions* routingActions = nullptr;
        memory::IMemoryService* memory = nullptr;
        memoryui::MemoryListModel* memList = nullptr;
        memoryui::MemoryStatsModel* memStats = nullptr;
        memoryui::MemoryTimelineModel* memTimeline = nullptr;
        memoryui::MemoryGraphModel* memGraph = nullptr;
        settings::ISettingsStore* settings = nullptr;
        // The authenticated principal (WhoAmI): gates the Users & Access route and
        // backs its page projection. Advisory only - the node enforces server-side.
        daemonapp::daemon::PrincipalModel* principal = nullptr;
        transports::ITransportRegistry* transportRegistry = nullptr;
        transports::IPresenceService* presence = nullptr;
        // [acct-mgmt] Transport contacts / roster (Phase D): backs the per-account Contacts group.
        transports::IContactsService* contacts = nullptr;
        // Release-feed updater: backs the Settings "Updates" auto-check toggle
        // (gated to builds that actually have a feed).
        update::UpdateManager* update = nullptr;
        // [wave2:app-delegation] F7/DEL-7: read-only delegation guardrail ceilings (Settings ->
        // Safety). Null in mock mode — the rows show "—" then.
        daemonapp::daemon::CapsRepository* caps = nullptr;
        // [wave2:app-gateway] Phase F: the node OpenAI-gateway control seam
        // (GatewayGet/GatewaySet). Backs the Settings -> Gateway enable toggle + status line. Null
        // in mock mode (the section then shows "—"/unavailable).
        daemonapp::daemon::GatewayRepository* gateway = nullptr;
        // [wave2:integration] C5: the session->engine label facade, used to attribute a foreign
        // requester on approval rows (parity with the GUI EngineOriginChip).
        daemonapp::daemon::EngineIdentity* engineIdentity = nullptr;
    };

    explicit TuiPageHub(Dependencies deps);

    [[nodiscard]] QString pageMarkdownForKind(int kind,
                                              const QString& profileRef = QString()) const;
    [[nodiscard]] bool openManagerPage(const QString& id) const;
    [[nodiscard]] int activePageKind(bool transcriptActive) const;
    [[nodiscard]] QList<QVariantMap> pageActionRows(int kind) const;
    // The highlighted row index for a hub kind (what the ▸ marker renders;
    // unclamped - callers bound it against their rows). Used by TuiSettingsEditor
    // to resolve the row a dialog edit targets and by root-level overlay hooks
    // like the profile editor ('e').
    [[nodiscard]] int pageSelection(int kind) const { return m_pageSel.value(kind, 0); }

    void clampSelection(int kind);
    void moveSelection(int kind, int delta);
    bool handlePageActionKey(int kind, Tui::ZKeyEvent* event);

    // Persist an edited Settings-row value through the row's seam - the same
    // ISettingsStore / IDaemonConfig call the GUI section makes. Rows whose
    // apply is a RootWidget live path (theme / zen) return false untouched.
    bool applySettingsValue(const QVariantMap& row, const QVariant& value) const;

private:
    // The editable Settings rows (hub_settings.cpp): one QVariantMap per row
    // with id/label/type/seam/value/options - the schema the markdown builder,
    // the key handler and TuiSettingsEditor share.
    [[nodiscard]] QList<QVariantMap> settingsActionRows() const;
    // Space/Enter on a toggle row: flip in place via applySettingsValue.
    bool applySettingsToggle(const QVariantMap& row, bool activate) const;
    [[nodiscard]] QString buildSettingsMarkdown(int sel = -1) const;
    [[nodiscard]] QString buildModelsMarkdown(int sel = -1) const;
    [[nodiscard]] QString buildAccountsMarkdown(int sel = -1) const;
    [[nodiscard]] QString buildProfilesMarkdown(int sel = -1) const;
    [[nodiscard]] QString buildDashboardMarkdown() const;
    [[nodiscard]] QString buildFleetMarkdown(int sel = -1) const;
    [[nodiscard]] QString buildSessionsMarkdown(int sel = -1) const;
    [[nodiscard]] QString buildApprovalsMarkdown(int sel = -1) const;
    [[nodiscard]] QString buildRoutingMarkdown(int sel = -1) const;
    [[nodiscard]] QString buildCronMarkdown(int sel = -1) const;
    [[nodiscard]] QString buildMemoryMarkdown() const;
    [[nodiscard]] QString buildProfileMarkdown(const QString& profileRef) const;
    [[nodiscard]] QString buildUsersAccessMarkdown() const;
    // [waveB:app-v30] D1: `sel` marks the highlighted account for disconnect/remove.
    [[nodiscard]] QString buildChannelsMarkdown(int sel = -1) const;
    // [acct-mgmt] The Channels page's typed flattened row list: account rows (id = transport)
    // each followed by their room rows (id = conversation id, rowKind "room"). Backs the j/k
    // selection + the row-contextual keys. Mirrors the GUI account card + its room rows.
    [[nodiscard]] QList<QVariantMap> channelRows() const;
    // Engine-identity token (C3/ENG-7): the TUI projection of the GUI engine chip for a profile /
    // fleet row carrying engine/acpAgent ("Native" for Core, "<agent> (ACP)" for foreign; empty
    // when the row has no engine field, e.g. mock rows).
    [[nodiscard]] static QString engineToken(const QVariantMap& row);
    // The Routing page's pin rows (B6/ROU): one QVariantMap per DaemonNet RoutingPin, carrying
    // the display fields + the flattened origin (so the 'x' unbind can rebuild it). `id` is the
    // canonical originKey.
    [[nodiscard]] QList<QVariantMap> routingPinRows() const;

    Dependencies m_deps;
    QHash<int, int> m_pageSel;
};
