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

    // Headless E2E hook: spin the event loop until the connection seam reaches "ready" (its Health
    // round-trip completes) or timeoutMs elapses; returns true iff ready. Lets the cross-repo
    // system-tests harness hard-assert the GUI -> daemon connectivity instead of racing it.
    [[nodiscard]] bool awaitConnectionReady(int timeoutMs);

protected:
    // Close-to-tray: when a tray is installed, intercept the root window's close
    // (X) event and hide instead of quitting.
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    // Reload translations and retranslate the live QML scene when the user picks
    // a new language in Settings (UiSettings.language). Connected generically so
    // the App module need not depend on the Settings type.
    void onLanguageChanged();

private:
    // Shared GUI/TUI service graph (session store, connection, fs, memory, nav, first-run,
    // config, models, accounts, profiles, fleet, automation, session facades). Read members
    // via m_services.* rather than mirroring them as duplicate pointers.
    daemonapp::daemon::AppServiceGraph m_services;
    // GUI-local services that are not part of the shared graph.
    platform::IPlatformServices* m_platform = nullptr;
    // The shared footer status model, exposed to QML as the `Status` context
    // property so the footer StatusBar and the active session's turn feed a
    // single instance (the TUI builds its own equivalent in RootWidget).
    StatusBarModel* m_status = nullptr;
    // Shared command-palette catalog, exposed to QML as `Commands`.
    CommandRegistry* m_commands = nullptr;
    // Transcript exporter, exposed to QML as `Exporter`.
    TranscriptExporter* m_exporter = nullptr;
    QQuickWindow* m_window = nullptr;
    // Held for live language switching: the engine to retranslate and the
    // UiSettings singleton whose `language` property drives it.
    QQmlApplicationEngine* m_engine = nullptr;
    QObject* m_uiSettings = nullptr;
};
