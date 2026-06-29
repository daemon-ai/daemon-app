// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "core/agent_ingest.h"
#include "core/document_store.h"
#include "core/transcript_search.h"
#include "interactive_turn_host.h"

#include <functional>
#include <QHash>
#include <QObject>
#include <QString>

class SessionController;
class SessionOrchestrator;
class TabModel;
class ITurnEngine;
class ITurnEngineFactory;

// Per-transcript-tab backend state. Each transcript tab owns an independent
// controller / orchestrator (turn) / document / ingest / mock host, so a tab that
// is streaming keeps growing its own document in the background; the single set of
// views always binds to the active session.
struct TabSession {
    int tabId = -1;
    QString sessionId;
    SessionController* controller = nullptr;
    SessionOrchestrator* orchestrator = nullptr; // owns `turn`
    ITurnEngine* turn = nullptr;
    InteractiveTurnHost* host = nullptr;
    be::DocumentStore doc;
    be::TranscriptIngest ingest{&doc};
    be::TranscriptSearchController search;

    TabSession();
    ~TabSession();
};

class TabSessionManager {
public:
    TabSessionManager(QObject* store, TabModel* tabs);
    ~TabSessionManager();

    // The turn-engine factory injected into every session's orchestrator (daemon engine vs mock
    // simulator). Set once at startup; null keeps the orchestrator's default mock simulator.
    void setTurnEngines(ITurnEngineFactory* factory) { m_turnEngines = factory; }

    [[nodiscard]] TabSession* ensureSession(int tabId,
                                            const std::function<void(TabSession*)>& wire);
    void rebindSession(int tabId, const QString& sessionId,
                       const std::function<void(TabSession*)>& wire);
    bool destroySession(int tabId, TabSession*& active,
                        const std::function<void()>& detachActiveDocument);

private:
    QObject* m_store = nullptr;
    TabModel* m_tabs = nullptr;
    ITurnEngineFactory* m_turnEngines = nullptr;
    QHash<int, TabSession*> m_sessions;
};
