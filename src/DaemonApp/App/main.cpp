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
    QFontDatabase::addApplicationFont(prefix + QStringLiteral("InterVariable.ttf"));
    QFontDatabase::addApplicationFont(prefix + QStringLiteral("fa-solid-900.ttf"));
    QFontDatabase::addApplicationFont(prefix + QStringLiteral("fa-brands-400.ttf"));
    QFontDatabase::addApplicationFont(prefix + QStringLiteral("material-symbols-outlined.ttf"));

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

    const QList<QPair<QString, QString>> themes = {
        { QStringLiteral("Light"), QStringLiteral("theme-light.png") },
        { QStringLiteral("Dark"), QStringLiteral("theme-dark.png") },
        { QStringLiteral("Sepia"), QStringLiteral("theme-sepia.png") },
    };

    for (const auto& [name, file] : themes) {
        QMetaObject::invokeMethod(theme, "setTheme", Q_ARG(QVariant, QVariant(name)));

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
Q_IMPORT_QML_PLUGIN(DaemonApp_ThemePlugin)
Q_IMPORT_QML_PLUGIN(DaemonApp_ControlsPlugin)
Q_IMPORT_QML_PLUGIN(DaemonApp_SidebarPlugin)
Q_IMPORT_QML_PLUGIN(DaemonApp_ConversationsListPlugin)
Q_IMPORT_QML_PLUGIN(DaemonApp_ConversationPlugin)
Q_IMPORT_QML_PLUGIN(DaemonApp_TranscriptPlugin)
Q_IMPORT_QML_PLUGIN(DaemonApp_ComposerPlugin)

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

    if (maybeRenderThemeShots(engine)) {
        return 0;
    }

    return app.exec();
}
