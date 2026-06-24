#include "application.h"

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
#include <QGuiApplication>
using AppBase = QGuiApplication;
#else
// QtWidgets is needed on desktop for the QSystemTrayIcon-based tray.
#include <QApplication>
using AppBase = QApplication;
#endif

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
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVariant>
#include <QtQml/qqmlextensionplugin.h>

namespace {

// Loads the bundled fonts into the application database and sets Inter as the
// default UI font, so all text is Inter even without a per-item family and the
// FontAwesome/Material families resolve for the icon glyphs.
//
// The bundled .ttf files are validated by tests/unit (tst_fonts) so we don't
// need a runtime probe here.
void loadBundledFonts()
{
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

// Visual proof of theming. When DAEMON_APP_RENDER_SHOTS is set to a directory,
// render the live window once per theme (Light/Dark/Sepia) and save a PNG each,
// then exit. This is a guarded, opt-in render mode (no effect on a normal run),
// kept here because the DaemonApp.App/Main module is baked into this executable.
// Run it offscreen (QT_QPA_PLATFORM=offscreen, QT_QUICK_BACKEND=software).
// Returns true if shots were requested (and the app should exit).
bool maybeRenderThemeShots(QQmlApplicationEngine& engine)
{
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

    QObject* theme = engine.singletonInstance<QObject*>(QStringLiteral("DaemonApp.Theme"),
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

    const QStringList themeNames = { QStringLiteral("Light"), QStringLiteral("Dark"),
                                     QStringLiteral("Sepia"), QStringLiteral("Midnight") };

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
            qInfo("render-shots: wrote %s (%dx%d)", qPrintable(path), shot.width(),
                  shot.height());
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
Q_IMPORT_QML_PLUGIN(DaemonApp_BlockEditorPlugin)
Q_IMPORT_QML_PLUGIN(DaemonApp_SidebarPlugin)
Q_IMPORT_QML_PLUGIN(DaemonApp_SessionsListPlugin)
Q_IMPORT_QML_PLUGIN(DaemonApp_SessionPlugin)
Q_IMPORT_QML_PLUGIN(DaemonApp_TabsPlugin)
Q_IMPORT_QML_PLUGIN(DaemonApp_FilesPlugin)
Q_IMPORT_QML_PLUGIN(DaemonApp_EditorPlugin)
Q_IMPORT_QML_PLUGIN(DaemonApp_TranscriptPlugin)
Q_IMPORT_QML_PLUGIN(DaemonApp_ComposerPlugin)
Q_IMPORT_QML_PLUGIN(DaemonApp_SettingsPlugin)
Q_IMPORT_QML_PLUGIN(DaemonApp_PagesPlugin)
Q_IMPORT_QML_PLUGIN(DaemonApp_MemoryPlugin)
Q_IMPORT_QML_PLUGIN(DaemonApp_StatusBarPlugin)

int main(int argc, char* argv[])
{
    AppBase app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("daemon-app"));
    QCoreApplication::setOrganizationName(QStringLiteral("daemon-app"));

    // Pin the Controls style to Basic: our kit (DaemonApp.Controls) restyles
    // controls via theme tokens, so we want the least-opinionated base style
    // (no Material elevation/shadows). setStyle() has the highest precedence,
    // making us immune to a host exporting QT_QUICK_CONTROLS_STYLE.
    QQuickStyle::setStyle(QStringLiteral("Basic"));

    loadBundledFonts();

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
            application.openPageForShots(spec.left(slash), spec.mid(slash + 1));
        } else {
            application.openPageForShots(spec);
        }
    }

    if (maybeRenderThemeShots(engine)) {
        return 0;
    }

    return app.exec();
}
