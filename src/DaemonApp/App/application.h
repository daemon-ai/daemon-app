#pragma once

#include <QObject>

QT_BEGIN_NAMESPACE
class QEvent;
class QQmlApplicationEngine;
class QQuickWindow;
QT_END_NAMESPACE

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

protected:
    // Close-to-tray: when a tray is installed, intercept the root window's close
    // (X) event and hide instead of quitting.
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    persistence::InMemoryConversationStore* m_store = nullptr;
    platform::IPlatformServices* m_platform = nullptr;
    QQuickWindow* m_window = nullptr;
};
