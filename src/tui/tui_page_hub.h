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
class IRoutingStore;
} // namespace automation
namespace config {
class IDaemonConfig;
}
namespace connection {
class IConnectionService;
}
namespace daemonapp::daemon {
class PrincipalModel;
}
namespace daemonnet {
class IDaemonNet;
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
namespace transports {
class IPresenceService;
class ITransportRegistry;
} // namespace transports

class TabModel;

class TuiPageHub {
    Q_DECLARE_TR_FUNCTIONS(TuiPageHub)

public:
    struct Dependencies {
        TabModel* tabModel = nullptr;
        config::IDaemonConfig* daemonConfig = nullptr;
        connection::IConnectionService* connection = nullptr;
        models::IModelCatalog* modelCatalog = nullptr;
        accounts::IAccountsService* accounts = nullptr;
        profiles::IProfileStore* profiles = nullptr;
        fleet::ISessionRoster* roster = nullptr;
        fleet::IFleetTree* fleetTree = nullptr;
        fleet::IApprovalsInbox* approvals = nullptr;
        fleet::IDashboard* dashboard = nullptr;
        automation::IRoutingStore* routing = nullptr;
        automation::ICronStore* cron = nullptr;
        daemonnet::IDaemonNet* daemonNet = nullptr;
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
    [[nodiscard]] QString buildChannelsMarkdown() const;

    Dependencies m_deps;
    QHash<int, int> m_pageSel;
};
