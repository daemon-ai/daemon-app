#include "mouse_terminal.h"

#include <cstdlib>
#include <cstring>
#include <termpaint_event.h>
#include <Tui/ZEvent.h>
#include <unistd.h>

namespace {

// Tracks whether the enable sequence is currently written, so enable/disable are
// idempotent (the destructor + atexit hook may both fire).
bool g_mouseReportingOn = false;

Qt::KeyboardModifiers translateModifiers(int mod) {
    Qt::KeyboardModifiers mods = Qt::NoModifier;
    if ((mod & TERMPAINT_MOD_SHIFT) != 0) {
        mods |= Qt::ShiftModifier;
    }
    if ((mod & TERMPAINT_MOD_CTRL) != 0) {
        mods |= Qt::ControlModifier;
    }
    if ((mod & TERMPAINT_MOD_ALT) != 0) {
        mods |= Qt::AltModifier;
    }
    return mods;
}

// Write the whole buffer to stdout, tolerating short writes. Used only for the
// tiny mouse-mode control sequences, kept separate from ZTerminal's own output.
void writeRaw(const char* seq) {
    const std::size_t len = std::strlen(seq);
    std::size_t off = 0;
    while (off < len) {
        const ssize_t n = ::write(STDOUT_FILENO, seq + off, len - off);
        if (n <= 0) {
            break;
        }
        off += static_cast<std::size_t>(n);
    }
}

} // namespace

void MouseTerminal::enableMouseReporting() {
    if (g_mouseReportingOn) {
        return;
    }
    // 1000h: report button press/release. 1006h: SGR extended coordinates, which
    // allows positions past column/row 223 and reports press vs release
    // unambiguously. We intentionally do NOT enable motion tracking (1002/1003):
    // this layer is click + wheel only, no hover/drag.
    writeRaw("\x1b[?1000h\x1b[?1006h");
    g_mouseReportingOn = true;
    // Belt-and-suspenders: ensure the host terminal is restored even if teardown
    // skips the destructor (the guard above makes a double-call a no-op).
    static bool atexitRegistered = false;
    if (!atexitRegistered) {
        std::atexit(&MouseTerminal::disableMouseReporting);
        atexitRegistered = true;
    }
}

void MouseTerminal::disableMouseReporting() {
    if (!g_mouseReportingOn) {
        return;
    }
    writeRaw("\x1b[?1006l\x1b[?1000l");
    g_mouseReportingOn = false;
}

MouseTerminal::~MouseTerminal() {
    disableMouseReporting();
}

bool MouseTerminal::event(QEvent* event) {
    if (event->type() == Tui::ZEventType::terminalNativeEvent()) {
        auto* native = static_cast<Tui::ZTerminalNativeEvent*>(event);
        auto* tpe = static_cast<termpaint_event*>(native->nativeEventPointer());
        if (tpe != nullptr && tpe->type == TERMPAINT_EV_MOUSE) {
            const QPoint pos(tpe->mouse.x, tpe->mouse.y);
            const Qt::KeyboardModifiers mods = translateModifiers(tpe->mouse.modifier);
            const int button = tpe->mouse.button;

            MouseAction action = MouseAction::Press;
            if (tpe->mouse.action == TERMPAINT_MOUSE_MOVE) {
                action = MouseAction::Move;
            } else if (button == 4) {
                action = MouseAction::WheelUp;
            } else if (button == 5) {
                action = MouseAction::WheelDown;
            } else if (tpe->mouse.action == TERMPAINT_MOUSE_RELEASE) {
                action = MouseAction::Release;
            }

            emit mouseInput(pos, action, button, mods);
            // Consume it: the base class would only drop it anyway.
            return true;
        }
    }
    return Tui::ZTerminal::event(event);
}
