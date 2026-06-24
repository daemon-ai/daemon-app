#pragma once

#include "interactive_turn_host.h"

#include "core/agent_ingest.h"
#include "core/document_store.h"
#include "core/transcript_search.h"

#include <QHash>
#include <QObject>

#include <functional>

class SessionController;
class SessionOrchestrator;
class TabModel;
class TurnController;

// Per-transcript-tab backend state. Each transcript tab owns an independent
// controller / orchestrator (turn) / document / ingest / mock host, so a tab that
// is streaming keeps growing its own document in the background; the single set of
// views always binds to the active session.
struct TabSession {
    int tabId = -1;
    int sessionId = -1;
    SessionController* controller = nullptr;
    SessionOrchestrator* orchestrator = nullptr; // owns `turn`
    TurnController* turn = nullptr;
    InteractiveTurnHost* host = nullptr;
    be::DocumentStore doc;
    be::TranscriptIngest ingest { &doc };
    be::TranscriptSearchController search;

    TabSession();
    ~TabSession();
};

class TabSessionManager {
public:
    TabSessionManager(QObject* store, TabModel* tabs);
    ~TabSessionManager();

    [[nodiscard]] TabSession* ensureSession(int tabId, std::function<void(TabSession*)> wire);
    void rebindSession(int tabId, int sessionId, std::function<void(TabSession*)> wire);
    bool destroySession(int tabId, TabSession*& active, std::function<void()> detachActiveDocument);

private:
    QObject* m_store = nullptr;
    TabModel* m_tabs = nullptr;
    QHash<int, TabSession*> m_sessions;
};
