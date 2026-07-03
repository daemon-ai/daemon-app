// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "application.h"
#include "daemon_app_version.h"
#include "i18n/localization.h"

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS) || defined(Q_OS_WASM)
// No QtWidgets on mobile or in the browser: pure Qt Quick on QGuiApplication.
#include <QGuiApplication>
using AppBase = QGuiApplication;
#else
// QtWidgets is needed on desktop for the QSystemTrayIcon-based tray.
#include <QApplication>
using AppBase = QApplication;
#endif

#include <cstdio>
#include <QByteArray>
#include <QDir>
#include <QEventLoop>
#include <QFont>
#include <QFontDatabase>
#include <QImage>
#include <QList>
#include <QQmlApplicationEngine>
#include <QQmlError>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QtQml/qqmlextensionplugin.h>
#include <QVariant>

namespace {

// Loads the bundled fonts into the application database and sets Inter as the
// default UI font, so all text is Inter even without a per-item family and the
// FontAwesome/Material families resolve for the icon glyphs.
//
// The bundled .ttf files are validated by tests/unit (tst_fonts) so we don't
// need a runtime probe here.
void loadBundledFonts() {
    const QString prefix = QStringLiteral(":/qt/qml/DaemonApp/Theme/fonts/");
    const QStringList bundled = {
        QStringLiteral("InterVariable.ttf"),
        QStringLiteral("fa-solid-900.ttf"),
        QStringLiteral("fa-brands-400.ttf"),
        QStringLiteral("material-symbols-outlined.ttf"),
        // Editor text-style families.
        QStringLiteral("Trykker-Regular.ttf"),
        QStringLiteral("IbarraRealNova.ttf"),
        QStringLiteral("IbarraRealNova-Italic.ttf"),
        QStringLiteral("iAWriterMonoS-Regular.ttf"),
        QStringLiteral("iAWriterMonoS-Bold.ttf"),
        QStringLiteral("iAWriterMonoS-Italic.ttf"),
        QStringLiteral("iAWriterMonoS-BoldItalic.ttf"),
        QStringLiteral("iAWriterDuoS-Regular.ttf"),
        QStringLiteral("iAWriterDuoS-Bold.ttf"),
        QStringLiteral("iAWriterDuoS-Italic.ttf"),
        QStringLiteral("iAWriterDuoS-BoldItalic.ttf"),
        QStringLiteral("iAWriterQuattroS-Regular.ttf"),
        QStringLiteral("iAWriterQuattroS-Bold.ttf"),
        QStringLiteral("iAWriterQuattroS-Italic.ttf"),
        QStringLiteral("iAWriterQuattroS-BoldItalic.ttf"),
    };
    for (const QString& file : bundled) {
        QFontDatabase::addApplicationFont(prefix + file);
    }

    QFont base(QStringLiteral("Inter"));
    base.setStyleStrategy(QFont::PreferAntialias);
    QGuiApplication::setFont(base);
}

// Offscreen render harness. When DAEMON_APP_RENDER_SHOTS is set to a directory,
// render the live window once per theme and save a PNG each, then exit. This is
// a guarded, opt-in test mode (no effect on a normal run), kept here because the
// DaemonApp.App/Main module is baked into this executable.
// Run it offscreen (QT_QPA_PLATFORM=offscreen, QT_QUICK_BACKEND=software).
// Returns true if shots were requested (and the app should exit).
bool maybeRunOffscreenRenderHarness(QQmlApplicationEngine& engine) {
    const QByteArray outDir = qgetenv("DAEMON_APP_RENDER_SHOTS");
    if (outDir.isEmpty()) {
        return false;
    }

    QDir().mkpath(QString::fromLocal8Bit(outDir));

    auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().constFirst());
    if (window == nullptr) {
        qWarning("render-shots: root object is not a window");
        return true;
    }

    auto* theme = engine.singletonInstance<QObject*>(QStringLiteral("DaemonApp.Theme"),
                                                     QStringLiteral("Theme"));
    if (theme == nullptr) {
        qWarning("render-shots: could not resolve the Theme singleton");
        return true;
    }

    window->show();

    // Open a session so the Transcript renders real markdown (headings,
    // lists, inline code, a table) instead of the empty-state placeholder. The
    // SessionController lives in the QML object tree; reach it by class name.
    const QList<QObject*> children = window->findChildren<QObject*>();
    for (QObject* obj : children) {
        if (qstrcmp(obj->metaObject()->className(), "SessionController") == 0) {
            QMetaObject::invokeMethod(obj, "open", Q_ARG(int, 1));
            break;
        }
    }
    // Let the open + block projection settle before the first grab.
    {
        QEventLoop loop;
        QTimer::singleShot(200, &loop, &QEventLoop::quit);
        loop.exec();
    }

    // Optionally open the settings menu so its 1:1 fidelity can be verified per
    // theme (guarded; the normal theme shots stay menu-free).
    const bool withMenu = !qgetenv("DAEMON_APP_RENDER_MENU").isEmpty();
    QObject* settingsMenu = nullptr;
    if (withMenu) {
        settingsMenu = window->findChild<QObject*>(QStringLiteral("settingsMenu"));
        if (settingsMenu == nullptr) {
            qWarning("render-shots: could not find the settingsMenu popup");
        }
    }
    const QString prefix = withMenu ? QStringLiteral("menu-") : QStringLiteral("theme-");

    const QStringList themeNames = {QStringLiteral("Light"), QStringLiteral("Dark"),
                                    QStringLiteral("Sepia"), QStringLiteral("Midnight")};

    for (const QString& name : themeNames) {
        const QString file = prefix + name.toLower() + QStringLiteral(".png");
        QMetaObject::invokeMethod(theme, "setTheme", Q_ARG(QVariant, QVariant(name)));
        if (settingsMenu != nullptr) {
            QMetaObject::invokeMethod(settingsMenu, "open");
        }

        // Let the binding updates settle and a frame compose before grabbing.
        QEventLoop loop;
        QTimer::singleShot(150, &loop, &QEventLoop::quit);
        loop.exec();

        const QImage shot = window->grabWindow();
        const QString path = QDir(QString::fromLocal8Bit(outDir)).filePath(file);
        if (shot.isNull() || !shot.save(path)) {
            qWarning("render-shots: failed to save %s", qPrintable(path));
        } else {
            qInfo("render-shots: wrote %s (%dx%d)", qPrintable(path), shot.width(), shot.height());
        }
    }

    return true;
}

} // namespace

// The feature modules are STATIC, so their QML plugins must be referenced
// explicitly or the linker discards them (shared Qt build => qt_import_qml_plugins
// does not apply). Class names are the module URI with dots->underscores + Plugin.
Q_IMPORT_QML_PLUGIN(DaemonApp_PresentationPlugin)
Q_IMPORT_QML_PLUGIN(DaemonApp_ThemeCorePlugin)
Q_IMPORT_QML_PLUGIN(DaemonApp_ComposerSessionPlugin)
Q_IMPORT_QML_PLUGIN(DaemonApp_TurnPlugin)
Q_IMPORT_QML_PLUGIN(DaemonApp_StatusModelPlugin)
Q_IMPORT_QML_PLUGIN(DaemonApp_ThemePlugin)
Q_IMPORT_QML_PLUGIN(DaemonApp_ControlsPlugin)
Q_IMPORT_QML_PLUGIN(DaemonApp_GraphPlugin)
Q_IMPORT_QML_PLUGIN(DaemonApp_BlockEditorPlugin)
Q_IMPORT_QML_PLUGIN(DaemonApp_SidebarPlugin)
Q_IMPORT_QML_PLUGIN(DaemonApp_SessionsListPlugin)
Q_IMPORT_QML_PLUGIN(DaemonApp_SessionPlugin)
Q_IMPORT_QML_PLUGIN(DaemonApp_TabsPlugin)
Q_IMPORT_QML_PLUGIN(DaemonApp_TerminalPlugin)
Q_IMPORT_QML_PLUGIN(DaemonApp_FilesPlugin)
Q_IMPORT_QML_PLUGIN(DaemonApp_ParticipantsPlugin)
Q_IMPORT_QML_PLUGIN(DaemonApp_EditorPlugin)
Q_IMPORT_QML_PLUGIN(DaemonApp_TranscriptPlugin)
Q_IMPORT_QML_PLUGIN(DaemonApp_ComposerPlugin)
Q_IMPORT_QML_PLUGIN(DaemonApp_SettingsPlugin)
Q_IMPORT_QML_PLUGIN(DaemonApp_PagesPlugin)
Q_IMPORT_QML_PLUGIN(DaemonApp_MemoryPlugin)
Q_IMPORT_QML_PLUGIN(DaemonApp_StatusBarPlugin)

int main(int argc, char* argv[]) {
    AppBase app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("daemon-app"));
    QCoreApplication::setOrganizationName(QStringLiteral("daemon-app"));
    QCoreApplication::setApplicationVersion(QStringLiteral(DAEMON_APP_VERSION_STR));

    // Pin the Controls style to Basic: our kit (DaemonApp.Controls) restyles
    // controls via theme tokens, so we want the least-opinionated base style
    // (no Material elevation/shadows). setStyle() has the highest precedence,
    // making us immune to a host exporting QT_QUICK_CONTROLS_STYLE.
    QQuickStyle::setStyle(QStringLiteral("Basic"));

    loadBundledFonts();

    // Install translations for the persisted UI language (default: follow the OS
    // locale) before the scene loads, so the first frame is already localized.
    // The settings picker can switch it live later (Application wires the
    // languageChanged signal to a QQmlEngine retranslate).
    {
        const QString language =
            QSettings(QStringLiteral("daemon-app"), QStringLiteral("daemon-app"))
                .value(QStringLiteral("ui/language"), QStringLiteral("system"))
                .toString();
        AppBase::setLayoutDirection(i18n::applyLocale(language));
    }

    Application application;

    QQmlApplicationEngine engine;

    // KSyntaxHighlighting ships its QML module (org.kde.syntaxhighlighting) as a
    // separate shared plugin, so unlike our STATIC feature modules it must be
    // found on the engine import path. Add both layouts: the installed tree
    // (<bindir>/../lib/qml next to the wrapped binary) and the build tree
    // (QT_QML_OUTPUT_DIRECTORY, passed in as DAEMON_APP_BUILD_QML_DIR) for
    // `nix develop` runs straight out of the build directory.
    engine.addImportPath(QCoreApplication::applicationDirPath() + QStringLiteral("/../lib/qml"));
#ifdef DAEMON_APP_BUILD_QML_DIR
    engine.addImportPath(QStringLiteral(DAEMON_APP_BUILD_QML_DIR));
#endif

    application.registerContext(engine);

    QObject::connect(&engine, &QQmlApplicationEngine::warnings,
                     [](const QList<QQmlError>& warnings) {
                         for (const QQmlError& w : warnings) {
                             qWarning("%s", qPrintable(w.toString()));
                         }
                     });

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        [] { QCoreApplication::exit(-1); }, Qt::QueuedConnection);

    engine.loadFromModule("DaemonApp.App", "Main");
    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    application.completeWiring(engine);

    // Guarded: open an app-level page overlay before rendering shots, so a
    // manager/settings surface can be screenshotted offscreen. No-op normally.
    const QByteArray pageEnv = qgetenv("DAEMON_APP_RENDER_PAGE");
    if (!pageEnv.isEmpty()) {
        const QString spec = QString::fromLocal8Bit(pageEnv);
        const qsizetype slash = spec.indexOf(QLatin1Char('/'));
        if (slash >= 0) {
            application.openPageForRenderHarness(spec.left(slash), spec.mid(slash + 1));
        } else {
            application.openPageForRenderHarness(spec);
        }
    }

    // Headless full-onboarding self-check (CON-4/6/7): connect, add the provider key, pick a model,
    // and finish - so the E2E harness can assert the credential/model wire ops + persisted setup
    // without QML clicks. Gated on DAEMON_APP_ONBOARD_KEY so a normal run is unaffected.
    const QByteArray onboardKey = qgetenv("DAEMON_APP_ONBOARD_KEY");
    if (!onboardKey.isEmpty()) {
        const QByteArray waitMs = qgetenv("DAEMON_APP_WAIT_READY");
        const int timeoutMs = waitMs.toInt() > 0 ? waitMs.toInt() : 15000;
        const QString provider = QString::fromUtf8(qgetenv("DAEMON_APP_ONBOARD_PROVIDER"));
        const bool ready = application.runHeadlessOnboarding(
            provider.isEmpty() ? QStringLiteral("anthropic") : provider,
            QString::fromUtf8(onboardKey), timeoutMs);
        std::fprintf(stdout, "DAEMON_APP_READY %s\n", ready ? "ok" : "timeout");
        std::fflush(stdout);
        return ready ? 0 : 2;
    }

    // Headless profile self-check (PRO-2/3): create (+ optionally edit) a profile via the store so
    // the E2E harness can assert ProfileCreate/Update cross the socket. Gated on
    // DAEMON_APP_PROFILE_CREATE.
    const QByteArray profileCreate = qgetenv("DAEMON_APP_PROFILE_CREATE");
    if (!profileCreate.isEmpty()) {
        const QByteArray waitMs = qgetenv("DAEMON_APP_WAIT_READY");
        const int timeoutMs = waitMs.toInt() > 0 ? waitMs.toInt() : 15000;
        const bool ready = application.runHeadlessProfile(
            QString::fromUtf8(profileCreate),
            QString::fromUtf8(qgetenv("DAEMON_APP_PROFILE_MODEL")),
            QString::fromUtf8(qgetenv("DAEMON_APP_PROFILE_PROMPT")), timeoutMs);
        std::fprintf(stdout, "DAEMON_APP_READY %s\n", ready ? "ok" : "timeout");
        std::fflush(stdout);
        return ready ? 0 : 2;
    }

    // Headless model-track self-check (Phase 2): connect, then exercise the model wire ops
    // (ModelCatalog / ModelDownloads / ModelSearch / ModelFiles / ModelDownload) and print a
    // per-frame round-trip summary so the E2E harness can assert the client wires them. Gated on
    // DAEMON_APP_MODELS_PROBE (the search query); DAEMON_APP_MODELS_REPO names the repo to probe.
    const QByteArray modelsProbe = qgetenv("DAEMON_APP_MODELS_PROBE");
    if (!modelsProbe.isEmpty()) {
        const QByteArray waitMs = qgetenv("DAEMON_APP_WAIT_READY");
        const int timeoutMs = waitMs.toInt() > 0 ? waitMs.toInt() : 30000;
        const QByteArray repoEnv = qgetenv("DAEMON_APP_MODELS_REPO");
        const QString repo = repoEnv.isEmpty()
                                 ? QStringLiteral("bartowski/SmolLM2-135M-Instruct-GGUF")
                                 : QString::fromUtf8(repoEnv);
        const QString summary =
            application.runHeadlessModels(QString::fromUtf8(modelsProbe), repo, timeoutMs);
        std::fprintf(stdout, "DAEMON_APP_MODELS %s\n", summary.toUtf8().constData());
        std::fflush(stdout);
        return summary.isEmpty() ? 2 : 0;
    }

    // Headless chat self-check (CHA-1/CHA-2): connect, drive one real turn, and emit the streamed
    // assistant text so the E2E harness can assert Submit + Subscribe crossed the socket and the
    // client consumed the AgentEvent stream. Gated on DAEMON_APP_CHAT_PROMPT.
    const QByteArray chatPrompt = qgetenv("DAEMON_APP_CHAT_PROMPT");
    if (!chatPrompt.isEmpty()) {
        const QByteArray waitMs = qgetenv("DAEMON_APP_WAIT_READY");
        const int timeoutMs = waitMs.toInt() > 0 ? waitMs.toInt() : 30000;
        const QString answer =
            application.runHeadlessChat(QString::fromUtf8(chatPrompt), timeoutMs,
                                        QString::fromUtf8(qgetenv("DAEMON_APP_CHAT_PROFILE")));
        std::fprintf(stdout, "DAEMON_APP_ANSWER %s\n", answer.toUtf8().constData());
        std::fflush(stdout);
        return answer.isEmpty() ? 2 : 0;
    }

    // Headless HITL self-check (CHA-4/5): connect, drive a turn that parks on a host gate, and
    // auto-resolve it per DAEMON_APP_HITL_DECISION ("approve"|"deny"|"choice"|"input:<text>").
    // Emits the streamed answer so the E2E harness can assert the park -> Respond -> resume loop.
    const QByteArray hitlPrompt = qgetenv("DAEMON_APP_HITL_PROMPT");
    if (!hitlPrompt.isEmpty()) {
        const QByteArray waitMs = qgetenv("DAEMON_APP_WAIT_READY");
        const int timeoutMs = waitMs.toInt() > 0 ? waitMs.toInt() : 30000;
        const QString decision = QString::fromUtf8(qgetenv("DAEMON_APP_HITL_DECISION"));
        const QString answer =
            application.runHeadlessHitl(QString::fromUtf8(hitlPrompt), decision, timeoutMs);
        std::fprintf(stdout, "DAEMON_APP_ANSWER %s\n", answer.toUtf8().constData());
        std::fflush(stdout);
        return answer.isEmpty() ? 2 : 0;
    }

    // Headless slash-command self-check (CHA-7): connect, CommandList (+ optional CommandInvoke),
    // emit the discovered names (or invoked output) so the E2E harness can assert the wire ops.
    // Gated on DAEMON_APP_COMMAND_LIST (set to "1" to just list; DAEMON_APP_COMMAND_INVOKE
    // invokes).
    const QByteArray commandList = qgetenv("DAEMON_APP_COMMAND_LIST");
    if (!commandList.isEmpty()) {
        const QByteArray waitMs = qgetenv("DAEMON_APP_WAIT_READY");
        const int timeoutMs = waitMs.toInt() > 0 ? waitMs.toInt() : 30000;
        QString out = application.runHeadlessCommands(
            QString::fromUtf8(qgetenv("DAEMON_APP_COMMAND_INVOKE")), timeoutMs);
        std::fprintf(stdout, "DAEMON_APP_COMMANDS %s\n",
                     out.replace(QLatin1Char('\n'), QLatin1Char(',')).toUtf8().constData());
        std::fflush(stdout);
        return out.isEmpty() ? 2 : 0;
    }

    // Headless session-search self-check (CHA-8): connect, SessionSearch, emit the hit session ids.
    // Gated on DAEMON_APP_SESSION_SEARCH (the query string).
    const QByteArray searchQuery = qgetenv("DAEMON_APP_SESSION_SEARCH");
    if (!searchQuery.isEmpty()) {
        const QByteArray waitMs = qgetenv("DAEMON_APP_WAIT_READY");
        const int timeoutMs = waitMs.toInt() > 0 ? waitMs.toInt() : 30000;
        QString out = application.runHeadlessSearch(QString::fromUtf8(searchQuery), timeoutMs);
        std::fprintf(stdout, "DAEMON_APP_SEARCH %s\n",
                     out.replace(QLatin1Char('\n'), QLatin1Char(',')).toUtf8().constData());
        std::fflush(stdout);
        return 0;
    }

    // Headless filesystem self-check (Phase 4 fs seam): connect, then drive the daemon-backed
    // IFsService over the wire (listRoots -> open -> write a probe file -> read it back) and emit a
    // summary so the E2E harness can assert FsRoots/FsList/FsWrite/FsRead crossed the socket
    // against the real daemon WorkspaceFs. Gated on DAEMON_APP_FS_PROBE.
    const QByteArray fsProbe = qgetenv("DAEMON_APP_FS_PROBE");
    if (!fsProbe.isEmpty()) {
        const QByteArray waitMs = qgetenv("DAEMON_APP_WAIT_READY");
        const int timeoutMs = waitMs.toInt() > 0 ? waitMs.toInt() : 30000;
        const QString out = application.runHeadlessFs(timeoutMs);
        std::fprintf(stdout, "DAEMON_APP_FS %s\n", out.toUtf8().constData());
        std::fflush(stdout);
        return out.isEmpty() ? 2 : 0;
    }

    // Headless fleet self-check (Phase 5b: PRO-9/10): connect, refresh the subagent Tree, issue a
    // Pause, and emit "units=N" so the E2E harness can assert Tree + Pause crossed the socket.
    // Gated on DAEMON_APP_FLEET_PROBE.
    const QByteArray fleetProbe = qgetenv("DAEMON_APP_FLEET_PROBE");
    if (!fleetProbe.isEmpty()) {
        const QByteArray waitMs = qgetenv("DAEMON_APP_WAIT_READY");
        const int timeoutMs = waitMs.toInt() > 0 ? waitMs.toInt() : 30000;
        const QString out = application.runHeadlessFleet(timeoutMs);
        std::fprintf(stdout, "DAEMON_APP_FLEET %s\n", out.toUtf8().constData());
        std::fflush(stdout);
        return out.isEmpty() ? 2 : 0;
    }

    // Headless channels self-check (Phase 6a: EIO-1/3/8): connect, refresh the transport adapters +
    // instances and enumerate conversations, emitting "adapters=N instances=M" so the E2E harness
    // can assert TransportAdapters + TransportInstances + ConvList crossed the socket. Gated on
    // DAEMON_APP_CHANNELS_PROBE.
    const QByteArray channelsProbe = qgetenv("DAEMON_APP_CHANNELS_PROBE");
    if (!channelsProbe.isEmpty()) {
        const QByteArray waitMs = qgetenv("DAEMON_APP_WAIT_READY");
        const int timeoutMs = waitMs.toInt() > 0 ? waitMs.toInt() : 30000;
        const QString out = application.runHeadlessChannels(timeoutMs);
        std::fprintf(stdout, "DAEMON_APP_CHANNELS %s\n", out.toUtf8().constData());
        std::fflush(stdout);
        return out.isEmpty() ? 2 : 0;
    }

    // Headless connectivity self-check for the cross-repo E2E harness: block until the daemon-mode
    // auto-connect Health round-trip resolves (or times out), emit a stable sentinel on stdout, and
    // exit deterministically. Keeps the harness from racing the async connect.
    const QByteArray waitReadyMs = qgetenv("DAEMON_APP_WAIT_READY");
    if (!waitReadyMs.isEmpty()) {
        const int timeoutMs = waitReadyMs.toInt() > 0 ? waitReadyMs.toInt() : 5000;
        // On first run nothing auto-connects; drive the onboarding "Local" connect (which, with
        // managed local daemon on, spawns the daemon if needed) so the harness can assert it.
        application.driveFirstRunConnect();
        const bool ready = application.awaitConnectionReady(timeoutMs);
        std::fprintf(stdout, "DAEMON_APP_READY %s\n", ready ? "ok" : "timeout");
        std::fflush(stdout);
        return ready ? 0 : 2;
    }

    if (maybeRunOffscreenRenderHarness(engine)) {
        return 0;
    }

    return AppBase::exec();
}
