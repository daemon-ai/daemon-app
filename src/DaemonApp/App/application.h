#pragma once

#include "daemon/app_service_graph.h"

#include <QObject>

QT_BEGIN_NAMESPACE
class QEvent;
class QQmlApplicationEngine;
class QQuickWindow;
QT_END_NAMESPACE

class StatusBarModel;
class CommandRegistry;
class TranscriptExporter;

namespace persistence {
class ISessionStore;
}
namespace platform {
class IPlatformServices;
}
namespace settings {
class ISettingsStore;
}
namespace connection {
class IConnectionService;
}
namespace fs {
class IFsService;
}
namespace memory {
class IMemoryService;
}
namespace nav {
class NavController;
}
namespace firstrun {
class FirstRunModel;
}
namespace config {
class IDaemonConfig;
}
namespace models {
class IModelCatalog;
}
namespace accounts {
class IAccountsService;
}
namespace profiles {
class IProfileStore;
}
namespace fleet {
class ISessionRoster;
class IFleetTree;
class IApprovalsInbox;
class IDashboard;
}
namespace automation {
class IRoutingStore;
class ICronStore;
}
namespace session {
class ISessionSettings;
class ICheckpointTimeline;
}

// Owns the application-wide services (session store, platform integrations) and
// wires them to the QML scene. Kept UI-toolkit agnostic: the only desktop bit
// (tray) lives behind IPlatformServices.
class Application : public QObject {
    Q_OBJECT

public:
    explicit Application(QObject* parent = nullptr);
    ~Application() override;

    // Expose C++ services to QML before the scene loads.
    void registerContext(QQmlApplicationEngine& engine);

    // Connect platform services to the loaded root window and install the tray.
    void completeWiring(QQmlApplicationEngine& engine);

    // Raise a native OS notification for a turn that hit an approval/clarify/host
    // gate, but only while the window is hidden or not active (an on-screen window
    // already shows the inline gate). Returns true if a notification was shown.
    // Bound in QML to the active turn's awaitingInput signal.
    Q_INVOKABLE bool notifyGate(const QString& title, const QString& body);

    // Guarded render-harness hook: open an app-level page (and optional section)
    // via the shared Nav seam. Used only by DAEMON_APP_RENDER_PAGE before the
    // offscreen screenshot pass; no effect on a normal run.
    void openPageForRenderHarness(const QString& page, const QString& section = QString());

protected:
    // Close-to-tray: when a tray is installed, intercept the root window's close
    // (X) event and hide instead of quitting.
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    daemonapp::daemon::AppServiceGraph m_services;
    persistence::ISessionStore* m_store = nullptr;
    platform::IPlatformServices* m_platform = nullptr;
    // The shared footer status model, exposed to QML as the `Status` context
    // property so the footer StatusBar and the active session's turn feed a
    // single instance (the TUI builds its own equivalent in RootWidget).
    StatusBarModel* m_status = nullptr;
    // Shared command-palette catalog, exposed to QML as `Commands`.
    CommandRegistry* m_commands = nullptr;
    // Transcript exporter, exposed to QML as `Exporter`.
    TranscriptExporter* m_exporter = nullptr;
    // Shared client-local preference store, exposed to QML as `AppSettings`.
    settings::ISettingsStore* m_settings = nullptr;
    // Connection seam (mock now), exposed to QML as `Connection`; its liveness
    // state drives the StatusBarModel gateway state.
    connection::IConnectionService* m_connection = nullptr;
    // Filesystem seam (dev local-disk impl now), exposed to QML as `Fs`; backs
    // the file tree, finder, and editor. A daemon adapter replaces it later.
    fs::IFsService* m_fs = nullptr;
    // Memory-inspection seam (seeded mock now), exposed to QML as `Memory`; backs
    // the Memory page (overview / list / graph / timeline). A daemon NodeApi
    // adapter replaces it later.
    memory::IMemoryService* m_memory = nullptr;
    // App-level page navigation seam, exposed to QML as `Nav`.
    nav::NavController* m_nav = nullptr;
    // First-run / onboarding gate, exposed to QML as `FirstRun`.
    firstrun::FirstRunModel* m_firstRun = nullptr;
    // Daemon-authoritative config facade (mock), exposed to QML as `DaemonConfig`.
    config::IDaemonConfig* m_daemonConfig = nullptr;
    // Model catalog facade (mock), exposed to QML as `ModelCatalog`.
    models::IModelCatalog* m_modelCatalog = nullptr;
    // Accounts/auth facade (mock), exposed to QML as `Accounts`.
    accounts::IAccountsService* m_accounts = nullptr;
    // Profiles/agents facade (mock), exposed to QML as `Profiles`.
    profiles::IProfileStore* m_profiles = nullptr;
    // Fleet/ops facades (mock), exposed to QML as `SessionRoster`, `FleetTree`,
    // `Approvals`, `Dashboard`.
    fleet::ISessionRoster* m_roster = nullptr;
    fleet::IFleetTree* m_fleetTree = nullptr;
    fleet::IApprovalsInbox* m_approvals = nullptr;
    fleet::IDashboard* m_dashboard = nullptr;
    // Automation facades (mock), exposed to QML as `Routing` and `Cron`.
    automation::IRoutingStore* m_routing = nullptr;
    automation::ICronStore* m_cron = nullptr;
    // Per-session override + checkpoint facades (mock), exposed to QML as
    // `SessionSettings` and `Checkpoints`.
    session::ISessionSettings* m_sessionSettings = nullptr;
    session::ICheckpointTimeline* m_checkpoints = nullptr;
    QQuickWindow* m_window = nullptr;
};
