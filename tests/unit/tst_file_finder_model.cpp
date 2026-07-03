// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "file_finder_model.h"
#include "fs/ifs_service.h"

#include <QSignalSpy>
#include <QtTest/QtTest>

namespace {

// A scriptable IFsService: records every open() and lets the test emit the signals by hand.
class FakeFsService : public fs::IFsService {
    Q_OBJECT

public:
    using fs::IFsService::IFsService;

    QList<fs::FsRoot> roots;
    QList<QPair<QString, QString>> opened; // (rootId, dir) in request order

    void listRoots() override { emit rootsChanged(roots); }
    void open(const QString& rootId, const QString& dir) override {
        opened.append(qMakePair(rootId, dir));
    }
    void stat(const QString&, const QString&) override {}
    void read(const QString&, const QString&) override {}
    void write(const QString&, const QString&, const QByteArray&, const QString&, bool) override {}
    void search(const QString&, const QString&, const QVariantMap&) override {}
    void watch(const QString&, const QString&) override {}
    void unwatch(const QString&, const QString&) override {}
};

fs::FsRoot root(const char* id, const char* label) {
    fs::FsRoot r;
    r.id = QString::fromLatin1(id);
    r.label = QString::fromLatin1(label);
    return r;
}

fs::FsEntry dirEntry(const QString& path) {
    fs::FsEntry e;
    e.path = path;
    e.name = path.section(QLatin1Char('/'), -1);
    e.isDir = true;
    return e;
}

fs::FsEntry fileEntry(const QString& path) {
    fs::FsEntry e;
    e.path = path;
    e.name = path.section(QLatin1Char('/'), -1);
    return e;
}

} // namespace

// The Ctrl+P index walk (FileFinderModel): which roots it crawls and how it survives failed
// directory listings. The perf-regression guard: the walk must never recurse into a host BROWSE
// root (the user's whole home served over the wire) — pre-pagination that crawl aborted on the
// first >64-entry directory (masking it); with wire pagination it would flood the node with
// thousands of FsList pages while the app sits "idle".
class TestFileFinderModel : public QObject {
    Q_OBJECT

private slots:
    // Host browse roots ("host:<id>") are excluded from the index walk; project-scoped roots
    // (workspace + session sandboxes) are crawled as before.
    void hostRootsAreNotIndexed() {
        FakeFsService svc;
        svc.roots = {root("workspace", "ws"), root("host:home", "home"),
                     root("session:s1", "sandbox")};
        files::FileFinderModel model;
        model.setService(&svc);

        QCOMPARE(svc.opened.size(), 2);
        QCOMPARE(svc.opened.at(0), qMakePair(QStringLiteral("workspace"), QString()));
        QCOMPARE(svc.opened.at(1), qMakePair(QStringLiteral("session:s1"), QString()));

        // Subdirectories of an indexed root are walked; the host root stays untouched.
        emit svc.listed(QStringLiteral("workspace"), QString(),
                        {dirEntry(QStringLiteral("src")), fileEntry(QStringLiteral("a.txt"))});
        emit svc.listed(QStringLiteral("session:s1"), QString(), {});
        QCOMPARE(svc.opened.size(), 3);
        QCOMPARE(svc.opened.at(2), qMakePair(QStringLiteral("workspace"), QStringLiteral("src")));
        emit svc.listed(QStringLiteral("workspace"), QStringLiteral("src"), {});
        QVERIFY(!model.indexing());
        QCOMPARE(model.fileCount(), 1);
    }

    // Only host roots: nothing to crawl — the walk finishes immediately instead of spinning.
    void hostOnlyRootsFinishImmediately() {
        FakeFsService svc;
        svc.roots = {root("host:home", "home")};
        files::FileFinderModel model;
        model.setService(&svc);
        QCOMPARE(svc.opened.size(), 0);
        QVERIFY(!model.indexing());
    }

    // A failed listing (unreadable dir / a paged listing that died mid-chain) frees its in-flight
    // slot so the rest of the walk proceeds — kMaxConcurrent(8) such failures used to wedge the
    // walk with the spinner stuck on.
    void errorFreesTheInFlightSlot() {
        FakeFsService svc;
        svc.roots = {root("workspace", "ws")};
        files::FileFinderModel model;
        model.setService(&svc);
        QCOMPARE(svc.opened.size(), 1);

        // 9 subdirectories: 8 fill every in-flight slot, the 9th queues.
        QList<fs::FsEntry> subdirs;
        for (int i = 0; i < 9; ++i) {
            subdirs.append(dirEntry(QStringLiteral("d%1").arg(i)));
        }
        emit svc.listed(QStringLiteral("workspace"), QString(), subdirs);
        QCOMPARE(svc.opened.size(), 9); // root + 8 in-flight subdirs

        // One of them fails: the slot frees and the queued 9th dir is requested.
        emit svc.error(QStringLiteral("workspace"), QStringLiteral("d0"),
                       QStringLiteral("permission denied"));
        QCOMPARE(svc.opened.size(), 10);
        QCOMPARE(svc.opened.at(9), qMakePair(QStringLiteral("workspace"), QStringLiteral("d8")));

        // The remaining dirs resolve empty; the walk completes despite the failure.
        for (int i = 1; i < 9; ++i) {
            emit svc.listed(QStringLiteral("workspace"), QStringLiteral("d%1").arg(i), {});
        }
        QVERIFY(!model.indexing());
    }
};

QTEST_GUILESS_MAIN(TestFileFinderModel)
#include "tst_file_finder_model.moc"
