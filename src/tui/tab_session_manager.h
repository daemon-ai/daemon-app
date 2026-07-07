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
namespace session {
class ISessionSettings;
}
namespace profiles {
class IProfileStore;
}

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
    // Submitted thumbs ratings for this tab's messages (message id -> 1/-1/0),
    // the TUI's per-tab analog of the GUI EditorController's rating map. Bound to
    // the shared TranscriptView on activation so the footer paints the selected
    // glyph; keyed by message id (unique within this tab's document).
    QHash<QString, int> feedbackRatings;

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

    // The shared per-session override seam + profile store, injected into every session's
    // orchestrator so the focused chat's profile drives the turn (#6b), mirroring the GUI's
    // `sessionSettings`/`profileStore` bindings. Set once at startup; null leaves the turn on the
    // node's active profile.
    void setSessionSettings(session::ISessionSettings* settings) { m_sessionSettings = settings; }
    void setProfileStore(profiles::IProfileStore* store) { m_profileStore = store; }

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
    session::ISessionSettings* m_sessionSettings = nullptr;
    profiles::IProfileStore* m_profileStore = nullptr;
    QHash<int, TabSession*> m_sessions;
};
