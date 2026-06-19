#include "application.h"

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
#include <QGuiApplication>
using AppBase = QGuiApplication;
#else
// QtWidgets is needed on desktop for the QSystemTrayIcon-based tray.
#include <QApplication>
using AppBase = QApplication;
#endif

#include <QFont>
#include <QFontDatabase>
#include <QList>
#include <QQmlApplicationEngine>
#include <QQmlError>
#include <QQuickStyle>
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

    return app.exec();
}
