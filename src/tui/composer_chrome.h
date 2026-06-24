#pragma once

#include "transcript_render.h" // Span

#include <Tui/ZWidget.h>

#include <QTimer>
#include <QVector>

class TurnController;
class ComposerSessionController;

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

    void setTurn(TurnController* turn);
    void setSession(ComposerSessionController* session);

    [[nodiscard]] QSize sizeHint() const override;

protected:
    void paintEvent(Tui::ZPaintEvent* event) override;

private:
    [[nodiscard]] QVector<Span> buildSpans() const;
    void syncSpinner();

    TurnController* m_turn = nullptr;
    ComposerSessionController* m_session = nullptr;
    QTimer m_spinner;
    int m_spinnerTick = 0;
};
