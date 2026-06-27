#include "application.h"

#include "accounts/iaccounts_service.h"
#include "app/cached_image_provider.h"
#include "app/image_cache.h"
#include "app/math_image_provider.h"
#include "automation/icron_store.h"
#include "automation/irouting_store.h"
#include "command_registry.h"
#include "config/idaemon_config.h"
#include "connection/iconnection_service.h"
#include "daemon/daemon_connection_service.h"
#include "daemon/repositories.h"
#include "daemonnet/idaemonnet.h"
#include "firstrun/first_run_model.h"
#include "fleet/iapprovals_inbox.h"
#include "fleet/idashboard.h"
#include "fleet/ifleet_tree.h"
#include "fleet/isession_roster.h"
#include "fs/ifs_service.h"
#include "i18n/localization.h"
#include "memory/imemory_service.h"
#include "models/imodel_catalog.h"
#include "nav/nav_controller.h"
#include "persistence/isession_store.h"
#include "platform/iplatform_services.h"
#include "platform/platform_services_factory.h"
#include "profiles/iprofile_store.h"
#include "session/icheckpoint_timeline.h"
#include "session/isession_settings.h"
#include "settings/isettings_store.h"
#include "status_bar_model.h"
#include "transcript_exporter.h"
#include "transports/ipresence_service.h"
#include "transports/itransport_registry.h"
#include "turn_engine_factory.h"

#include <core/formula.h>
#include <latex.h>
#include <QCoreApplication>
#include <QEvent>
#include <QEventLoop>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QString>
#include <QTimer>
#include <QUuid>

Application::Application(QObject* parent)
    : QObject(parent), m_services(daemonapp::daemon::createAppServiceGraph(
                           daemonapp::daemon::serviceModeFromEnvironment(), this)),
      m_platform(platform::createPlatformServices(this)), m_status(new StatusBarModel(this)),
      m_commands(new CommandRegistry(this)), m_exporter(new TranscriptExporter(this)) {
    // The connection seam owns liveness; mirror its state into the footer's
    // gateway indicator (the single surface for connection state).
    connect(m_services.connection, &connection::IConnectionService::stateChanged, this,
            [this] { m_status->setGatewayState(m_services.connection->state()); });

    // Turn engine: a real Submit + Subscribe engine when the connection is daemon-backed, else the
    // mock simulator. Exposed to QML as `TurnEngines`; each TranscriptPage assigns it onto its
    // SessionOrchestrator (mirroring how the other seams are gated by ServiceMode).
    if (qobject_cast<daemonapp::daemon::DaemonConnectionService*>(m_services.connection) !=
        nullptr) {
        m_turnEngines = new DaemonTurnEngineFactory(m_services.nodeApi, this);
    } else {
        m_turnEngines = new MockTurnEngineFactory(this);
    }

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

Application::~Application() {
    shutdownManagedDaemon();
    tex::LaTeX::release();
}

void Application::shutdownManagedDaemon() const {
    if (auto* daemonConnection =
            qobject_cast<daemonapp::daemon::DaemonConnectionService*>(m_services.connection)) {
        daemonConnection->shutdownManagedDaemon();
    }
}

void Application::registerContext(QQmlApplicationEngine& engine) {
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
    engine.rootContext()->setContextProperty(QStringLiteral("DaemonConfig"),
                                             m_services.daemonConfig);

    // Filesystem seam (dev local-disk impl) backing the file tree, finder, and editor.
    engine.rootContext()->setContextProperty(QStringLiteral("Fs"), m_services.fs);

    // Memory-inspection seam (seeded mock) backing the Memory page.
    engine.rootContext()->setContextProperty(QStringLiteral("Memory"), m_services.memory);

    // Model catalog facade (mock) backing the Models hub.
    engine.rootContext()->setContextProperty(QStringLiteral("ModelCatalog"),
                                             m_services.modelCatalog);

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

    // Transport-adapter seams (mock): the multi-protocol "Add channel" picker + configured
    // instances (Transports) and per-account connection/presence (Presence). See
    // daemon-transport-adapter-spec.md + docs/multi-protocol-client-surface.md.
    engine.rootContext()->setContextProperty(QStringLiteral("Transports"),
                                             m_services.transportRegistry);
    engine.rootContext()->setContextProperty(QStringLiteral("Presence"), m_services.presence);

    // The unified mock DaemonNet (actors/places + sessions-as-nodes); surfaces project from it. See
    // multi-protocol-client-surface.md §1 + the DaemonNet meta-plan.
    engine.rootContext()->setContextProperty(QStringLiteral("DaemonNet"), m_services.daemonNet);

    // Per-session override + checkpoint facades (mock) backing the composer popovers.
    engine.rootContext()->setContextProperty(QStringLiteral("SessionSettings"),
                                             m_services.sessionSettings);
    engine.rootContext()->setContextProperty(QStringLiteral("Checkpoints"), m_services.checkpoints);

    // Notifier seam: QML binds the active turn's awaitingInput signal to
    // App.notifyGate(...) to raise an OS notification when the window is hidden.
    engine.rootContext()->setContextProperty(QStringLiteral("TurnEngines"), m_turnEngines);

    engine.rootContext()->setContextProperty(QStringLiteral("App"), this);

    // The BlockEditor renderer resolves image://imgcache/<url> through the shared
    // ImageCache; instantiate it on the GUI thread and register the provider.
    be::app::ImageCache::instance();
    engine.addImageProvider(QStringLiteral("imgcache"), new be::app::CachedImageProvider);

    // LaTeX math: image://math/<payload> requests are rasterized by MicroTeX.
    engine.addImageProvider(QStringLiteral("math"), new be::app::MathImageProvider);
}

void Application::openPageForRenderHarness(const QString& page, const QString& section) const {
    m_services.nav->open(page, section);
}

bool Application::awaitConnectionReady(int timeoutMs) const {
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

void Application::driveFirstRunConnect() const {
    if (m_services.firstRun == nullptr || !m_services.firstRun->active()) {
        return; // returning users already auto-connected in completeWiring
    }
    const QString target = m_services.settings->resolvedConnectionTarget();
    m_services.settings->setLastConnection(QStringLiteral("local"), target);
    m_services.connection->connectTo(QStringLiteral("local"), target);
}

void Application::settle(int ms) const {
    QEventLoop loop;
    QTimer::singleShot(ms, &loop, &QEventLoop::quit);
    loop.exec();
}

bool Application::runHeadlessProfile(const QString& createId, const QString& model,
                                     const QString& prompt, int timeoutMs) {
    driveFirstRunConnect();
    if (!awaitConnectionReady(timeoutMs)) {
        return false;
    }
    settle(800); // let the on-ready ProfileList (+ per-profile ProfileGet) drain
    if (m_services.profiles == nullptr) {
        return true;
    }
    if (!createId.isEmpty()) {
        m_services.profiles->createProfile(createId);
        settle(800); // ProfileCreate -> re-list -> ProfileGet caches the new spec
        if (!model.isEmpty() || !prompt.isEmpty()) {
            QVariantMap fields;
            if (!model.isEmpty()) {
                fields.insert(QStringLiteral("model"), model);
            }
            if (!prompt.isEmpty()) {
                fields.insert(QStringLiteral("systemPrompt"), prompt);
            }
            m_services.profiles->updateProfile(createId, fields);
            settle(800); // flush ProfileUpdate (+ its re-list/re-get)
        }
    }
    return true;
}

QString Application::runHeadlessChat(const QString& prompt, int timeoutMs, const QString& profile) {
    driveFirstRunConnect();
    if (!awaitConnectionReady(timeoutMs)) {
        return {};
    }
    settle(600); // let the on-ready refreshes drain off the single-in-flight queue first

    auto* engine = new DaemonTurnEngine(m_services.nodeApi, this);
    engine->setSessionId(QStringLiteral("s-") + QUuid::createUuid().toString(QUuid::WithoutBraces));
    engine->setProfile(profile); // PRO-5: bind the turn to the selected profile (empty = active)

    QString answer;
    QEventLoop loop;
    connect(engine, &ITurnEngine::eventsEmitted, this, [&answer](const QVariantList& events) {
        for (const QVariant& item : events) {
            const QVariantMap map = item.toMap();
            if (map.value(QStringLiteral("type")).toString() == QStringLiteral("text")) {
                answer += map.value(QStringLiteral("text")).toString();
            }
        }
    });
    connect(engine, &ITurnEngine::turnFinished, &loop, &QEventLoop::quit);
    QTimer::singleShot(timeoutMs, &loop, &QEventLoop::quit);
    engine->start(prompt);
    loop.exec();
    engine->cancel();
    engine->deleteLater();
    return answer;
}

QString Application::runHeadlessModels(const QString& query, const QString& repo, int timeoutMs) {
    using daemonapp::daemon::ModelRepository;
    driveFirstRunConnect();
    if (!awaitConnectionReady(timeoutMs)) {
        return {};
    }
    settle(600); // let the on-ready auto-refreshes drain off the single-in-flight queue first

    auto* models = new ModelRepository(m_services.nodeApi, nullptr, this);
    const int perProbe = qMax(2500, timeoutMs / 6);

    // Shared probe state: a persistent operationFailed handler records `err`; each probe wires its
    // own success signal to record `ok`. Probes run strictly sequentially (no reentrancy).
    bool got = false;
    bool ok = false;
    QEventLoop* active = nullptr;
    const auto onFail =
        connect(models, &ModelRepository::operationFailed, this, [&](const QString&) {
            got = true;
            ok = false;
            if (active != nullptr) {
                active->quit();
            }
        });

    const auto probe = [&](auto successSignal, const std::function<void()>& trigger) -> QString {
        QEventLoop loop;
        active = &loop;
        got = false;
        ok = false;
        const auto onOk = connect(models, successSignal, this, [&] {
            got = true;
            ok = true;
            loop.quit();
        });
        QTimer::singleShot(perProbe, &loop, &QEventLoop::quit);
        trigger();
        loop.exec();
        disconnect(onOk);
        active = nullptr;
        return !got ? QStringLiteral("timeout")
                    : (ok ? QStringLiteral("ok") : QStringLiteral("err"));
    };

    // Deterministic everywhere (no manager / empty registry -> []): ModelCatalog + ModelDownloads.
    const QString catalog =
        probe(&ModelRepository::catalogChanged, [&] { models->refreshCatalog(); });
    const QString downloads =
        probe(&ModelRepository::downloadsChanged, [&] { models->refreshDownloads(); });
    // Network-dependent (HF) -> ok with a real worker/network, err otherwise; never a timeout if
    // the frame round-trips (the daemon decoded it and replied, even with an error).
    const QString search =
        probe(&ModelRepository::searchHitsChanged, [&] { models->search(query); });
    const QString files = probe(&ModelRepository::filesLoaded, [&] { models->requestFiles(repo); });
    const QString download = probe(&ModelRepository::downloadStarted,
                                   [&] { models->download(repo, QStringLiteral("model.gguf")); });

    disconnect(onFail);
    models->deleteLater();
    return QStringLiteral("catalog=%1 downloads=%2 search=%3 files=%4 download=%5")
        .arg(catalog, downloads, search, files, download);
}

bool Application::runHeadlessOnboarding(const QString& provider, const QString& key,
                                        int timeoutMs) {
    driveFirstRunConnect();
    if (!awaitConnectionReady(timeoutMs)) {
        return false;
    }
    // Let the on-ready auto-refreshes (ProfileList / CredentialList / Models / ModelCurrent) flush
    // so the active profile + discovered models are known before we add the key / pick a model.
    settle(1000);
    if (!key.isEmpty() && m_services.accounts != nullptr) {
        m_services.accounts->addApiKey(provider, QString(), key, QString());
    }
    if (m_services.modelCatalog != nullptr) {
        const QStringList ids = m_services.modelCatalog->installedIds();
        if (!ids.isEmpty()) {
            m_services.modelCatalog->activate(ids.first());
        }
    }
    if (m_services.firstRun != nullptr) {
        m_services.firstRun->completeInference();
    }
    // Flush the CredentialSet (+ its re-list) round-trip before the harness exits.
    settle(1000);
    return true;
}

void Application::completeWiring(QQmlApplicationEngine& engine) {
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

bool Application::notifyGate(const QString& title, const QString& body) {
    if (m_platform == nullptr) {
        return false;
    }
    // An on-screen, active window already shows the inline gate, so only alert
    // when the window is hidden, minimized, or not the active window.
    if (m_window != nullptr && m_window->isVisible() &&
        m_window->visibility() != QWindow::Minimized && m_window->isActive()) {
        return false;
    }
    return m_platform->notify(title, body);
}

void Application::onLanguageChanged() {
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

bool Application::eventFilter(QObject* watched, QEvent* event) {
    if (watched == m_window && event->type() == QEvent::Close) {
        event->ignore();
        m_window->hide(); // hide to tray; the app keeps running
        return true;      // swallow the close so the window is not destroyed
    }
    return QObject::eventFilter(watched, event);
}
