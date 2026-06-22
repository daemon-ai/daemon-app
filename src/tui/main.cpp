#include "mouse_terminal.h"
#include "root_widget.h"
#include "tui_palette.h"

#include "theme/theme_palette.h"

#include <Tui/ZCommon.h>
#include <Tui/ZImage.h>
#include <Tui/ZTerminal.h>
#include <Tui/ZTest.h>

#include <QCoreApplication>
#include <QSettings>
#include <QString>
#include <QTextStream>
#include <QTimer>

#include <cstdio>
#include <functional>
#include <memory>

namespace {

// Apply the persisted (GUI-shared) theme before any widget is built so the
// startup palette is correct. DAEMON_TUI_THEME overrides it for offscreen tests.
// The TUI defaults to Dark (its historical look) when nothing is persisted.
void applyStartupTheme()
{
    QString name = QString::fromUtf8(qgetenv("DAEMON_TUI_THEME"));
    if (name.isEmpty()) {
        const QSettings settings(QStringLiteral("daemon-app"), QStringLiteral("daemon-app"));
        name = settings.value(QStringLiteral("ui/theme"), QStringLiteral("Dark")).toString();
    }
    if (theme::ThemePalette::isKnown(name)) {
        tpal::setActiveTheme(theme::ThemePalette::fromString(name));
    }
}

} // namespace

namespace {

// Headless self-check: render one frame into an off-screen terminal of the given
// "WIDTHxHEIGHT" and dump it as plain text. Lets the spike be validated in CI / a
// sandbox with no real TTY (set DAEMON_TUI_OFFSCREEN=120x36). Returns true if the
// offscreen path ran (and the app should exit).
bool maybeRenderOffscreen()
{
    const QByteArray spec = qgetenv("DAEMON_TUI_OFFSCREEN");
    if (spec.isEmpty()) {
        return false;
    }

    const QList<QByteArray> parts = spec.split('x');
    const int width = parts.value(0).toInt() > 0 ? parts.value(0).toInt() : 120;
    const int height = parts.value(1).toInt() > 0 ? parts.value(1).toInt() : 36;

    Tui::ZTerminal terminal{Tui::ZTerminal::OffScreen{width, height}};
    RootWidget root;
    terminal.setMainWidget(&root);

    // Let the event loop run the deferred layout/paint, then optionally drive the
    // focused widget before grabbing the frame: DAEMON_TUI_KEYS sends a sequence
    // of named keys to the default-focused sidebar (e.g. "down,down,left" to
    // exercise tree navigation + collapse), and DAEMON_TUI_TYPE types into the
    // composer.
    const QByteArray keys = qgetenv("DAEMON_TUI_KEYS");
    const QByteArray typed = qgetenv("DAEMON_TUI_TYPE");
    const int settleMs = qMax(50, qgetenv("DAEMON_TUI_TURN_MS").toInt());

    // Dispatch one named key to the focused widget.
    const auto sendOne = [&terminal, &root](const QByteArray& k) {
        const QByteArray name = k.trimmed().toLower();
        {
                if (name == "down") {
                    Tui::ZTest::sendKey(&terminal, Qt::Key_Down, Qt::NoModifier);
                } else if (name == "up") {
                    Tui::ZTest::sendKey(&terminal, Qt::Key_Up, Qt::NoModifier);
                } else if (name == "left") {
                    Tui::ZTest::sendKey(&terminal, Qt::Key_Left, Qt::NoModifier);
                } else if (name == "right") {
                    Tui::ZTest::sendKey(&terminal, Qt::Key_Right, Qt::NoModifier);
                } else if (name == "tab") {
                    Tui::ZTest::sendKey(&terminal, Qt::Key_Tab, Qt::NoModifier);
                } else if (name == "enter") {
                    Tui::ZTest::sendKey(&terminal, Qt::Key_Enter, Qt::NoModifier);
                } else if (name == "esc") {
                    Tui::ZTest::sendKey(&terminal, Qt::Key_Escape, Qt::NoModifier);
                } else if (name == "ctrl-q") {
                    // forShortcut() matches char events, so emulate via sendText
                    // (sendKey with a letter+Control does not trigger it).
                    Tui::ZTest::sendText(&terminal, QStringLiteral("q"), Qt::ControlModifier);
                } else if (name == "ctrl-c") {
                    Tui::ZTest::sendText(&terminal, QStringLiteral("c"), Qt::ControlModifier);
                } else if (name == "ctrl-enter") {
                    Tui::ZTest::sendKey(&terminal, Qt::Key_Enter, Qt::ControlModifier);
                } else if (name == "ctrl-o") {
                    Tui::ZTest::sendKey(&terminal, Qt::Key_O, Qt::ControlModifier);
                } else if (name == "ctrl-t") {
                    // New transcript tab (text-matched in RootWidget::keyEvent, so
                    // emulate the char event like ctrl-q rather than sendKey).
                    Tui::ZTest::sendText(&terminal, QStringLiteral("t"), Qt::ControlModifier);
                } else if (name == "ctrl-w") {
                    // Close the current tab.
                    Tui::ZTest::sendText(&terminal, QStringLiteral("w"), Qt::ControlModifier);
                } else if (name == "ctrl-tab") {
                    // Switch to the next tab (wraps).
                    Tui::ZTest::sendKey(&terminal, Qt::Key_Tab, Qt::ControlModifier);
                } else if (name == "ctrl-shift-tab") {
                    // Switch to the previous tab (wraps).
                    Tui::ZTest::sendKey(&terminal, Qt::Key_Tab,
                                        Qt::ControlModifier | Qt::ShiftModifier);
                } else if (name == "del") {
                    Tui::ZTest::sendKey(&terminal, Qt::Key_Delete, Qt::NoModifier);
                } else if (name == "space") {
                    Tui::ZTest::sendKey(&terminal, Qt::Key_Space, Qt::NoModifier);
                } else if (name == "cycle-theme") {
                    Tui::ZTest::sendKey(&terminal, Qt::Key_F8, Qt::NoModifier);
                } else if (name == "focus-composer") {
                    root.focusComposer();
                } else if (name == "focus-transcript") {
                    root.focusTranscript();
                } else if (name.startsWith("t:")) {
                    // Type a literal into whatever holds focus (e.g. the composer),
                    // without the auto-Enter that DAEMON_TUI_TYPE appends. Used to
                    // exercise the context-sensitive Esc levels.
                    Tui::ZTest::sendText(&terminal, QString::fromUtf8(k.trimmed().mid(2)),
                                         Qt::NoModifier);
                }
        }
    };

    const QList<QByteArray> seq = keys.isEmpty() ? QList<QByteArray>{} : keys.split(',');
    // Process the key sequence with cumulative timing: a `sleep:N` pseudo-key
    // defers the rest of the sequence by N ms so a frame can be driven *after* a
    // timed event (e.g. a live turn reaching its approval gate). When the sequence
    // is exhausted, run DAEMON_TUI_TYPE then settle + grab.
    auto step = std::make_shared<std::function<void(int)>>();
    *step = [&terminal, &root, &typed, settleMs, seq, sendOne, step](int i) {
        if (i >= seq.size()) {
            if (!typed.isEmpty()) {
                root.focusComposer();
                Tui::ZTest::sendText(&terminal, QString::fromUtf8(typed), Qt::NoModifier);
                Tui::ZTest::sendKey(&terminal, Qt::Key_Enter, Qt::NoModifier);
            }
            // Let any deferred show/layout/paint settle before grabbing the frame;
            // DAEMON_TUI_TURN_MS extends this for a streaming turn.
            QTimer::singleShot(settleMs, [] { QCoreApplication::quit(); });
            return;
        }
        const QByteArray name = seq.at(i).trimmed().toLower();
        if (name.startsWith("sleep:")) {
            const int ms = qMax(0, name.mid(6).toInt());
            QTimer::singleShot(ms, [step, i] { (*step)(i + 1); });
            return;
        }
        sendOne(seq.at(i));
        (*step)(i + 1);
    };
    QTimer::singleShot(0, [step] { (*step)(0); });
    QCoreApplication::exec();

    if (!qgetenv("DAEMON_TUI_DEBUG").isEmpty()) {
        root.dumpGeometry();
    }

    const Tui::ZImage img = terminal.grabCurrentImage();
    QTextStream out(stdout);
    for (int y = 0; y < img.height(); ++y) {
        QString line;
        for (int x = 0; x < img.width(); ++x) {
            int left = 0;
            int right = 0;
            const QString cell = img.peekText(x, y, &left, &right);
            // peekText reports the whole cluster a cell belongs to (blank runs are
            // one cluster). Only emit at the cluster start; render every other
            // column as a space. Correct for the narrow glyphs the UI uses.
            if (left == x) {
                line += cell.isEmpty() ? QStringLiteral(" ") : cell;
            } else {
                line += QLatin1Char(' ');
            }
        }
        out << line << '\n';
    }
    return true;
}

} // namespace

// daemon-tui: a feasibility spike of the GUI ported to a terminal UI on Tui
// Widgets. It runs on a plain QCoreApplication event loop (no QtGui/QtQuick) and
// reuses the app's C++ view models verbatim - see src/tui/CMakeLists.txt.
int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("daemon-tui"));
    QCoreApplication::setOrganizationName(QStringLiteral("daemon-app"));

    // Honor the theme the GUI persisted (shared QSettings file) before any widget
    // builds its palette. The TUI's app name differs (daemon-tui), so the lookup
    // inside applyStartupTheme() names the GUI's daemon-app file explicitly.
    applyStartupTheme();

    if (maybeRenderOffscreen()) {
        return 0;
    }

    MouseTerminal terminal;
    RootWidget root;
    terminal.setMainWidget(&root);

    // A thin bar caret tinted to the theme accent marks the focused text input
    // (the composer); blink follows the host terminal. RootWidget::cycleTheme keeps
    // the color in sync when the theme changes.
    terminal.setCursorStyle(Tui::CursorStyle::Bar);
    const Tui::ZColor caret = tpal::accent();
    terminal.setCursorColor(caret.red(), caret.green(), caret.blue());

    // Mouse: feed decoded events to the shell's hit-tester. Optional stderr trace
    // (DAEMON_TUI_MOUSE_DEBUG) helps diagnose terminals whose reports differ; it
    // corrupts the alt-screen while on, so it is opt-in only.
    const bool mouseDebug = qEnvironmentVariableIsSet("DAEMON_TUI_MOUSE_DEBUG");
    QObject::connect(&terminal, &MouseTerminal::mouseInput, &root,
                     [&root, mouseDebug](QPoint pos, MouseTerminal::MouseAction action, int button,
                                         Qt::KeyboardModifiers mods) {
                         if (mouseDebug) {
                             fprintf(stderr, "mouse: (%d,%d) action=%d button=%d mods=%d\n", pos.x(),
                                     pos.y(), static_cast<int>(action), button,
                                     static_cast<int>(mods));
                         }
                         root.handleMouse(pos, action, button, mods);
                     });

    // Enable mouse reporting once the event loop is running (so it lands after
    // ZTerminal has initialized the terminal/alt-screen, not interleaved with it).
    QTimer::singleShot(0, &terminal, [] { MouseTerminal::enableMouseReporting(); });

    return app.exec();
}
