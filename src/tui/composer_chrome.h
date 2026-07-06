// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "transcript_render.h" // Span

#include <QTimer>
#include <QVector>
#include <Tui/ZWidget.h>

class ITurnEngine;
class ComposerSessionController;
namespace profiles {
class IProfileStore;
}
namespace session {
class ISessionSettings;
}

// A one-line painted indicator above the composer. It merges the GUI's
// SessionChrome streaming pill with a send/stop/steer affordance hint, driven
// entirely by the shared TurnController:
//   - error    -> "\u26a0 <errorText>" (red)
//   - stalled  -> "\u25d0 Still thinking\u2026 Ns" (peach)
//   - active   -> "\u25d0 Thinking\u2026 Ns" (accent)
//   - idle     -> dim "\u25b2 Enter send  \u00b7  \u2726 Ctrl+Enter steer"
// While a turn is in flight it also shows "\u25a0 Esc stop". When a session is set,
// the idle hint also shows the shared current-model name (the GUI's model pill).
class ComposerChrome : public Tui::ZWidget {
    Q_OBJECT

public:
    explicit ComposerChrome(Tui::ZWidget* parent = nullptr);

    void setTurn(ITurnEngine* turn);
    void setSession(ComposerSessionController* session);
    // The engine-identity + approval-policy chips (C3/E1): the idle line appends the session's
    // engine ("Native" / "<agent> (ACP)") and its approval mode ("Ask"/"Edits"/"Auto"/"Deny",
    // reflecting the client's last-set value — v28 wires no per-session mode getter).
    void setFacades(session::ISessionSettings* settings, profiles::IProfileStore* profileStore);

    [[nodiscard]] QSize sizeHint() const override;

protected:
    void paintEvent(Tui::ZPaintEvent* event) override;

private:
    [[nodiscard]] QVector<Span> buildSpans() const;
    void syncSpinner();

    ITurnEngine* m_turn = nullptr;
    ComposerSessionController* m_session = nullptr;
    session::ISessionSettings* m_settings = nullptr;
    profiles::IProfileStore* m_profiles = nullptr;
    QTimer m_spinner;
    int m_spinnerTick = 0;
};
