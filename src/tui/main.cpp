#include "root_widget.h"

#include <Tui/ZImage.h>
#include <Tui/ZTerminal.h>
#include <Tui/ZTest.h>

#include <QCoreApplication>
#include <QString>
#include <QTextStream>
#include <QTimer>

#include <cstdio>

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
    QTimer::singleShot(0, [&] {
        if (!keys.isEmpty()) {
            const QList<QByteArray> seq = keys.split(',');
            for (const QByteArray& k : seq) {
                const QByteArray name = k.trimmed().toLower();
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
                } else if (name == "focus-composer") {
                    root.focusComposer();
                } else if (name.startsWith("t:")) {
                    // Type a literal into whatever holds focus (e.g. the composer),
                    // without the auto-Enter that DAEMON_TUI_TYPE appends. Used to
                    // exercise the context-sensitive Esc levels.
                    Tui::ZTest::sendText(&terminal, QString::fromUtf8(k.trimmed().mid(2)),
                                         Qt::NoModifier);
                }
            }
        }
        if (!typed.isEmpty()) {
            root.focusComposer();
            Tui::ZTest::sendText(&terminal, QString::fromUtf8(typed), Qt::NoModifier);
            Tui::ZTest::sendKey(&terminal, Qt::Key_Enter, Qt::NoModifier);
        }
        // Let any deferred show/layout/paint (e.g. the quit-confirmation dialog,
        // which ZDialog shows on a queued timer) settle before grabbing the frame.
        // DAEMON_TUI_TURN_MS extends this window so a submitted prompt's scripted
        // assistant turn can stream into the transcript and the footer's Running
        // timer can tick before the frame is grabbed.
        const int settleMs = qMax(50, qgetenv("DAEMON_TUI_TURN_MS").toInt());
        QTimer::singleShot(settleMs, [] { QCoreApplication::quit(); });
    });
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

    if (maybeRenderOffscreen()) {
        return 0;
    }

    Tui::ZTerminal terminal;
    RootWidget root;
    terminal.setMainWidget(&root);

    return app.exec();
}
