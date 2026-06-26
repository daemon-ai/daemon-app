#pragma once

#include <QPoint>
#include <QtCore/qnamespace.h>
#include <Tui/ZTerminal.h>

// Mouse support layer for the TUI.
//
// Tui Widgets 0.2.3 has no mouse API at all (no ZMouseEvent, no ZWidget mouse
// handler, no terminal mouse toggle). The termpaint layer beneath it, however,
// parses mouse escape sequences into TERMPAINT_EV_MOUSE native events; ZTerminal
// wraps every native termpaint event as a ZTerminalNativeEvent routed through
// ZTerminal::event(), where the base implementation only handles key/paste/
// repaint and silently drops mouse. We subclass it to intercept those mouse
// events and re-emit them as a Qt signal the shell can hit-test.
//
// Mouse reporting must also be turned on at the terminal: ZTerminal exposes no
// toggle and hides the underlying termpaint_terminal*, so we write the DECSET
// enable/disable sequences to stdout ourselves and restore them on teardown.
class MouseTerminal : public Tui::ZTerminal {
    Q_OBJECT

public:
    // Press/Release/Move are the literal termpaint actions; WheelUp/WheelDown are
    // synthesized from the wheel "buttons" (4/5) so callers never see termpaint.
    enum class MouseAction { Press, Release, Move, WheelUp, WheelDown };
    Q_ENUM(MouseAction)

    using Tui::ZTerminal::ZTerminal;
    ~MouseTerminal() override;

    // Write the DECSET enable / disable sequences for SGR mouse reporting (button
    // events via mode 1000 + SGR extended coordinates via mode 1006) straight to
    // stdout. Both are idempotent; disableMouseReporting() also runs from the
    // destructor and an atexit hook so a normal exit leaves the host terminal
    // clean even though ZTerminal's own restore logic knows nothing about it.
    static void enableMouseReporting();
    static void disableMouseReporting();

signals:
    // A decoded mouse event in absolute terminal coordinates (0-based, top-left).
    // `button` is 0/1/2 for left/middle/right (meaningless for wheel/move).
    void mouseInput(QPoint pos, MouseTerminal::MouseAction action, int button,
                    Qt::KeyboardModifiers modifiers);

protected:
    bool event(QEvent* event) override;
};
