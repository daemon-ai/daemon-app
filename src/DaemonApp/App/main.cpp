#include <QGuiApplication>
#include <QQmlApplicationEngine>

// Minimal bootstrap for the scaffold: open the App module's Main window.
// The desktop QApplication switch (for Widgets-based tray/updater) arrives with
// the platform layer; an empty Quick window needs only QGuiApplication.
int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("DaemonApp.App", "Main");

    return app.exec();
}
