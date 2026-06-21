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
    // composer to exercise the input path before grabbing the frame.
    const QByteArray typed = qgetenv("DAEMON_TUI_TYPE");
    QTimer::singleShot(0, [&] {
        if (!typed.isEmpty()) {
            root.focusComposer();
            Tui::ZTest::sendText(&terminal, QString::fromUtf8(typed), Qt::NoModifier);
            Tui::ZTest::sendKey(&terminal, Qt::Key_Enter, Qt::NoModifier);
        }
        QCoreApplication::quit();
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
