// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "daemon_turn_engine.h"
#include "i_turn_engine.h"
#include "turn_controller.h"

#include <QObject>
#include <QtQml/qqmlregistration.h>

namespace daemonapp::daemon {
class NodeApiClient;
class DaemonCacheStore;
class SubscriptionManager;
} // namespace daemonapp::daemon

// Builds the per-session turn engine. Injected into SessionOrchestrator (QML assigns the
// `TurnEngines` context property; the TUI passes it explicitly), so the front ends pick the mock
// simulator or the daemon engine purely by which factory the app graph wired - mirroring the
// ServiceMode gate used for the other seams.
class ITurnEngineFactory : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("ITurnEngineFactory is provided by the app graph")

public:
    using QObject::QObject;
    ~ITurnEngineFactory() override = default;

    [[nodiscard]] virtual ITurnEngine* create(QObject* parent) = 0;
};

// Mock mode / pre-backend UI: the canned TurnController simulator.
class MockTurnEngineFactory : public ITurnEngineFactory {
    Q_OBJECT

public:
    using ITurnEngineFactory::ITurnEngineFactory;
    [[nodiscard]] ITurnEngine* create(QObject* parent) override {
        return new TurnController(parent);
    }
};

// Daemon mode: the real Submit + Subscribe engine over the shared NodeApiClient.
class DaemonTurnEngineFactory : public ITurnEngineFactory {
    Q_OBJECT

public:
    explicit DaemonTurnEngineFactory(
        daemonapp::daemon::NodeApiClient* client,
        daemonapp::daemon::DaemonCacheStore* cache = nullptr,
        daemonapp::daemon::SubscriptionManager* subscriptions = nullptr, QObject* parent = nullptr)
        : ITurnEngineFactory(parent), m_client(client), m_cache(cache),
          m_subscriptions(subscriptions) {}
    [[nodiscard]] ITurnEngine* create(QObject* parent) override {
        return new DaemonTurnEngine(m_client, m_cache, m_subscriptions, parent);
    }

private:
    daemonapp::daemon::NodeApiClient* m_client = nullptr;
    daemonapp::daemon::DaemonCacheStore* m_cache = nullptr;
    daemonapp::daemon::SubscriptionManager* m_subscriptions = nullptr;
};
