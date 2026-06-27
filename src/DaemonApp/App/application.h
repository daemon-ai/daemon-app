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
class ITurnEngineFactory;

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
} // namespace fleet
namespace automation {
class IRoutingStore;
class ICronStore;
} // namespace automation
namespace session {
class ISessionSettings;
class ICheckpointTimeline;
} // namespace session

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
    void openPageForRenderHarness(const QString& page, const QString& section = QString()) const;

    // Headless E2E hook: spin the event loop until the connection seam reaches "ready" (its Health
    // round-trip completes) or timeoutMs elapses; returns true iff ready. Lets the cross-repo
    // system-tests harness hard-assert the GUI -> daemon connectivity instead of racing it.
    [[nodiscard]] bool awaitConnectionReady(int timeoutMs) const;

    // Headless E2E hook: drive a first-run "Local" connect programmatically (the equivalent of the
    // user picking Local + Connect), using the resolved target (honors DAEMON_APP_SOCKET). No-op
    // once setup is complete, so it is safe to call unconditionally before awaitConnectionReady.
    // Lets the system-tests harness exercise the onboarding connect path without QML click sim.
    void driveFirstRunConnect() const;

    // Headless E2E hook (CON-4/6/7): drive the full first-run provider+model step without QML
    // clicks - connect, wait until ready, add the API key (CredentialSet on the active profile),
    // pick the first discovered model, and finish. Pumps the event loop so the credential/model
    // requests + responses actually round-trip. Returns true iff the connection reached ready.
    [[nodiscard]] bool runHeadlessOnboarding(const QString& provider, const QString& key,
                                             int timeoutMs);

    // Headless E2E hook (CHA-1 / CHA-2): connect, then drive one real turn through a
    // DaemonTurnEngine (Submit{StartTurn} + Subscribe stream) and return the assistant text
    // accumulated from the AgentEvent stream. Empty string on connect failure / empty turn.
    // `profile` optionally binds the turn to a specific profile (PRO-5).
    [[nodiscard]] QString runHeadlessChat(const QString& prompt, int timeoutMs,
                                          const QString& profile = QString());

    // Headless E2E hook (PRO-2/3): connect, then exercise the profile store - create `createId`
    // (ProfileCreate), then optionally update its model/prompt (ProfileUpdate), pumping the event
    // loop so the wire ops round-trip. Returns true iff the connection reached ready.
    [[nodiscard]] bool runHeadlessProfile(const QString& createId, const QString& model,
                                          const QString& prompt, int timeoutMs);

    // Headless E2E hook (Phase 2 local model track): connect, then exercise the model-track wire
    // ops through a ModelRepository - ModelCatalog, ModelDownloads, ModelSearch(query),
    // ModelFiles(repo), ModelDownload(repo,file) - and return a machine-readable summary of which
    // frames round-tripped, e.g. "catalog=ok downloads=ok search=ok files=err download=err". Each
    // probe records ok (success response), err (ApiError response), or timeout (no response). The
    // hermetic test asserts the always-deterministic ones (catalog/downloads) are ok and the
    // network-dependent ones produced a structured response (ok|err, never timeout) - proving the
    // client encodes + sends + the daemon decodes every model-track frame.
    [[nodiscard]] QString runHeadlessModels(const QString& query, const QString& repo,
                                            int timeoutMs);

    // Headless E2E hook (CHA-7): connect, issue a CommandList (and optionally CommandInvoke
    // `invoke`), and return the discovered command names (newline-joined), or the CommandOutput
    // text when `invoke` is set. Empty on connect failure. Exercises the slash-command wire ops
    // over the real NodeApiClient.
    [[nodiscard]] QString runHeadlessCommands(const QString& invoke, int timeoutMs);

    // Headless E2E hook (CHA-8): connect, issue a SessionSearch for `query`, and return the hit
    // session ids (newline-joined). Empty on connect failure / no hits.
    [[nodiscard]] QString runHeadlessSearch(const QString& query, int timeoutMs);

    // Headless E2E hook (CHA-4 / CHA-5 HITL): connect, drive one real turn that parks on a host
    // gate (the scripted provider's tool call), auto-resolve the gate per `decision`
    // ("approve"|"deny"|"choice"|"input:<text>"), and return the streamed assistant text. Proves
    // the park -> Respond -> resume loop end-to-end (the proxy sees Submit + Respond crossing).
    [[nodiscard]] QString runHeadlessHitl(const QString& prompt, const QString& decision,
                                          int timeoutMs);

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
    // Stop a daemon this client spawned, honoring the shutdown-on-exit preference (no-op
    // otherwise). Called from the destructor so every exit path - normal quit and the early-return
    // headless harness modes - cleans up a managed daemon when the user opted in.
    void shutdownManagedDaemon() const;

    // Run the event loop for `ms` (a bounded settle so queued NodeApi requests + their responses
    // flush before a headless harness exits). Used only by the headless onboarding hook.
    void settle(int ms) const;

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
    // Turn-engine factory (daemon Submit/Subscribe vs mock simulator), exposed to QML as
    // `TurnEngines`; each TranscriptPage assigns it onto its SessionOrchestrator.
    ITurnEngineFactory* m_turnEngines = nullptr;
    QQuickWindow* m_window = nullptr;
    // Held for live language switching: the engine to retranslate and the
    // UiSettings singleton whose `language` property drives it.
    QQmlApplicationEngine* m_engine = nullptr;
    QObject* m_uiSettings = nullptr;
};
