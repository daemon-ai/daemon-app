// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "application.h"

#include "accounts/iaccounts_service.h"
#include "app/cached_image_provider.h"
#include "app/image_cache.h"
#include "app/math_image_provider.h"
#include "auth/auth_flow_controller.h"
#include "automation/icron_store.h"
#include "command_registry.h"
#include "config/idaemon_config.h"
#include "connection/iconnection_service.h"
#include "daemon/daemon_cache_store.h"
#include "daemon/daemon_connection_service.h"
#include "daemon/engine_identity.h"
#include "daemon/node_api_client.h"
#include "daemon/node_api_codec.h"
#include "daemon/principal_model.h"
#include "daemon/repositories.h"
#include "feedback/ifeedback.h"
#include "firstrun/first_run_model.h"
#include "fleet/iapprovals_inbox.h"
#include "fleet/idashboard.h"
#include "fleet/ifleet_tree.h"
#include "fleet/isession_roster.h"
#include "fs/ifs_service.h"
#include "i18n/localization.h"
#include "memory/imemory_service.h"
#include "models/imodel_catalog.h"
#include "models/iprovider_catalog.h"
#include "nav/nav_controller.h"
#include "persistence/isession_store.h"
#include "platform/iplatform_services.h"
#include "platform/platform_services_factory.h"
#include "platform/wasm_contracts.h"
#include "profiles/iprofile_store.h"
#include "session/icheckpoint_timeline.h"

// [mirror M2] Complete types for the Mirror/Outbox context-property upcasts (spec 09 §2/§6).
#include "mirror/mirror_service.h"
#include "outbox.h"
#include "session/isession_settings.h"
#include "settings/isettings_store.h"
#include "setup/agent_setup_model.h"
#include "status_bar_model.h"
#include "tools/itool_inventory.h" // [wave2:app-approvals-safety] D2
#include "transcript_exporter.h"
#include "transports/ichat_service.h"
#include "transports/icontacts_service.h"
#include "transports/ipersons_service.h"
#include "transports/ipresence_service.h"
#include "transports/itransport_registry.h"
#include "turn_engine_factory.h"
#include "update/update_manager.h"

#include <core/formula.h>
#include <cstdio>
#include <latex.h>
#include <memory>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QEvent>
#include <QEventLoop>
#include <QFile>
#include <QGuiApplication>
#ifdef Q_OS_ANDROID
#include <QDirIterator>
#include <QFileInfo>
#include <QStandardPaths>
#endif
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QSettings>
#include <QStandardPaths>
#include <QString>
#include <QTimer>
#include <QUuid>
#include <QVariantList>
#include <QVariantMap>
#include <string>

namespace {
#ifdef Q_OS_ANDROID
// Android ships the res tree inside the APK (assets/microtex-res, packed via
// androiddeployqt's assets mechanism - see App/CMakeLists.txt). Assets are
// not POSIX paths and MicroTeX reads plain files, so extract the tree to the
// app data dir on first boot (marker: the versioned stamp file, so an app
// update refreshes the copy) and resolve from there.
QString androidExtractMicrotexRes() {
    const QDir dataRoot(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    const QString target = dataRoot.filePath(QStringLiteral("microtex-res"));
    const QString stampPath =
        target + QStringLiteral("/.extracted-") + QCoreApplication::applicationVersion();
    if (QFile::exists(stampPath)) {
        return target;
    }
    QDir(target).removeRecursively(); // stale/partial copy from an older version
    const QString assetRoot = QStringLiteral("assets:/microtex-res");
    QDirIterator it(assetRoot, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString asset = it.next();
        const QString relative = asset.mid(assetRoot.size() + 1);
        const QString destination = target + QLatin1Char('/') + relative;
        QDir().mkpath(QFileInfo(destination).absolutePath());
        QFile::copy(asset, destination);
    }
    QFile stamp(stampPath);
    if (!stamp.open(QIODevice::WriteOnly)) {
        qWarning("microtex: could not write extraction stamp %s", qPrintable(stampPath));
    }
    return target;
}
#endif

// MicroTeX's fonts/XML ship with the app (share/daemon-app/microtex-res, see
// App/CMakeLists.txt) so the installed binary carries no reference to the
// build-time source tree. Resolution order: explicit env override, then the
// installed copies relative to the binary (prefix, flat portable, and macOS
// bundle layouts), then the compile-time MICROTEX_RES_DIR - the build-tree
// fallback for dev runs. The browser build skips the probing: there
// MICROTEX_RES_DIR is the fixed MEMFS mount the emscripten preload bundle
// fills. Android skips it too: the tree is extracted from the APK's assets on
// first boot.
std::string microtexResDir() {
#ifdef Q_OS_ANDROID
    return androidExtractMicrotexRes().toStdString();
#endif
#ifndef Q_OS_WASM
    const QString envOverride = qEnvironmentVariable("DAEMON_APP_MICROTEX_RES");
    if (!envOverride.isEmpty()) {
        return envOverride.toStdString();
    }
    const QString appDir = QCoreApplication::applicationDirPath();
    for (const QString& candidate : {appDir + QStringLiteral("/../share/daemon-app/microtex-res"),
                                     appDir + QStringLiteral("/microtex-res"),
                                     appDir + QStringLiteral("/../Resources/microtex-res")}) {
        if (QDir(candidate).exists()) {
            return QDir::cleanPath(candidate).toStdString();
        }
    }
#endif
    return MICROTEX_RES_DIR;
}
} // namespace

#ifdef Q_OS_WASM
#include <emscripten.h>
#endif

Application::Application(QObject* parent)
    : QObject(parent), m_services(daemonapp::daemon::createAppServiceGraph(
                           daemonapp::daemon::serviceModeFromEnvironment(), this)),
      m_platform(platform::createPlatformServices(this)), m_status(new StatusBarModel(this)),
      m_commands(new CommandRegistry(this)), m_exporter(new TranscriptExporter(this)) {
    // The connection seam owns liveness; mirror its state into the footer's
    // gateway indicator (the single surface for connection state).
    connect(m_services.connection, &connection::IConnectionService::stateChanged, this,
            [this] { m_status->setGatewayState(m_services.connection->state()); });

    // Phase F: mirror the node's OpenAI-gateway status (GatewayRepository) and the retained node
    // service health (DaemonConnectionService) into the shared footer model — DISTINCT from the
    // connection-liveness gatewayState above. Daemon mode only (gatewayRepository is null in mock).
    if (m_services.gatewayRepository != nullptr) {
        auto* gw = m_services.gatewayRepository;
        auto syncGateway = [this, gw] {
            m_status->setOpenAiGatewayStatus(gw->supported(), gw->enabled(), gw->listening(),
                                             gw->lastError());
        };
        connect(gw, &daemonapp::daemon::GatewayRepository::statusChanged, m_status, syncGateway);
        syncGateway();
    }
    if (auto* daemonConn =
            qobject_cast<daemonapp::daemon::DaemonConnectionService*>(m_services.connection)) {
        auto syncHealth = [this, daemonConn] {
            QVariantList services;
            for (const auto& svc : daemonConn->healthServices()) {
                services.append(QVariantMap{
                    {QStringLiteral("name"), svc.name},
                    {QStringLiteral("ok"), svc.ok},
                    {QStringLiteral("restarts"), svc.restarts},
                    {QStringLiteral("detail"), svc.detail},
                });
            }
            m_status->setHealthServices(services);
        };
        connect(daemonConn, &daemonapp::daemon::DaemonConnectionService::healthChanged, m_status,
                syncHealth);
        syncHealth();
    }

    // Turn engine: a real Submit + Subscribe engine when the connection is daemon-backed, else the
    // mock simulator. Exposed to QML as `TurnEngines`; each TranscriptPage assigns it onto its
    // SessionOrchestrator (mirroring how the other seams are gated by ServiceMode).
    if (qobject_cast<daemonapp::daemon::DaemonConnectionService*>(m_services.connection) !=
        nullptr) {
        m_turnEngines = new DaemonTurnEngineFactory(m_services.nodeApi, m_services.cache,
                                                    m_services.subscriptions,
                                                    m_services.transcriptMirrorSink, this);
    } else {
        m_turnEngines = new MockTurnEngineFactory(this);
    }

    // MicroTeX loads its fonts/XML resources once, from the directory resolved
    // at startup (see microtexResDir above). Done here so the "math" image
    // provider can parse formulas as soon as the scene requests them.
    tex::LaTeX::init(microtexResDir());

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

void Application::emitOpenChat() {
    const QString profileId =
        m_services.profiles != nullptr ? m_services.profiles->defaultProfileId() : QString();
    emit openChatRequested(profileId);
}

void Application::openFirstChat() {
    emitOpenChat();
}

void Application::openNewAgentChat() {
    emitOpenChat();
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

    // mirror A7 (M4) → AD: the ONE session store EVERY session/fleet consumer binds (6→1
    // unification) — the real MirrorSessionStore projection in both modes (daemon: ingestor-fed;
    // mock: scenario-seeded). The legacy `SessionStore` rollback binding died with the legacy
    // stores.
    engine.rootContext()->setContextProperty(QStringLiteral("SessionStoreMirror"),
                                             m_services.storeMirror);

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

    // Authenticated principal (WhoAmI): advisory capability gating of admin/ownership surfaces.
    // The node still enforces every capability server-side; this only hides/shows UI.
    engine.rootContext()->setContextProperty(QStringLiteral("Principal"), m_services.principal);

    // Hide capability-gated command-palette entries (e.g. Users & Access) from principals that lack
    // them, and re-filter whenever the principal changes (login/logout/role change).
    if (m_services.principal != nullptr) {
        auto* principal = m_services.principal;
        auto applyCaps = [this, principal] {
            m_commands->setCapabilityProvider(
                [principal](const QString& cap) { return principal->hasCapability(cap); });
        };
        applyCaps();
        connect(principal, &daemonapp::daemon::PrincipalModel::changed, m_commands, applyCaps);
    }

    // App-level page navigation: Main.qml mounts the active manager/settings page
    // as an overlay bound to Nav.page.
    engine.rootContext()->setContextProperty(QStringLiteral("Nav"), m_services.nav);

    // First-run / onboarding gate: Main.qml mounts it over the shell until setup
    // completes. Compute the initial phase from persisted setupComplete now.
    engine.rootContext()->setContextProperty(QStringLiteral("FirstRun"), m_services.firstRun);
    // P0-B: when onboarding finishes (the wizard's final step configured + set the default
    // profile), open the first chat so the user lands in a working transcript instead of an empty
    // shell. The shell (Main.qml) handles openChatRequested by opening a transcript tab.
    connect(m_services.firstRun, &firstrun::FirstRunModel::finished, this,
            &Application::openFirstChat);
    m_services.firstRun->begin();

    // Daemon-authoritative config facade (mock) backing the settings sections.
    engine.rootContext()->setContextProperty(QStringLiteral("DaemonConfig"),
                                             m_services.daemonConfig);

    // User-feedback + telemetry-consent seam (mock). Backs the transcript thumbs,
    // the status-bar app-feedback dialog, and the Advanced settings consent row
    // (the node-owned source of truth the FeedbackController routes through).
    engine.rootContext()->setContextProperty(QStringLiteral("Feedback"), m_services.feedback);

    // Filesystem seam (dev local-disk impl) backing the file tree, finder, and editor.
    engine.rootContext()->setContextProperty(QStringLiteral("Fs"), m_services.fs);

    // Memory-inspection seam (seeded mock) backing the Memory page.
    engine.rootContext()->setContextProperty(QStringLiteral("Memory"), m_services.memory);

    // Model catalog facade (mock) backing the Models hub.
    engine.rootContext()->setContextProperty(QStringLiteral("ModelCatalog"),
                                             m_services.modelCatalog);

    // Provider/model discovery facade: node-driven provider list (ProviderCatalog) + per-provider
    // model list (ProviderModels) backing the ProfileEditor + first-run provider->model pickers.
    engine.rootContext()->setContextProperty(QStringLiteral("ProviderCatalog"),
                                             m_services.providerCatalog);

    // Foreign-agent catalog (foreign engines; wire v29): backs the engine picker + Agents settings.
    // Null in mock mode — QML guards with `typeof Agents !== "undefined" && Agents`.
    engine.rootContext()->setContextProperty(QStringLiteral("Agents"), m_services.agents);
    // Shared engine-identity facade (C3/C4/C5): session/profile -> engine chip label. Consumed by
    // ComposerControls' engine chip and the approval EngineOriginChip (C5).
    engine.rootContext()->setContextProperty(QStringLiteral("EngineIdentity"),
                                             m_services.engineIdentity);

    // Shared agent/profile setup view-model (Phase C): the one engine+backend+inference pipeline
    // the setup surfaces (New-agent dialog, Profiles +New, editor, first-run) bind to.
    engine.rootContext()->setContextProperty(QStringLiteral("AgentSetup"), m_services.agentSetup);

    // Accounts/auth facade (mock) backing the Accounts manager + wizard.
    engine.rootContext()->setContextProperty(QStringLiteral("Accounts"), m_services.accounts);

    // Interactive-auth flow view-model (begin -> browser -> complete; AuthFlowSheet).
    engine.rootContext()->setContextProperty(QStringLiteral("AuthFlow"),
                                             m_services.authFlowController);

    // Profiles/agents facade (mock) backing the profile editor + curator.
    engine.rootContext()->setContextProperty(QStringLiteral("Profiles"), m_services.profiles);

    // Fleet/ops facades (mock) backing the dashboard / fleet / sessions / approvals.
    engine.rootContext()->setContextProperty(QStringLiteral("SessionRoster"), m_services.roster);
    engine.rootContext()->setContextProperty(QStringLiteral("FleetTree"), m_services.fleetTree);
    engine.rootContext()->setContextProperty(QStringLiteral("Approvals"), m_services.approvals);
    // [wave2:app-approvals-safety] D2: read-only tool inventory (Settings -> Tools).
    engine.rootContext()->setContextProperty(QStringLiteral("Tools"), m_services.tools);
    engine.rootContext()->setContextProperty(QStringLiteral("Dashboard"), m_services.dashboard);

    // Automation facade (mock) backing the cron manager. (The routing manager reads the mirror
    // store; see the RoutingActions context property below.)
    engine.rootContext()->setContextProperty(QStringLiteral("Cron"), m_services.cron);

    // Transport-adapter seams (mock): the multi-protocol "Add channel" picker + configured
    // instances (Transports) and per-account connection/presence (Presence). See
    // daemon-transport-adapter-spec.md + docs/multi-protocol-client-surface.md.
    engine.rootContext()->setContextProperty(QStringLiteral("Transports"),
                                             m_services.transportRegistry);
    engine.rootContext()->setContextProperty(QStringLiteral("Presence"), m_services.presence);
    // [acct-mgmt] Transport contacts / roster (Phase D, wire v34): the per-account Contacts section
    // in ChannelsPage binds this seam (RosterList + contact ops).
    engine.rootContext()->setContextProperty(QStringLiteral("Contacts"), m_services.contacts);
    // [integrations wire v38] The cross-transport person registry (PersonList): the integrations
    // tree's Persons sections bind this seam (endpoints per transport).
    engine.rootContext()->setContextProperty(QStringLiteral("Persons"), m_services.persons);
    // [integrations wire v38] Native chat (A4): ChatPage's ChatConversationController binds this
    // IChatService seam (ConvHistory transcript + ConvSend + MessagesChanged refresh).
    engine.rootContext()->setContextProperty(QStringLiteral("Chat"), m_services.chat);
    // [mirror M2] The mirror composition root + durable outbox (spec 09 §2/§6). Daemon mode only
    // at M2 (null in mock — the chat surfaces fall back to the legacy seams until A8's seeder):
    // ChatPage reads the ChatWindowModel timeline via `Mirror` and routes sends through the
    // ConvSend outbox lane via `Outbox` (manual drain; §6.8 auto-replay stays off).
    engine.rootContext()->setContextProperty(QStringLiteral("Mirror"), m_services.mirrorService);
    engine.rootContext()->setContextProperty(QStringLiteral("Outbox"), m_services.outbox);

    // Routing manager (M3): the RoutingPage's RoutingManagerController reads the pin table off the
    // mirror store (`Mirror`) and drives pin/unbind mutations through the node-authoritative
    // RoutingActions seam (RoutingRepository IS-A daemonnet::IRoutingActions; null in mock).
    engine.rootContext()->setContextProperty(QStringLiteral("RoutingActions"),
                                             m_services.routingRepository);

    // Per-session override + checkpoint facades (mock) backing the composer popovers.
    engine.rootContext()->setContextProperty(QStringLiteral("SessionSettings"),
                                             m_services.sessionSettings);
    engine.rootContext()->setContextProperty(QStringLiteral("Checkpoints"), m_services.checkpoints);
    // [wave2:app-delegation] F7/DEL-7: read-only delegation guardrail ceilings (null in mock mode;
    // the Settings section binds defensively on `Caps && Caps.loaded`).
    engine.rootContext()->setContextProperty(QStringLiteral("Caps"), m_services.capsRepository);
    // Phase F: the node OpenAI-gateway control seam (GatewayGet/GatewaySet). Null in mock mode; the
    // GatewaySection binds defensively on `Gateway && Gateway.supported`.
    engine.rootContext()->setContextProperty(QStringLiteral("Gateway"),
                                             m_services.gatewayRepository);

    // Release-feed / auto-update surface: the shell binds this for the update
    // banner + settings toggle. Inert (no feed, no UI) unless the package job
    // compiled in a capability dial (packaging/UPDATES.md).
    engine.rootContext()->setContextProperty(QStringLiteral("Update"), m_services.update);
    // Mirror an available update into the shared footer model so the GUI status
    // bar (and the TUI, via the same model) surface it from one source.
    if (m_services.update != nullptr && m_status != nullptr) {
        auto* upd = m_services.update;
        auto syncUpdate = [this, upd] {
            const bool offered =
                upd->stateName() == QStringLiteral("UpdateAvailable") && !upd->dismissed();
            m_status->setUpdateVersion(offered ? upd->latestVersion() : QString());
        };
        connect(upd, &update::UpdateManager::stateChanged, m_status, syncUpdate);
        syncUpdate();
    }

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

bool Application::isDaemonBacked() const {
    return qobject_cast<daemonapp::daemon::DaemonConnectionService*>(m_services.connection) !=
           nullptr;
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
#ifdef Q_OS_WASM
    // The browser's only transport is the WebSocket mux; drive the resolved remote-ws target
    // (the `?ws=` page override or the page-origin `/ws` default) instead of a local socket.
    const QString mode = QStringLiteral("remote-ws");
#else
    const QString mode = QStringLiteral("local");
#endif
    const QString target = m_services.settings->resolvedConnectionTarget();
    m_services.settings->setLastConnection(mode, target);
    m_services.connection->connectTo(mode, target);
}

void Application::settleFirstRunGate(int ms) const {
    firstrun::FirstRunModel* firstRun = m_services.firstRun;
    if (firstRun == nullptr || firstRun->phase() == QStringLiteral("done")) {
        return;
    }
    QEventLoop loop;
    const auto conn =
        connect(firstRun, &firstrun::FirstRunModel::phaseChanged, &loop, [firstRun, &loop] {
            if (firstRun->phase() == QStringLiteral("done")) {
                loop.quit();
            }
        });
    QTimer::singleShot(ms, &loop, &QEventLoop::quit);
    loop.exec();
    disconnect(conn);
}

void Application::installAutomationLoginHook(const QString& userPass) const {
    auto* conn = m_services.connection;
    if (conn == nullptr || userPass.isEmpty()) {
        return;
    }
    // "user:pass" — split on the first colon so a password may itself contain colons.
    const qsizetype sep = userPass.indexOf(QLatin1Char(':'));
    const QString user = sep >= 0 ? userPass.left(sep) : userPass;
    const QString pass = sep >= 0 ? userPass.mid(sep + 1) : QString();
    // Fire login() the first time the node asks for interactive credentials. A shared one-shot
    // guard prevents re-driving login on a later authenticating transition (a wrong-password retry
    // must not loop). The lambda is owned by the connection object, so it lives as long as it does.
    auto fired = std::make_shared<bool>(false);
    connect(conn, &connection::IConnectionService::stateChanged, conn, [conn, user, pass, fired] {
        if (*fired || conn->state() != QStringLiteral("authenticating")) {
            return;
        }
        *fired = true;
        conn->login(user, pass);
    });
}

#ifdef Q_OS_WASM
void Application::announceConnectionReady(int timeoutMs) const {
    auto* conn = m_services.connection;
    const auto announce = [](bool ok) {
        std::fprintf(stdout, "DAEMON_APP_READY %s\n", ok ? "ok" : "timeout");
        std::fflush(stdout);
    };
    if (conn->ready()) {
        announce(true);
        return;
    }
    // The timer doubles as the connection context: resolving either way tears the other arm
    // down (stop + deleteLater kills the pending timeout; deleting the timer disconnects the
    // state watcher), so the sentinel prints exactly once.
    auto* timeout = new QTimer(conn);
    timeout->setSingleShot(true);
    connect(conn, &connection::IConnectionService::stateChanged, timeout,
            [conn, timeout, announce] {
                if (conn->ready()) {
                    timeout->stop();
                    timeout->deleteLater();
                    announce(true);
                }
            });
    connect(timeout, &QTimer::timeout, conn, [timeout, announce] {
        timeout->deleteLater();
        announce(false);
    });
    timeout->start(timeoutMs);
}

void Application::announceReloadSentinels() const {
    // (1) Cache rows at boot. This read is on the PRE-AUTH default cache namespace, which is empty
    // by design (the durable rows live in the per-user db that AuthOk opens - see (1b)); a 0 here
    // on a fresh boot is a valid sentinel line for the harness (it proves the emitter path is
    // wired).
    if (m_services.cache != nullptr) {
        std::fprintf(stdout, "%s%d\n", platform::kSentinelCacheRowsPrefix,
                     m_services.cache->approxRowCount());
        std::fflush(stdout);
    }

    // (1b) Re-emit the cache-row count once the per-user namespace is active, BEFORE the
    // on-ready baseline re-fetches. The per-user db is opened by AuthOk (cache->setUserNamespace in
    // app_service_graph.cpp, which fires on NodeApiClient::authenticated and precedes "ready"); on
    // a resume that db is the IDBFS-preloaded copy, so this reading is a true PRE-FETCH proof that
    // the SQLite cache survived the reload (the reload-survival harness asserts rows>0 on load 2
    // here, not on the boot line above). This connect is registered after the graph's own
    // `authenticated` handler, so Qt runs it after setUserNamespace has switched the namespace.
    if (m_services.nodeApi != nullptr && m_services.cache != nullptr) {
        auto* cache = m_services.cache;
        connect(
            m_services.nodeApi, &daemonapp::daemon::NodeApiClient::authenticated, cache,
            [cache] {
                std::fprintf(stdout, "%s%d\n", platform::kSentinelCacheRowsPrefix,
                             cache->approxRowCount());
                std::fflush(stdout);
            },
            Qt::SingleShotConnection);
    }

    // (2) Auth outcome: resumed (AuthOk via a persisted token, no SCRAM challenge) vs scram (a
    // fresh interactive login). Printed once on the first AuthOk; the connection service reports
    // which path it took. Wired before the async handshake completes, so it is never missed.
    if (auto* daemonConn =
            qobject_cast<daemonapp::daemon::DaemonConnectionService*>(m_services.connection)) {
        connect(
            daemonConn, &daemonapp::daemon::DaemonConnectionService::authOutcome, daemonConn,
            [](bool resumed) {
                std::fprintf(stdout, "%s\n",
                             resumed ? platform::kSentinelAuthResumed
                                     : platform::kSentinelAuthScram);
                std::fflush(stdout);
            },
            Qt::SingleShotConnection);
    }

    // (3) First-run gate completion, async (a nested QEventLoop is forbidden on single-threaded
    // wasm): print once the phase reaches "done"; if it is already done at boot, print now. A
    // shared guard keeps it to exactly one line even if the phase toggles.
    firstrun::FirstRunModel* firstRun = m_services.firstRun;
    if (firstRun == nullptr) {
        return;
    }
    const auto emitFirstRunDone = [] {
        std::fprintf(stdout, "%s\n", platform::kSentinelFirstRunDone);
        std::fflush(stdout);
    };
    if (firstRun->phase() == QStringLiteral("done")) {
        emitFirstRunDone();
        return;
    }
    auto printed = std::make_shared<bool>(false);
    connect(firstRun, &firstrun::FirstRunModel::phaseChanged, firstRun,
            [firstRun, printed, emitFirstRunDone] {
                if (!*printed && firstRun->phase() == QStringLiteral("done")) {
                    *printed = true;
                    emitFirstRunDone();
                }
            });
}
#endif

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
            if (!model.isEmpty()) {
                m_services.profiles->updateProfile(createId, {{QStringLiteral("model"), model}});
            }
            if (!prompt.isEmpty()) {
                // The persona rides the persona seam (SoulSet), not the profile spec.
                m_services.profiles->setSoul(createId, prompt);
            }
            settle(800); // flush the update/persona writes (+ their re-list/re-get)
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

    auto* engine =
        new DaemonTurnEngine(m_services.nodeApi, m_services.cache, m_services.subscriptions,
                             m_services.transcriptMirrorSink, this);
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

QString Application::runHeadlessFleet(int timeoutMs) {
    using daemonapp::daemon::FleetRepository;
    driveFirstRunConnect();
    if (!awaitConnectionReady(timeoutMs)) {
        return {};
    }
    settle(600);
    FleetRepository* repo = m_services.fleetRepository;
    mirror::MirrorService* svc = m_services.mirrorService;
    if (repo == nullptr || svc == nullptr) {
        return {};
    }
    // AD: the tree feed is the mirror's — refetch the Tree into `fleet_units` and count the
    // mirrored rows (the same read path the Fleet page renders).
    int units = -1;
    {
        QEventLoop loop;
        const auto c = connect(&svc->store(), &mirror::Store::committed, this, [&] {
            units = static_cast<int>(svc->store().snapshot().fleet_units.size());
            loop.quit();
        });
        QTimer::singleShot(qMax(2000, timeoutMs / 2), &loop, &QEventLoop::quit);
        svc->ingestor().refetchFleet();
        loop.exec();
        disconnect(c);
    }
    // Exercise the control wire op (result irrelevant: a fresh daemon has no unit, so this is an
    // error/Unsupported - the proxy records the Pause frame crossing either way).
    repo->pause(QStringLiteral("probe-unit"));
    settle(300);
    return QStringLiteral("units=%1").arg(units);
}

QString Application::runHeadlessChannels(int timeoutMs) {
    using daemonapp::daemon::TransportRepository;
    driveFirstRunConnect();
    if (!awaitConnectionReady(timeoutMs)) {
        return {};
    }
    settle(600);
    TransportRepository* repo = m_services.transportRepository;
    if (repo == nullptr) {
        return {};
    }
    int adapters = -1;
    {
        QEventLoop loop;
        const auto c = connect(repo, &TransportRepository::adaptersRefreshed, this, [&] {
            adapters = static_cast<int>(repo->adapters().size());
            loop.quit();
        });
        QTimer::singleShot(qMax(2000, timeoutMs / 3), &loop, &QEventLoop::quit);
        repo->refreshAdapters();
        loop.exec();
        disconnect(c);
    }
    int instances = -1;
    {
        QEventLoop loop;
        const auto c = connect(repo, &TransportRepository::instancesRefreshed, this, [&] {
            instances = static_cast<int>(repo->cachedInstances().size());
            loop.quit();
        });
        QTimer::singleShot(qMax(2000, timeoutMs / 3), &loop, &QEventLoop::quit);
        repo->refreshInstances();
        loop.exec();
        disconnect(c);
    }
    // EIO-8: exercise the ConvList crossing (a fresh daemon may have no rooms; the proxy records
    // the frame either way).
    repo->refreshConversations(QStringLiteral("probe"));
    settle(300);
    return QStringLiteral("adapters=%1 instances=%2").arg(adapters).arg(instances);
}

QString Application::runHeadlessFs(int timeoutMs) {
    driveFirstRunConnect();
    if (!awaitConnectionReady(timeoutMs)) {
        return {};
    }
    settle(600); // let the on-ready auto-refreshes (incl. the first listRoots) drain first
    fs::IFsService* fs = m_services.fs;
    if (fs == nullptr) {
        return {};
    }
    const int perProbe = qMax(2500, timeoutMs / 5);
    const auto runLoop = [&](const QMetaObject::Connection& c,
                             const std::function<void()>& trigger) {
        QEventLoop loop;
        m_fsLoop = &loop;
        QTimer::singleShot(perProbe, &loop, &QEventLoop::quit);
        trigger();
        loop.exec();
        m_fsLoop = nullptr;
        disconnect(c);
    };

    // listRoots -> rootsChanged (re-issue to be deterministic; capture the first root id).
    QString rootId;
    qsizetype rootCount = 0;
    runLoop(connect(fs, &fs::IFsService::rootsChanged, this,
                    [&](const QList<fs::FsRoot>& roots) {
                        rootCount = roots.size();
                        // Prefer the writable workspace root (Host browse roots are read-only, so a
                        // write probe there would fail); fall back to the first listed root.
                        for (const fs::FsRoot& r : roots) {
                            if (r.id == QStringLiteral("workspace")) {
                                rootId = r.id;
                                break;
                            }
                        }
                        if (rootId.isEmpty() && !roots.isEmpty()) {
                            rootId = roots.first().id;
                        }
                        if (m_fsLoop != nullptr) {
                            m_fsLoop->quit();
                        }
                    }),
            [&] { fs->listRoots(); });
    if (rootId.isEmpty()) {
        return QStringLiteral("roots=0");
    }

    // open(root, "") -> listed.
    qsizetype listCount = 0;
    runLoop(connect(fs, &fs::IFsService::listed, this,
                    [&](const QString&, const QString&, const QList<fs::FsEntry>& entries) {
                        listCount = entries.size();
                        if (m_fsLoop != nullptr) {
                            m_fsLoop->quit();
                        }
                    }),
            [&] { fs->open(rootId, QString()); });

    // write a probe file -> writeResult.
    const QString probePath = QStringLiteral("daemon-app-e2e-probe.txt");
    const QByteArray content = QByteArrayLiteral("offline-first fs probe\n");
    bool wrote = false;
    runLoop(connect(fs, &fs::IFsService::writeResult, this,
                    [&](const QString&, const QString&, bool ok, const QString&, const QString&) {
                        wrote = ok;
                        if (m_fsLoop != nullptr) {
                            m_fsLoop->quit();
                        }
                    }),
            [&] { fs->write(rootId, probePath, content, QString(), false); });

    // read it back -> fileRead.
    QByteArray readBack;
    runLoop(connect(fs, &fs::IFsService::fileRead, this,
                    [&](const QString&, const QString&, const QByteArray& bytes, const QString&,
                        bool, bool) {
                        readBack = bytes;
                        if (m_fsLoop != nullptr) {
                            m_fsLoop->quit();
                        }
                    }),
            [&] { fs->read(rootId, probePath); });

    return QStringLiteral("roots=%1 root=%2 list=%3 write=%4 read=%5")
        .arg(QString::number(rootCount), rootId, QString::number(listCount),
             wrote ? QStringLiteral("ok") : QStringLiteral("err"),
             readBack == content ? QStringLiteral("ok") : QStringLiteral("mismatch"));
}

namespace {
// Send one request over the client and block (bounded) until its correlated response arrives,
// returning the response CBOR (empty on timeout). Used by the CHA-7/8 headless harness hooks to
// exercise a single wire op over the real NodeApiClient.
QByteArray requestSync(daemonapp::daemon::NodeApiClient* client, const QByteArray& requestCbor,
                       const QString& correlation, int timeoutMs) {
    if (client == nullptr || requestCbor.isEmpty()) {
        return {};
    }
    QByteArray response;
    QEventLoop loop;
    auto* ctx = new QObject;
    QObject::connect(client, &daemonapp::daemon::NodeApiClient::responseReady, ctx,
                     [&](const QString& id, const QByteArray& cbor) {
                         if (id == correlation) {
                             response = cbor;
                             loop.quit();
                         }
                     });
    QObject::connect(client, &daemonapp::daemon::NodeApiClient::failed, ctx,
                     [&](const QString& id, const QString&) {
                         if (id == correlation) {
                             loop.quit();
                         }
                     });
    QTimer::singleShot(timeoutMs, &loop, &QEventLoop::quit);
    client->sendRequest(requestCbor, correlation);
    loop.exec();
    delete ctx;
    return response;
}
} // namespace

QString Application::runHeadlessCommands(const QString& invoke, int timeoutMs) {
    using daemonapp::daemon::NodeApiCodec;
    driveFirstRunConnect();
    if (!awaitConnectionReady(timeoutMs)) {
        return {};
    }
    settle(400);
    const QByteArray listed =
        requestSync(m_services.nodeApi, NodeApiCodec::encodeCommandListRequest(),
                    QStringLiteral("harness/commands"), timeoutMs);
    QList<daemonapp::daemon::DecodedCommandSpec> commands;
    NodeApiCodec::decodeCommands(listed, &commands);
    if (!invoke.isEmpty()) {
        const QByteArray out =
            requestSync(m_services.nodeApi, NodeApiCodec::encodeCommandInvokeRequest(invoke),
                        QStringLiteral("harness/command-invoke"), timeoutMs);
        QString text;
        NodeApiCodec::decodeCommandOutput(out, &text);
        return text;
    }
    QStringList names;
    names.reserve(commands.size());
    for (const daemonapp::daemon::DecodedCommandSpec& c : commands) {
        names << c.name;
    }
    return names.join(QLatin1Char('\n'));
}

QString Application::runHeadlessSearch(const QString& query, int timeoutMs) {
    using daemonapp::daemon::NodeApiCodec;
    driveFirstRunConnect();
    if (!awaitConnectionReady(timeoutMs)) {
        return {};
    }
    settle(400);
    const QByteArray hitsCbor =
        requestSync(m_services.nodeApi, NodeApiCodec::encodeSessionSearchRequest(query, 16),
                    QStringLiteral("harness/session-search"), timeoutMs);
    QList<daemonapp::daemon::DecodedSessionSearchHit> hits;
    NodeApiCodec::decodeSessionSearch(hitsCbor, &hits);
    QStringList sessions;
    sessions.reserve(hits.size());
    for (const daemonapp::daemon::DecodedSessionSearchHit& h : hits) {
        sessions << h.session;
    }
    return sessions.join(QLatin1Char('\n'));
}

QString Application::runHeadlessHitl(const QString& prompt, const QString& decision,
                                     int timeoutMs) {
    driveFirstRunConnect();
    if (!awaitConnectionReady(timeoutMs)) {
        return {};
    }
    settle(600);

    auto* engine =
        new DaemonTurnEngine(m_services.nodeApi, m_services.cache, m_services.subscriptions,
                             m_services.transcriptMirrorSink, this);
    engine->setSessionId(QStringLiteral("s-") + QUuid::createUuid().toString(QUuid::WithoutBraces));

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

    // A live socket Submit under the default Ask policy surfaces the gated tool as an IN-STREAM
    // HostRequest on the Subscribe/merged-log stream (proven by daemon-node's
    // node_interface::live_approval_park_then_respond conformance test) - NOT the durable
    // ApprovalsPending inbox (that surface is for assigned/durable sessions). So resolve the parked
    // gate over the wire with Respond: on the engine's awaitingInput signal, answer per `decision`.
    connect(engine, &ITurnEngine::awaitingInput, this, [engine, decision](const QString& kind) {
        if (kind == QStringLiteral("approval")) {
            engine->respondApproval(QString(), decision != QStringLiteral("deny"));
        } else if (kind == QStringLiteral("clarify")) {
            engine->respondChoice(QString(), 0);
        } else if (kind == QStringLiteral("input")) {
            const QString text =
                decision.startsWith(QStringLiteral("input:")) ? decision.mid(6) : decision;
            engine->respondInput(QString(), text);
        }
    });

    QTimer::singleShot(timeoutMs, &loop, &QEventLoop::quit);
    engine->start(prompt);
    loop.exec();
    engine->cancel();
    engine->deleteLater();
    return answer;
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

void Application::clearLocalData() const {
    // "Forget this device": wipe every client-local trace of the node, but never touch the node's
    // own server-side data. Ordered so nothing races a live connection.

    // 1. Drop the active connection first (stops the reconnect/backoff loop and any in-flight
    //    writes to the caches we are about to clear).
    if (m_services.connection != nullptr) {
        // Best-effort keychain/settings token clear for the active target (desktop keychain path);
        // the wholesale QSettings clear below also removes the settings-fallback tokens.
        m_services.connection->logout();
        m_services.connection->disconnect();
    }

    // 2. Wipe the shared preference store (conn/*, app/setupComplete, ui/*, conn/tokens/*). Both
    //    QtSettingsStore and UiSettings bind this same ("daemon-app","daemon-app") scope, so one
    //    clear() covers connection prefs, the first-run flag, and every UI preference.
    QSettings settings(QStringLiteral("daemon-app"), QStringLiteral("daemon-app"));
    settings.clear();
    settings.sync();

    // 3. Empty the offline-first SQLite cache (roster / transcripts / fs / fleet / channels) and
    //    delete any other user-namespaced cache files on this device.
    if (m_services.cache != nullptr) {
        m_services.cache->clearAll();
    }

    // 4. Drop the on-disk images cache (QNetworkDiskCache dir under CacheLocation/images).
    const QString cacheBase = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (!cacheBase.isEmpty()) {
        QDir(QDir(cacheBase).filePath(QStringLiteral("images"))).removeRecursively();
    }

#ifdef Q_OS_WASM
    // 5a. Browser: force a final write-back of the emptied IDBFS mount, then hard-reload into a
    //     clean first-run state (settings are gone, so setupComplete reads false and the gate
    //     shows).
    // clang-format off
    EM_ASM({
        if (typeof FS !== 'undefined' && FS.syncfs) {
            FS.syncfs(false, function() { location.reload(); });
        } else {
            location.reload();
        }
    });
    // clang-format on
#else
    // 5b. Desktop/mobile: return to the first-run gate in place (no process restart). FirstRun
    //     .restart() flips setupComplete back off and re-enters the onboarding phase.
    if (m_services.firstRun != nullptr) {
        m_services.firstRun->restart();
    }
#endif
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
