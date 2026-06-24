#pragma once

#include <Tui/ZEvent.h>

#include <QHash>
#include <QList>
#include <QString>
#include <QVariantMap>

namespace accounts {
class IAccountsService;
}
namespace automation {
class ICronStore;
class IRoutingStore;
}
namespace config {
class IDaemonConfig;
}
namespace connection {
class IConnectionService;
}
namespace fleet {
class IApprovalsInbox;
class IDashboard;
class IFleetTree;
class ISessionRoster;
}
namespace memory {
class IMemoryService;
}
namespace memoryui {
class MemoryGraphModel;
class MemoryListModel;
class MemoryStatsModel;
class MemoryTimelineModel;
}
namespace models {
class IModelCatalog;
}
namespace profiles {
class IProfileStore;
}

class TabModel;

class TuiPageHub {
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
        memory::IMemoryService* memory = nullptr;
        memoryui::MemoryListModel* memList = nullptr;
        memoryui::MemoryStatsModel* memStats = nullptr;
        memoryui::MemoryTimelineModel* memTimeline = nullptr;
        memoryui::MemoryGraphModel* memGraph = nullptr;
    };

    explicit TuiPageHub(Dependencies deps);

    [[nodiscard]] QString pageMarkdownForKind(int kind, const QString& profileRef = QString()) const;
    [[nodiscard]] bool openManagerPage(const QString& id) const;
    [[nodiscard]] int activePageKind(bool transcriptActive) const;
    [[nodiscard]] QList<QVariantMap> pageActionRows(int kind) const;

    void clampSelection(int kind);
    void moveSelection(int kind, int delta);
    bool handlePageActionKey(int kind, Tui::ZKeyEvent* event);

private:
    [[nodiscard]] QString buildSettingsMarkdown() const;
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

    Dependencies m_deps;
    QHash<int, int> m_pageSel;
};
