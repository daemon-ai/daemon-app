// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// Offscreen tests for the TUI fuzzy file-finder overlay (FileFinderDialog over
// the shared files::FileFinderModel, the same view-model the GUI FileFinder.qml
// binds): open -> type -> results populate from a temp workspace served through
// the LocalDiskFsService fs seam -> Enter chooses a preview open, Shift+Enter a
// pinned open, Esc dismisses without choosing. Also locks the composer attach
// picker's kind heuristic (rwdetail::attachmentKindForName) to the GUI
// DropArea's image-extension rule.

#include "file_finder_dialog.h"
#include "file_finder_model.h"
#include "fs/local_disk_fs_service.h"
#include "root_widget_detail.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>
#include <Tui/ZImage.h>
#include <Tui/ZRoot.h>
#include <Tui/ZTerminal.h>
#include <Tui/ZTest.h>

namespace {

// Flatten the off-screen terminal frame to text (cluster-aware, like the
// daemon-tui offscreen dump) so we can assert on the visible glyphs.
QString frameText(Tui::ZTerminal& terminal) {
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

bool writeFile(const QString& path, const QByteArray& bytes) {
    if (!QDir().mkpath(QFileInfo(path).absolutePath())) {
        return false;
    }
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) {
        return false;
    }
    return f.write(bytes) == bytes.size();
}

// Seed the workspace the tests index: a root-level file, a nested text file,
// and a nested image.
bool makeTree(const QString& base) {
    return writeFile(base + QStringLiteral("/alpha.txt"), QByteArrayLiteral("alpha")) &&
           writeFile(base + QStringLiteral("/notes/beta.md"), QByteArrayLiteral("beta")) &&
           writeFile(base + QStringLiteral("/img/pic.png"), QByteArrayLiteral("png"));
}

} // namespace

class TestFileFinderDialog : public QObject {
    Q_OBJECT

private slots:
    // Open -> type -> the model-driven list narrows to the fuzzy match ->
    // Enter reports the (rootId, root-relative path) with pinned=false (the
    // preview open), and ranks the choice first on the next empty-query open.
    void typeFiltersAndEnterChoosesPreview() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QVERIFY(makeTree(dir.path()));

        fs::LocalDiskFsService service(dir.path());
        files::FileFinderModel model;
        model.setService(&service);
        QTRY_VERIFY(!model.indexing() && model.fileCount() == 3);

        Tui::ZTerminal terminal{Tui::ZTerminal::OffScreen{80, 24}};
        Tui::ZRoot root;
        terminal.setMainWidget(&root);
        auto* dialog = new FileFinderDialog(QStringLiteral("Go to file"), QStringLiteral("hint"),
                                            &model, &root);
        QSignalSpy chosenSpy(dialog, &FileFinderDialog::chosen);

        dialog->openCentered();
        QTRY_COMPARE(model.rowCount(), 3); // empty query lists the whole index

        Tui::ZTest::sendText(&terminal, QStringLiteral("beta"), Qt::NoModifier);
        QTRY_COMPARE(model.rowCount(), 1); // fuzzy-narrowed to notes/beta.md
        QTRY_VERIFY2(frameText(terminal).contains(QStringLiteral("beta.md")),
                     qPrintable(frameText(terminal)));

        Tui::ZTest::sendKey(&terminal, Qt::Key_Enter, Qt::NoModifier);
        QCOMPARE(chosenSpy.count(), 1);
        QVERIFY(!chosenSpy.at(0).at(0).toString().isEmpty()); // a real root id
        QCOMPARE(chosenSpy.at(0).at(1).toString(), QStringLiteral("notes/beta.md"));
        QCOMPARE(chosenSpy.at(0).at(2).toBool(), false); // Enter previews
        QTRY_VERIFY(!dialog->isVisible());

        // The choice was noted as recent: an empty-query reopen ranks it first.
        dialog->openCentered();
        QTRY_COMPARE(model.rowCount(), 3);
        QCOMPARE(model.data(model.index(0, 0), files::FileFinderModel::PathRole).toString(),
                 QStringLiteral("notes/beta.md"));
    }

    // Shift+Enter is the deliberate variant: pinned=true (the GUI explorer's
    // double-click mapped to keys).
    void shiftEnterChoosesPinned() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QVERIFY(makeTree(dir.path()));

        fs::LocalDiskFsService service(dir.path());
        files::FileFinderModel model;
        model.setService(&service);
        QTRY_VERIFY(!model.indexing() && model.fileCount() == 3);

        Tui::ZTerminal terminal{Tui::ZTerminal::OffScreen{80, 24}};
        Tui::ZRoot root;
        terminal.setMainWidget(&root);
        auto* dialog = new FileFinderDialog(QStringLiteral("Go to file"), QStringLiteral("hint"),
                                            &model, &root);
        QSignalSpy chosenSpy(dialog, &FileFinderDialog::chosen);

        dialog->openCentered();
        Tui::ZTest::sendText(&terminal, QStringLiteral("alpha"), Qt::NoModifier);
        QTRY_COMPARE(model.rowCount(), 1);
        Tui::ZTest::sendKey(&terminal, Qt::Key_Enter, Qt::ShiftModifier);
        QCOMPARE(chosenSpy.count(), 1);
        QCOMPARE(chosenSpy.at(0).at(1).toString(), QStringLiteral("alpha.txt"));
        QCOMPARE(chosenSpy.at(0).at(2).toBool(), true); // Shift+Enter pins
    }

    // Esc dismisses without a choice (the attach picker uses this to hand
    // focus back to the composer).
    void escDismissesWithoutChoice() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QVERIFY(makeTree(dir.path()));

        fs::LocalDiskFsService service(dir.path());
        files::FileFinderModel model;
        model.setService(&service);
        QTRY_VERIFY(!model.indexing() && model.fileCount() == 3);

        Tui::ZTerminal terminal{Tui::ZTerminal::OffScreen{80, 24}};
        Tui::ZRoot root;
        terminal.setMainWidget(&root);
        auto* dialog = new FileFinderDialog(QStringLiteral("Attach file"), QStringLiteral("hint"),
                                            &model, &root);
        QSignalSpy chosenSpy(dialog, &FileFinderDialog::chosen);
        QSignalSpy dismissedSpy(dialog, &FileFinderDialog::dismissed);

        dialog->openCentered();
        QTRY_VERIFY(dialog->isVisible());
        Tui::ZTest::sendKey(&terminal, Qt::Key_Escape, Qt::NoModifier);
        QCOMPARE(dismissedSpy.count(), 1);
        QCOMPARE(chosenSpy.count(), 0);
        QTRY_VERIFY(!dialog->isVisible());
    }

    // The attach flow's kind detection matches the GUI DropArea heuristic:
    // image extensions attach as "image", everything else as "file".
    void attachmentKindMatchesGuiDropHeuristic() {
        QCOMPARE(rwdetail::attachmentKindForName(QStringLiteral("img/pic.png")),
                 QStringLiteral("image"));
        QCOMPARE(rwdetail::attachmentKindForName(QStringLiteral("photo.JPeG")),
                 QStringLiteral("image"));
        QCOMPARE(rwdetail::attachmentKindForName(QStringLiteral("art.svg")),
                 QStringLiteral("image"));
        QCOMPARE(rwdetail::attachmentKindForName(QStringLiteral("notes/beta.md")),
                 QStringLiteral("file"));
        QCOMPARE(rwdetail::attachmentKindForName(QStringLiteral("archive.png.zip")),
                 QStringLiteral("file"));
    }
};

QTEST_MAIN(TestFileFinderDialog)
#include "tst_file_finder_dialog.moc"
