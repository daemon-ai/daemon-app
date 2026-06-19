#include "application.h"

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
#include <QGuiApplication>
using AppBase = QGuiApplication;
#else
// QtWidgets is needed on desktop for the QSystemTrayIcon-based tray.
#include <QApplication>
using AppBase = QApplication;
#endif

#include <QList>
#include <QQmlApplicationEngine>
#include <QQmlError>
#include <QQuickStyle>
#include <QtQml/qqmlextensionplugin.h>

// The feature modules are STATIC, so their QML plugins must be referenced
// explicitly or the linker discards them (shared Qt build => qt_import_qml_plugins
// does not apply). Class names are the module URI with dots->underscores + Plugin.
Q_IMPORT_QML_PLUGIN(DaemonApp_ThemePlugin)
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

    // Pin the Controls style so the app looks the same everywhere and is immune
    // to a host session exporting QT_QUICK_CONTROLS_STYLE (e.g. org.kde.desktop):
    // QQuickStyle::setStyle() has the highest precedence of all style selectors.
    QQuickStyle::setStyle(QStringLiteral("Fusion"));

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
