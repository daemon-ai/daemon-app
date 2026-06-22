#pragma once

#include <QObject>

QT_BEGIN_NAMESPACE
class QEvent;
class QQmlApplicationEngine;
class QQuickWindow;
QT_END_NAMESPACE

class StatusBarModel;
class CommandRegistry;
class TranscriptExporter;

namespace persistence {
class IConversationStore;
class InMemoryConversationStore;
}
namespace platform {
class IPlatformServices;
}

// Owns the application-wide services (conversation store, platform integrations) and
// wires them to the QML scene. Kept UI-toolkit agnostic: the only desktop bit
// (tray) lives behind IPlatformServices.
class Application : public QObject {
    Q_OBJECT

public:
    explicit Application(QObject* parent = nullptr);
    ~Application() override;

    // Expose C++ services to QML before the scene loads.
    void registerContext(QQmlApplicationEngine& engine);

    // Connect platform services to the loaded root window and install the tray.
    void completeWiring(QQmlApplicationEngine& engine);

    // Raise a native OS notification for a turn that hit an approval/clarify/host
    // gate, but only while the window is hidden or not active (an on-screen window
    // already shows the inline gate). Returns true if a notification was shown.
    // Bound in QML to the active turn's awaitingInput signal.
    Q_INVOKABLE bool notifyGate(const QString& title, const QString& body);

protected:
    // Close-to-tray: when a tray is installed, intercept the root window's close
    // (X) event and hide instead of quitting.
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    persistence::InMemoryConversationStore* m_store = nullptr;
    platform::IPlatformServices* m_platform = nullptr;
    // The shared footer status model, exposed to QML as the `Status` context
    // property so the footer StatusBar and the active conversation's turn feed a
    // single instance (the TUI builds its own equivalent in RootWidget).
    StatusBarModel* m_status = nullptr;
    // Shared command-palette catalog, exposed to QML as `Commands`.
    CommandRegistry* m_commands = nullptr;
    // Transcript exporter, exposed to QML as `Exporter`.
    TranscriptExporter* m_exporter = nullptr;
    QQuickWindow* m_window = nullptr;
};
