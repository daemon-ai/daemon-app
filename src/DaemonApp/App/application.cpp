// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

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
#include "daemon/node_api_client.h"
#include "daemon/node_api_codec.h"
#include "daemon/principal_model.h"
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
#include "models/iprovider_catalog.h"
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
#include <cstdio>
#include <latex.h>
#include <QCoreApplication>
#include <QDateTime>
#include <QEvent>
#include <QEventLoop>
#include <QFile>
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
        m_turnEngines = new DaemonTurnEngineFactory(m_services.nodeApi, m_services.cache,
                                                    m_services.subscriptions, this);
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

    // ACP agent catalog (foreign engines; wire v23): backs the new-agent dialog's engine picker.
    // Null in mock mode — QML guards with `typeof AcpAgents !== "undefined" && AcpAgents`.
    engine.rootContext()->setContextProperty(QStringLiteral("AcpAgents"), m_services.acp);

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

    auto* engine =
        new DaemonTurnEngine(m_services.nodeApi, m_services.cache, m_services.subscriptions, this);
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
    if (repo == nullptr) {
        return {};
    }
    int units = -1;
    {
        QEventLoop loop;
        const auto c = connect(repo, &FleetRepository::treeRefreshed, this, [&] {
            units = static_cast<int>(repo->cachedUnits().size());
            loop.quit();
        });
        QTimer::singleShot(qMax(2000, timeoutMs / 2), &loop, &QEventLoop::quit);
        repo->refreshTree();
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
        new DaemonTurnEngine(m_services.nodeApi, m_services.cache, m_services.subscriptions, this);
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
