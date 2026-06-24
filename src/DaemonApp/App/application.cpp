#include "application.h"

#include "app/cached_image_provider.h"
#include "app/image_cache.h"
#include "app/math_image_provider.h"
#include "persistence/sqlite_session_store.h"
#include "platform/iplatform_services.h"
#include "platform/platform_services_factory.h"
#include "config/mock_daemon_config.h"
#include "connection/mock_connection_service.h"
#include "fs/local_disk_fs_service.h"
#include "memory/mock_memory_service.h"
#include "accounts/mock_accounts_service.h"
#include "automation/mock_cron_store.h"
#include "automation/mock_routing_store.h"
#include "fleet/mock_approvals_inbox.h"
#include "fleet/mock_dashboard.h"
#include "fleet/mock_fleet_tree.h"
#include "fleet/mock_session_roster.h"
#include "models/mock_model_catalog.h"
#include "profiles/mock_profile_store.h"
#include "session/mock_checkpoint_timeline.h"
#include "session/mock_session_settings.h"
#include "firstrun/first_run_model.h"
#include "nav/nav_controller.h"
#include "settings/qt_settings_store.h"
#include "command_registry.h"
#include "status_bar_model.h"
#include "transcript_exporter.h"

#include <QCoreApplication>
#include <QEvent>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>

#include <core/formula.h>
#include <latex.h>

Application::Application(QObject* parent)
    : QObject(parent)
    , m_store(new persistence::SqliteSessionStore(QString(), this))
    , m_platform(platform::createPlatformServices(this))
    , m_status(new StatusBarModel(this))
    , m_commands(new CommandRegistry(this))
    , m_exporter(new TranscriptExporter(this))
    , m_settings(new settings::QtSettingsStore(this))
    , m_connection(new connection::MockConnectionService(this))
    , m_memory(new memory::MockMemoryService(this))
    , m_nav(new nav::NavController(this))
    , m_firstRun(new firstrun::FirstRunModel(m_settings, m_connection, this))
    , m_daemonConfig(new config::MockDaemonConfig(this))
    , m_modelCatalog(new models::MockModelCatalog(this))
    , m_accounts(new accounts::MockAccountsService(this))
    , m_profiles(new profiles::MockProfileStore(this))
    , m_roster(new fleet::MockSessionRoster(this))
    , m_fleetTree(new fleet::MockFleetTree(this))
    , m_approvals(new fleet::MockApprovalsInbox(this))
    , m_dashboard(new fleet::MockDashboard(m_roster, m_fleetTree, m_approvals, this))
    , m_routing(new automation::MockRoutingStore(this))
    , m_cron(new automation::MockCronStore(this))
    , m_sessionSettings(new session::MockSessionSettings(this))
    , m_checkpoints(new session::MockCheckpointTimeline(this))
{
    // The connection seam owns liveness; mirror its state into the footer's
    // gateway indicator (the single surface for connection state).
    connect(m_connection, &connection::IConnectionService::stateChanged, this,
            [this] { m_status->setGatewayState(m_connection->state()); });

    // Filesystem seam: a DEV local-disk implementation rooted at the configured
    // workspace root (falls back to $HOME). This is not the production source of
    // truth; a daemon adapter over IFsService replaces it later. Built in the
    // body so it can read the daemon-config workspace root.
    m_fs = new fs::LocalDiskFsService(
        m_daemonConfig->value(QStringLiteral("workspace/root")).toString(), QString(), this);

    // MicroTeX loads its fonts/XML resources once; the path is baked in at build
    // time (MICROTEX_RES_DIR). Done here so the "math" image provider can parse
    // formulas as soon as the scene requests them.
    tex::LaTeX::init(std::string(MICROTEX_RES_DIR));

    // Pin the base font size MicroTeX draws with (its QFonts are cached on first
    // use, so this must happen before the first parse and never change). The
    // library defaults to a 1pt font scaled entirely by the painter transform,
    // which overflows Qt's FreeType raster on HiDPI and drops every glyph; see
    // be::app::kMathBaseFontPt for the full rationale and the matching render.
    tex::Formula::setDPITarget(72.f * be::app::kMathBaseFontPt);
}

Application::~Application()
{
    tex::LaTeX::release();
}

void Application::registerContext(QQmlApplicationEngine& engine)
{
    // Shared store; QML view models bind their `store` property to this.
    engine.rootContext()->setContextProperty(QStringLiteral("SessionStore"), m_store);

    // Shared footer status model: the StatusBar footer renders it and the active
    // session's turn feeds it (see TranscriptPage.qml), so both halves of the
    // window agree on one busy/usage/context source.
    engine.rootContext()->setContextProperty(QStringLiteral("Status"), m_status);

    // Shared command-palette catalog: Main.qml's Mod+K overlay binds this model and
    // routes commandTriggered(id) back to the existing actions.
    engine.rootContext()->setContextProperty(QStringLiteral("Commands"), m_commands);

    // Transcript exporter (the /save + session "Export" action).
    engine.rootContext()->setContextProperty(QStringLiteral("Exporter"), m_exporter);

    // Shared client-local preference store (setupComplete, last connection, ...).
    engine.rootContext()->setContextProperty(QStringLiteral("AppSettings"), m_settings);

    // Connection seam: the first-run gate + Connection settings drive it; the
    // footer reads its liveness via the StatusBarModel mirror wired above.
    engine.rootContext()->setContextProperty(QStringLiteral("Connection"), m_connection);

    // App-level page navigation: Main.qml mounts the active manager/settings page
    // as an overlay bound to Nav.page.
    engine.rootContext()->setContextProperty(QStringLiteral("Nav"), m_nav);

    // First-run / onboarding gate: Main.qml mounts it over the shell until setup
    // completes. Compute the initial phase from persisted setupComplete now.
    engine.rootContext()->setContextProperty(QStringLiteral("FirstRun"), m_firstRun);
    m_firstRun->begin();

    // Daemon-authoritative config facade (mock) backing the settings sections.
    engine.rootContext()->setContextProperty(QStringLiteral("DaemonConfig"), m_daemonConfig);

    // Filesystem seam (dev local-disk impl) backing the file tree, finder, and editor.
    engine.rootContext()->setContextProperty(QStringLiteral("Fs"), m_fs);

    // Memory-inspection seam (seeded mock) backing the Memory page.
    engine.rootContext()->setContextProperty(QStringLiteral("Memory"), m_memory);

    // Model catalog facade (mock) backing the Models hub.
    engine.rootContext()->setContextProperty(QStringLiteral("ModelCatalog"), m_modelCatalog);

    // Accounts/auth facade (mock) backing the Accounts manager + wizard.
    engine.rootContext()->setContextProperty(QStringLiteral("Accounts"), m_accounts);

    // Profiles/agents facade (mock) backing the profile editor + curator.
    engine.rootContext()->setContextProperty(QStringLiteral("Profiles"), m_profiles);

    // Fleet/ops facades (mock) backing the dashboard / fleet / sessions / approvals.
    engine.rootContext()->setContextProperty(QStringLiteral("SessionRoster"), m_roster);
    engine.rootContext()->setContextProperty(QStringLiteral("FleetTree"), m_fleetTree);
    engine.rootContext()->setContextProperty(QStringLiteral("Approvals"), m_approvals);
    engine.rootContext()->setContextProperty(QStringLiteral("Dashboard"), m_dashboard);

    // Automation facades (mock) backing the routing matrix + cron manager.
    engine.rootContext()->setContextProperty(QStringLiteral("Routing"), m_routing);
    engine.rootContext()->setContextProperty(QStringLiteral("Cron"), m_cron);

    // Per-session override + checkpoint facades (mock) backing the composer popovers.
    engine.rootContext()->setContextProperty(QStringLiteral("SessionSettings"), m_sessionSettings);
    engine.rootContext()->setContextProperty(QStringLiteral("Checkpoints"), m_checkpoints);

    // Notifier seam: QML binds the active turn's awaitingInput signal to
    // App.notifyGate(...) to raise an OS notification when the window is hidden.
    engine.rootContext()->setContextProperty(QStringLiteral("App"), this);

    // The BlockEditor renderer resolves image://imgcache/<url> through the shared
    // ImageCache; instantiate it on the GUI thread and register the provider.
    be::app::ImageCache::instance();
    engine.addImageProvider(QStringLiteral("imgcache"), new be::app::CachedImageProvider);

    // LaTeX math: image://math/<payload> requests are rasterized by MicroTeX.
    engine.addImageProvider(QStringLiteral("math"), new be::app::MathImageProvider);
}

void Application::openPageForShots(const QString& page, const QString& section)
{
    m_nav->open(page, section);
}

void Application::completeWiring(QQmlApplicationEngine& engine)
{
    QQuickWindow* window = nullptr;
    const auto roots = engine.rootObjects();
    for (QObject* obj : roots) {
        if (auto* w = qobject_cast<QQuickWindow*>(obj)) {
            window = w;
            break;
        }
    }

    if (window) {
        const auto showWindow = [window] {
            window->show();
            window->raise();
            window->requestActivate();
        };
        connect(m_platform, &platform::IPlatformServices::showWindowRequested, this, showWindow);
        connect(m_platform, &platform::IPlatformServices::toggleWindowRequested, this,
                [window, showWindow] {
                    if (window->isVisible() && window->visibility() != QWindow::Minimized) {
                        window->hide();
                    } else {
                        showWindow();
                    }
                });
    }

    connect(m_platform, &platform::IPlatformServices::quitRequested, this,
            [] { QCoreApplication::quit(); });

    const bool trayInstalled = m_platform->installTray(QCoreApplication::applicationName());

    // Close-to-tray: only when a tray is actually present, so a desktop without a
    // system tray (or mobile) keeps the default quit-on-close and the user is
    // never stranded with a hidden window and no way back. The tray's Quit action
    // (quitRequested) remains the explicit exit.
    if (window && trayInstalled) {
        m_window = window;
        QGuiApplication::setQuitOnLastWindowClosed(false);
        window->installEventFilter(this);
    }

    // Returning users (setup already complete) auto-open the saved connection so
    // the footer gateway indicator reflects liveness. On first launch the
    // onboarding connection picker drives connectTo instead.
    if (m_settings->setupComplete()) {
        m_connection->connectTo(m_settings->lastConnectionMode(),
                                m_settings->lastConnectionTarget().isEmpty()
                                    ? QStringLiteral("/run/daemon.sock")
                                    : m_settings->lastConnectionTarget());
    }
}

bool Application::notifyGate(const QString& title, const QString& body)
{
    if (m_platform == nullptr) {
        return false;
    }
    // An on-screen, active window already shows the inline gate, so only alert
    // when the window is hidden, minimized, or not the active window.
    if (m_window != nullptr && m_window->isVisible()
        && m_window->visibility() != QWindow::Minimized && m_window->isActive()) {
        return false;
    }
    return m_platform->notify(title, body);
}

bool Application::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_window && event->type() == QEvent::Close) {
        event->ignore();
        m_window->hide(); // hide to tray; the app keeps running
        return true;      // swallow the close so the window is not destroyed
    }
    return QObject::eventFilter(watched, event);
}
