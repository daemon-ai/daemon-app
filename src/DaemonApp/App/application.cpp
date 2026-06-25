#include "application.h"

#include "app/cached_image_provider.h"
#include "app/image_cache.h"
#include "app/math_image_provider.h"
#include "accounts/iaccounts_service.h"
#include "automation/icron_store.h"
#include "automation/irouting_store.h"
#include "config/idaemon_config.h"
#include "connection/iconnection_service.h"
#include "platform/iplatform_services.h"
#include "platform/platform_services_factory.h"
#include "firstrun/first_run_model.h"
#include "fleet/iapprovals_inbox.h"
#include "fleet/idashboard.h"
#include "fleet/ifleet_tree.h"
#include "fleet/isession_roster.h"
#include "fs/ifs_service.h"
#include "memory/imemory_service.h"
#include "models/imodel_catalog.h"
#include "nav/nav_controller.h"
#include "persistence/isession_store.h"
#include "profiles/iprofile_store.h"
#include "session/icheckpoint_timeline.h"
#include "session/isession_settings.h"
#include "settings/isettings_store.h"
#include "command_registry.h"
#include "status_bar_model.h"
#include "transcript_exporter.h"

#include "i18n/localization.h"

#include <QCoreApplication>
#include <QEvent>
#include <QEventLoop>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QString>
#include <QTimer>

#include <core/formula.h>
#include <latex.h>

Application::Application(QObject* parent)
    : QObject(parent)
    , m_services(daemonapp::daemon::createAppServiceGraph(
          daemonapp::daemon::serviceModeFromEnvironment(), this))
    , m_platform(platform::createPlatformServices(this))
    , m_status(new StatusBarModel(this))
    , m_commands(new CommandRegistry(this))
    , m_exporter(new TranscriptExporter(this))
{
    // The connection seam owns liveness; mirror its state into the footer's
    // gateway indicator (the single surface for connection state).
    connect(m_services.connection, &connection::IConnectionService::stateChanged, this,
            [this] { m_status->setGatewayState(m_services.connection->state()); });

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
    // Held so a language change can retranslate the live scene (see
    // onLanguageChanged, wired in completeWiring once the singleton exists).
    m_engine = &engine;

    // Shared store; QML view models bind their `store` property to this.
    engine.rootContext()->setContextProperty(QStringLiteral("SessionStore"), m_services.store);

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
    engine.rootContext()->setContextProperty(QStringLiteral("AppSettings"), m_services.settings);

    // Connection seam: the first-run gate + Connection settings drive it; the
    // footer reads its liveness via the StatusBarModel mirror wired above.
    engine.rootContext()->setContextProperty(QStringLiteral("Connection"), m_services.connection);

    // App-level page navigation: Main.qml mounts the active manager/settings page
    // as an overlay bound to Nav.page.
    engine.rootContext()->setContextProperty(QStringLiteral("Nav"), m_services.nav);

    // First-run / onboarding gate: Main.qml mounts it over the shell until setup
    // completes. Compute the initial phase from persisted setupComplete now.
    engine.rootContext()->setContextProperty(QStringLiteral("FirstRun"), m_services.firstRun);
    m_services.firstRun->begin();

    // Daemon-authoritative config facade (mock) backing the settings sections.
    engine.rootContext()->setContextProperty(QStringLiteral("DaemonConfig"), m_services.daemonConfig);

    // Filesystem seam (dev local-disk impl) backing the file tree, finder, and editor.
    engine.rootContext()->setContextProperty(QStringLiteral("Fs"), m_services.fs);

    // Memory-inspection seam (seeded mock) backing the Memory page.
    engine.rootContext()->setContextProperty(QStringLiteral("Memory"), m_services.memory);

    // Model catalog facade (mock) backing the Models hub.
    engine.rootContext()->setContextProperty(QStringLiteral("ModelCatalog"), m_services.modelCatalog);

    // Accounts/auth facade (mock) backing the Accounts manager + wizard.
    engine.rootContext()->setContextProperty(QStringLiteral("Accounts"), m_services.accounts);

    // Profiles/agents facade (mock) backing the profile editor + curator.
    engine.rootContext()->setContextProperty(QStringLiteral("Profiles"), m_services.profiles);

    // Fleet/ops facades (mock) backing the dashboard / fleet / sessions / approvals.
    engine.rootContext()->setContextProperty(QStringLiteral("SessionRoster"), m_services.roster);
    engine.rootContext()->setContextProperty(QStringLiteral("FleetTree"), m_services.fleetTree);
    engine.rootContext()->setContextProperty(QStringLiteral("Approvals"), m_services.approvals);
    engine.rootContext()->setContextProperty(QStringLiteral("Dashboard"), m_services.dashboard);

    // Automation facades (mock) backing the routing matrix + cron manager.
    engine.rootContext()->setContextProperty(QStringLiteral("Routing"), m_services.routing);
    engine.rootContext()->setContextProperty(QStringLiteral("Cron"), m_services.cron);

    // Per-session override + checkpoint facades (mock) backing the composer popovers.
    engine.rootContext()->setContextProperty(QStringLiteral("SessionSettings"), m_services.sessionSettings);
    engine.rootContext()->setContextProperty(QStringLiteral("Checkpoints"), m_services.checkpoints);

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

void Application::openPageForRenderHarness(const QString& page, const QString& section)
{
    m_services.nav->open(page, section);
}

bool Application::awaitConnectionReady(int timeoutMs)
{
    auto* conn = m_services.connection;
    if (conn->ready()) {
        return true;
    }
    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    connect(conn, &connection::IConnectionService::stateChanged, &loop, [&] {
        if (conn->ready()) {
            loop.quit();
        }
    });
    connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
    timeout.start(timeoutMs);
    loop.exec();
    return conn->ready();
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

    // Live language switching: the DaemonApp.Settings/UiSettings singleton owns
    // the persisted `language`; reload translations and retranslate the scene
    // whenever it changes. Resolved + connected generically (QObject + property)
    // so this module needs no dependency on the Settings C++ type.
    m_uiSettings = engine.singletonInstance<QObject*>(QStringLiteral("DaemonApp.Settings"),
                                                      QStringLiteral("UiSettings"));
    if (m_uiSettings != nullptr) {
        connect(m_uiSettings, SIGNAL(languageChanged()), this, SLOT(onLanguageChanged()));
    }

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
    if (m_services.settings->setupComplete()) {
        m_services.connection->connectTo(m_services.settings->lastConnectionMode(),
                                         m_services.settings->resolvedConnectionTarget());
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

void Application::onLanguageChanged()
{
    if (m_uiSettings == nullptr) {
        return;
    }
    const QString code = m_uiSettings->property("language").toString();
    QGuiApplication::setLayoutDirection(i18n::applyLocale(code));
    if (m_engine != nullptr) {
        // Re-evaluates every qsTr()/tr() binding in the live QML scene so the UI
        // updates in place without a restart.
        m_engine->retranslate();
    }
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
