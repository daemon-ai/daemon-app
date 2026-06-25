#include "tab_session_manager.h"

#include "session_controller.h"
#include "session_orchestrator.h"
#include "tab_model.h"
#include "turn_controller.h"

#include <QtAlgorithms>

#include <utility>

TabSession::TabSession()
{
    search.setDocument(&doc);
}

TabSession::~TabSession()
{
    delete host;
    delete orchestrator; // deletes its child TurnController
    delete controller;
}

TabSessionManager::TabSessionManager(QObject* store, TabModel* tabs)
    : m_store(store)
    , m_tabs(tabs)
{
}

TabSessionManager::~TabSessionManager()
{
    qDeleteAll(m_sessions);
}

TabSession* TabSessionManager::ensureSession(int tabId, std::function<void(TabSession*)> wire)
{
    if (auto it = m_sessions.find(tabId); it != m_sessions.end()) {
        return it.value();
    }
    if (m_tabs == nullptr) {
        return nullptr;
    }
    const int row = m_tabs->indexOfTabId(tabId);
    if (row < 0 || m_tabs->kindAt(row) != TabModel::Transcript) {
        return nullptr;
    }

    auto* s = new TabSession();
    s->tabId = tabId;
    s->sessionId = m_tabs->sessionIdAt(row);
    s->controller = new SessionController();
    s->controller->setStore(m_store);
    s->orchestrator = new SessionOrchestrator();
    s->orchestrator->setSession(s->controller);
    s->turn = s->orchestrator->turn();
    s->host = new InteractiveTurnHost(&s->doc, &s->ingest);
    m_sessions.insert(tabId, s);
    if (wire) {
        wire(s);
    }
    s->controller->open(s->sessionId);
    return s;
}

void TabSessionManager::rebindSession(int tabId, const QString& sessionId,
                                      std::function<void(TabSession*)> wire)
{
    Q_UNUSED(wire)
    auto it = m_sessions.find(tabId);
    if (it == m_sessions.end()) {
        return;
    }
    TabSession* s = it.value();
    if (s->sessionId == sessionId) {
        return;
    }
    s->sessionId = sessionId;
    s->controller->open(sessionId);
}

bool TabSessionManager::destroySession(int tabId, TabSession*& active,
                                       std::function<void()> detachActiveDocument)
{
    auto it = m_sessions.find(tabId);
    if (it == m_sessions.end()) {
        return false;
    }
    TabSession* s = it.value();
    if (active == s) {
        active = nullptr;
        if (detachActiveDocument) {
            detachActiveDocument();
        }
    }
    m_sessions.erase(it);
    delete s;
    return true;
}
