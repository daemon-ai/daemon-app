#include "application.h"

#include "persistence/in_memory_chat_store.h"
#include "platform/iplatform_services.h"
#include "platform/platform_services_factory.h"

#include <QCoreApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>

Application::Application(QObject* parent)
    : QObject(parent)
    , m_store(new persistence::InMemoryChatStore(this))
    , m_platform(platform::createPlatformServices(this))
{
}

void Application::registerContext(QQmlApplicationEngine& engine)
{
    // Shared store; QML view models bind their `store` property to this.
    engine.rootContext()->setContextProperty(QStringLiteral("ChatStore"), m_store);
}

void Application::completeWiring(QQmlApplicationEngine& engine)
{
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

    m_platform->installTray(QCoreApplication::applicationName());
}
