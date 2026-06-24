// Render + interaction tests for the TUI TabStripView (the terminal analog of the
// QML TabBar). They drive a real TabStripView bound to the shared TabModel inside
// an off-screen Tui terminal, then assert the painted frame (tab titles + the "+"
// affordance) and the hit-testing (click a chip to activate, click its "x" to
// close, click "+" to request a new tab).

#include "tab_strip_view.h"

#include "tab_model.h"

#include <Tui/ZImage.h>
#include <Tui/ZTerminal.h>
#include <Tui/ZTest.h>
#include <Tui/ZWidget.h>

#include <QEventLoop>
#include <QSignalSpy>
#include <QTimer>
#include <QtTest>

namespace {

// Flatten the off-screen terminal frame to one string per row (cluster-aware, like
// the daemon-tui offscreen dump) so we can assert on the visible glyphs.
QString frameText(Tui::ZTerminal& terminal)
{
    const Tui::ZImage img = terminal.grabCurrentImage();
    QString out;
    for (int y = 0; y < img.height(); ++y) {
        for (int x = 0; x < img.width(); ++x) {
            int left = 0;
            int right = 0;
            const QString cell = img.peekText(x, y, &left, &right);
            out += (left == x && !cell.isEmpty()) ? cell : QStringLiteral(" ");
        }
        out += QLatin1Char('\n');
    }
    return out;
}

void settle()
{
    QEventLoop loop;
    QTimer::singleShot(20, &loop, &QEventLoop::quit);
    loop.exec();
}

} // namespace

class TestTabStrip : public QObject {
    Q_OBJECT

private slots:
    // The strip paints each open tab's title, an "x" per closable tab, and the
    // trailing "+" affordance.
    void rendersTabsAndPlus()
    {
        Tui::ZTerminal terminal { Tui::ZTerminal::OffScreen { 80, 1 } };
        Tui::ZWidget root;
        terminal.setMainWidget(&root);

        TabModel model;
        auto* strip = new TabStripView(&root);
        strip->setGeometry({ 0, 0, 80, 1 });
        strip->setModel(&model);

        model.openTranscript(1, QStringLiteral("Alpha"));
        model.openTranscript(2, QStringLiteral("Beta"));
        settle();

        const QString frame = frameText(terminal);
        QVERIFY2(frame.contains(QStringLiteral("Alpha")), qPrintable(frame));
        QVERIFY2(frame.contains(QStringLiteral("Beta")), qPrintable(frame));
        QVERIFY2(frame.contains(QStringLiteral("\u00d7")), qPrintable(frame)); // close x
        QVERIFY2(frame.contains(QStringLiteral("+")), qPrintable(frame));      // new-tab
    }

    // Closing the active tab drops its title from the strip.
    void closeUpdatesStrip()
    {
        Tui::ZTerminal terminal { Tui::ZTerminal::OffScreen { 80, 1 } };
        Tui::ZWidget root;
        terminal.setMainWidget(&root);

        TabModel model;
        auto* strip = new TabStripView(&root);
        strip->setGeometry({ 0, 0, 80, 1 });
        strip->setModel(&model);

        model.openTranscript(1, QStringLiteral("Alpha"));
        model.openTranscript(2, QStringLiteral("Beta"));
        model.closeTab(0); // remove Alpha
        settle();

        const QString frame = frameText(terminal);
        QVERIFY2(!frame.contains(QStringLiteral("Alpha")), qPrintable(frame));
        QVERIFY2(frame.contains(QStringLiteral("Beta")), qPrintable(frame));
    }

    // Clicking a chip's body activates it; clicking its "x" closes it; clicking the
    // "+" affordance emits newTabRequested. Layout per chip is
    // " " + title + " " + "x" + " ", laid left-to-right from x=0.
    void clickActivatesClosesAndAdds()
    {
        Tui::ZTerminal terminal { Tui::ZTerminal::OffScreen { 80, 1 } };
        Tui::ZWidget root;
        terminal.setMainWidget(&root);

        TabModel model;
        auto* strip = new TabStripView(&root);
        strip->setGeometry({ 0, 0, 80, 1 });
        strip->setModel(&model);
        QSignalSpy newSpy(strip, &TabStripView::newTabRequested);

        model.openTranscript(1, QStringLiteral("Alpha")); // chip 0: x0=0  x1=9  closeX=7
        model.openTranscript(2, QStringLiteral("Beta"));  // chip 1: x0=9  x1=17 closeX=15
                                                          // "+":     x0=17 x1=20
        QCOMPARE(model.currentIndex(), 1);

        // Click the body of chip 0 -> it becomes active.
        strip->clickAt({ 2, 0 });
        QCOMPARE(model.currentIndex(), 0);

        // Click the "+" affordance -> a new tab is requested.
        strip->clickAt({ 18, 0 });
        QCOMPARE(newSpy.count(), 1);

        // Click chip 0's "x" -> it closes (Beta remains).
        strip->clickAt({ 7, 0 });
        QCOMPARE(model.count(), 1);
        QCOMPARE(model.sessionIdAt(0), 2);
    }

    // A preview open reuses the single preview chip in place: the strip repaints
    // the reassigned title rather than growing a second chip.
    void previewReusesSingleChip()
    {
        Tui::ZTerminal terminal { Tui::ZTerminal::OffScreen { 80, 1 } };
        Tui::ZWidget root;
        terminal.setMainWidget(&root);

        TabModel model;
        auto* strip = new TabStripView(&root);
        strip->setGeometry({ 0, 0, 80, 1 });
        strip->setModel(&model);

        model.previewTranscript(1, QStringLiteral("Alpha"));
        settle();
        QString frame = frameText(terminal);
        QVERIFY2(frame.contains(QStringLiteral("Alpha")), qPrintable(frame));

        model.previewTranscript(2, QStringLiteral("Beta"));
        settle();
        QCOMPARE(model.count(), 1); // reused, not appended
        frame = frameText(terminal);
        QVERIFY2(frame.contains(QStringLiteral("Beta")), qPrintable(frame));
        QVERIFY2(!frame.contains(QStringLiteral("Alpha")), qPrintable(frame));
    }

    // The focused strip moves the active tab with Left/Right, clamped at the
    // ends (global Ctrl+Tab wrapping is handled elsewhere). Plain arrows only act
    // while the strip holds focus.
    void leftRightMovesActiveTabWhenFocused()
    {
        Tui::ZTerminal terminal { Tui::ZTerminal::OffScreen { 80, 1 } };
        Tui::ZWidget root;
        root.setFocusPolicy(Tui::StrongFocus);
        terminal.setMainWidget(&root);

        TabModel model;
        auto* strip = new TabStripView(&root);
        strip->setGeometry({ 0, 0, 80, 1 });
        strip->setModel(&model);

        model.openTranscript(1, QStringLiteral("Alpha"));
        model.openTranscript(2, QStringLiteral("Beta"));
        model.openTranscript(3, QStringLiteral("Gamma"));
        QCOMPARE(model.currentIndex(), 2);

        strip->setFocus();
        QVERIFY(strip->focus());

        Tui::ZTest::sendKey(&terminal, Qt::Key_Left, Qt::NoModifier);
        QCOMPARE(model.currentIndex(), 1);
        Tui::ZTest::sendKey(&terminal, Qt::Key_Left, Qt::NoModifier);
        QCOMPARE(model.currentIndex(), 0);
        Tui::ZTest::sendKey(&terminal, Qt::Key_Left, Qt::NoModifier);
        QCOMPARE(model.currentIndex(), 0); // clamped at the start

        Tui::ZTest::sendKey(&terminal, Qt::Key_Right, Qt::NoModifier);
        QCOMPARE(model.currentIndex(), 1);
    }

    // Enter pins the current preview tab (mirroring the GUI double-click), and
    // Delete closes a closable current tab.
    void enterPinsPreviewAndDeleteCloses()
    {
        Tui::ZTerminal terminal { Tui::ZTerminal::OffScreen { 80, 1 } };
        Tui::ZWidget root;
        root.setFocusPolicy(Tui::StrongFocus);
        terminal.setMainWidget(&root);

        TabModel model;
        auto* strip = new TabStripView(&root);
        strip->setGeometry({ 0, 0, 80, 1 });
        strip->setModel(&model);

        model.previewTranscript(1, QStringLiteral("Alpha"));
        QVERIFY(model.isPreviewAt(0));
        strip->setFocus();

        Tui::ZTest::sendKey(&terminal, Qt::Key_Enter, Qt::NoModifier);
        QVERIFY(!model.isPreviewAt(0)); // graduated to a pinned tab

        model.openTranscript(2, QStringLiteral("Beta"));
        QCOMPARE(model.count(), 2);
        QCOMPARE(model.currentIndex(), 1);
        Tui::ZTest::sendKey(&terminal, Qt::Key_Delete, Qt::NoModifier);
        QCOMPARE(model.count(), 1);
    }
};

QTEST_MAIN(TestTabStrip)
#include "tst_tab_strip.moc"
