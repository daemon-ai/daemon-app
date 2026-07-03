// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemon/daemon_cache_store.h"
#include "daemon/daemon_fs_service.h"
#include "daemon/daemon_transport.h"
#include "daemon/node_api_client.h"
#include "fs/fs_dtos.h"
#include "wire_mux_fixture.h"

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

using daemonapp::daemon::DaemonCacheStore;
using daemonapp::daemon::DaemonTransport;
using daemonapp::daemon::NodeApiClient;
using daemonapp::test::cborText;
using daemonapp::test::cborUint;
using daemonapp::test::WireMuxServer;
using fs::DaemonFsService;

namespace {

void mapHdr(QByteArray& b, int n) {
    b.append(static_cast<char>(0xA0 | n));
}
void arrHdr(QByteArray& b, int n) {
    b.append(static_cast<char>(0x80 | n));
}
void bstr(QByteArray& b, const QByteArray& bytes) {
    b.append(static_cast<char>(0x40 | static_cast<char>(bytes.size()))); // bstr, len < 24
    b.append(bytes);
}

// {"FsRoots": [ {"id":"Workspace","label":"ws","kind":"workspace"} ]}
QByteArray fsRootsResp() {
    QByteArray b;
    mapHdr(b, 1);
    cborText(b, "FsRoots");
    arrHdr(b, 1);
    mapHdr(b, 3);
    cborText(b, "id");
    cborText(b, "workspace"); // FsRootId::Workspace = the bare tstr variant "workspace"
    cborText(b, "label");
    cborText(b, "ws");
    cborText(b, "kind");
    cborText(b, "workspace");
    return b;
}

void fsEntry(QByteArray& b, const char* name) {
    mapHdr(b, 5);
    cborText(b, "name");
    cborText(b, name);
    cborText(b, "path");
    cborText(b, name); // root-relative path == name for a flat listing
    cborText(b, "kind");
    cborText(b, "file");
    cborText(b, "size");
    cborUint(b, 5);
    cborText(b, "mtime_ms");
    cborUint(b, 9);
}

// One fs_list page (the uniform WirePage, wire v25): {"FsList": {"items":[...], ?"next":
// <cursor>}}.
QByteArray fsListPageResp(const QList<QByteArray>& names, const char* next = nullptr) {
    QByteArray b;
    mapHdr(b, 1);
    cborText(b, "FsList");
    mapHdr(b, next != nullptr ? 2 : 1);
    cborText(b, "items");
    arrHdr(b, static_cast<int>(names.size()));
    for (const QByteArray& n : names) {
        fsEntry(b, n.constData());
    }
    if (next != nullptr) {
        cborText(b, "next");
        cborText(b, next);
    }
    return b;
}

QByteArray fsListResp() {
    return fsListPageResp({QByteArrayLiteral("a.txt")});
}

// {"FsRead": {"bytes":<bstr "hello">,"revision":{"mtime_ms":9,"size":5}}}
QByteArray fsReadResp() {
    QByteArray b;
    mapHdr(b, 1);
    cborText(b, "FsRead");
    mapHdr(b, 2);
    cborText(b, "bytes");
    bstr(b, QByteArrayLiteral("hello")); // bstr - the byte-array=bstr contract fix
    cborText(b, "revision");
    mapHdr(b, 2);
    cborText(b, "mtime_ms");
    cborUint(b, 9);
    cborText(b, "size");
    cborUint(b, 5);
    return b;
}

// {"FsWrite": {"mtime_ms":11,"size":5}}
QByteArray fsWriteResp() {
    QByteArray b;
    mapHdr(b, 1);
    cborText(b, "FsWrite");
    mapHdr(b, 2);
    cborText(b, "mtime_ms");
    cborUint(b, 11);
    cborText(b, "size");
    cborUint(b, 5);
    return b;
}

// {"FsWatch": {"events":[],"next_seq":N,"head_seq":N,"reset":bool}}
QByteArray fsWatchResp(quint64 seq, bool reset) {
    QByteArray b;
    mapHdr(b, 1);
    cborText(b, "FsWatch");
    mapHdr(b, 4);
    cborText(b, "events");
    arrHdr(b, 0);
    cborText(b, "next_seq");
    cborUint(b, seq);
    cborText(b, "head_seq");
    cborUint(b, seq);
    cborText(b, "reset");
    b.append(static_cast<char>(reset ? 0xF5 : 0xF4)); // true / false
    return b;
}

} // namespace

// Phase 4: DaemonFsService serves the file explorer/editor over the NodeApi fs_* ops + the codec
// facade, and the lossless watch path forces a re-list (changed) on a reset page.
class TestDaemonFsService : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_tmp;
    QString sock(const QString& n) { return m_tmp.filePath(n); }
    QString db(const QString& n) { return m_tmp.filePath(n); }

private slots:
    void rootsListReadWriteRoundTrip() {
        const QString path = sock(QStringLiteral("fs.sock"));
        WireMuxServer fake;
        QVERIFY2(fake.start(path), "listen");
        DaemonTransport transport;
        transport.setSocketPath(path);
        NodeApiClient client(&transport);
        DaemonCacheStore cache(db(QStringLiteral("fs.db")));
        DaemonFsService svc(&client, &cache);

        // listRoots -> rootsChanged with the Workspace root.
        fake.setReplyPayload(fsRootsResp());
        QSignalSpy roots(&svc, &fs::IFsService::rootsChanged);
        svc.listRoots();
        QTRY_COMPARE_WITH_TIMEOUT(roots.count(), 1, 3000);
        const auto rootList = qvariant_cast<QList<fs::FsRoot>>(roots.at(0).at(0));
        QCOMPARE(rootList.size(), 1);
        QCOMPARE(rootList.first().id, QStringLiteral("workspace"));

        // open -> listed; the entry is mirrored into the offline cache (daemon_fs_entries).
        fake.setReplyPayload(fsListResp());
        QSignalSpy listed(&svc, &fs::IFsService::listed);
        svc.open(QStringLiteral("workspace"), QString());
        QTRY_COMPARE_WITH_TIMEOUT(listed.count(), 1, 3000);
        const auto entries = qvariant_cast<QList<fs::FsEntry>>(listed.at(0).at(2));
        QCOMPARE(entries.size(), 1);
        QCOMPARE(entries.first().name, QStringLiteral("a.txt"));
        QCOMPARE(cache.fsEntries(QStringLiteral("workspace")).size(), 1);

        // read -> fileRead with the bstr-decoded content + the "mtime:size" revision.
        fake.setReplyPayload(fsReadResp());
        QSignalSpy readSpy(&svc, &fs::IFsService::fileRead);
        svc.read(QStringLiteral("workspace"), QStringLiteral("a.txt"));
        QTRY_COMPARE_WITH_TIMEOUT(readSpy.count(), 1, 3000);
        QCOMPARE(readSpy.at(0).at(2).toByteArray(), QByteArrayLiteral("hello"));
        QCOMPARE(readSpy.at(0).at(3).toString(), QStringLiteral("9:5"));

        // write -> writeResult ok + the new revision.
        fake.setReplyPayload(fsWriteResp());
        QSignalSpy wrote(&svc, &fs::IFsService::writeResult);
        svc.write(QStringLiteral("workspace"), QStringLiteral("a.txt"), QByteArrayLiteral("hullo"),
                  QStringLiteral("9:5"), false);
        QTRY_COMPARE_WITH_TIMEOUT(wrote.count(), 1, 3000);
        QVERIFY(wrote.at(0).at(2).toBool());                            // ok
        QCOMPARE(wrote.at(0).at(3).toString(), QStringLiteral("11:5")); // revision
    }

    // Wire v24 page loop: a listing spanning two pages is fetched to completion — the second
    // request carries the first page's `next` as its `after` cursor — and the consumer still
    // sees exactly ONE listed() with the full directory (FsExplorerModel's contract).
    void listPagesToOneCompleteSignal() {
        const QString path = sock(QStringLiteral("fsp.sock"));
        WireMuxServer fake;
        QVERIFY2(fake.start(path), "listen");
        DaemonTransport transport;
        transport.setSocketPath(path);
        NodeApiClient client(&transport);
        DaemonCacheStore cache(db(QStringLiteral("fsp.db")));
        DaemonFsService svc(&client, &cache);

        fake.setReplySequence(
            {fsListPageResp({QByteArrayLiteral("a.txt"), QByteArrayLiteral("b.txt")}, "b.txt"),
             fsListPageResp({QByteArrayLiteral("c.txt")})});
        QSignalSpy listed(&svc, &fs::IFsService::listed);
        svc.open(QStringLiteral("workspace"), QString());
        QTRY_COMPARE_WITH_TIMEOUT(listed.count(), 1, 3000);
        QTest::qWait(100); // settle: a page-loop bug would emit again / issue extra requests
        QCOMPARE(listed.count(), 1);

        // One complete signal, all three entries, page order preserved.
        const auto entries = qvariant_cast<QList<fs::FsEntry>>(listed.at(0).at(2));
        QCOMPARE(entries.size(), 3);
        QCOMPARE(entries.at(0).name, QStringLiteral("a.txt"));
        QCOMPARE(entries.at(1).name, QStringLiteral("b.txt"));
        QCOMPARE(entries.at(2).name, QStringLiteral("c.txt"));

        // The continuation request carried the cursor: its bytes are exactly the codec's encoding
        // of the same FsList with after = "b.txt".
        const QList<QByteArray> calls = fake.callPayloads();
        QCOMPARE(calls.size(), 2);
        QCOMPARE(calls.at(0), daemonapp::daemon::NodeApiCodec::encodeFsListRequest(
                                  QStringLiteral("workspace"), QString()));
        QCOMPARE(calls.at(1),
                 daemonapp::daemon::NodeApiCodec::encodeFsListRequest(
                     QStringLiteral("workspace"), QString(), false, QStringLiteral("b.txt")));

        // All pages were mirrored into the offline cache.
        QCOMPARE(cache.fsEntries(QStringLiteral("workspace")).size(), 3);
    }

    // A defective server that echoes a NON-ADVANCING cursor (`next` == the `after` the client just
    // sent) must not spin the page loop: the client stops after the one wasted retry, serves what
    // accumulated in a single listed(), and issues no further request. (The node-side `paginate`
    // proves it never echoes the cursor; this guards the client against any future server bug.)
    void nonAdvancingCursorStopsTheLoop() {
        const QString path = sock(QStringLiteral("fsc.sock"));
        WireMuxServer fake;
        QVERIFY2(fake.start(path), "listen");
        DaemonTransport transport;
        transport.setSocketPath(path);
        NodeApiClient client(&transport);
        DaemonCacheStore cache(db(QStringLiteral("fsc.db")));
        DaemonFsService svc(&client, &cache);

        // Every reply claims more pages remain at the SAME cursor; an unguarded loop would
        // re-issue after="b.txt" forever (the fixed payload keeps serving it past the sequence).
        const QByteArray echoing =
            fsListPageResp({QByteArrayLiteral("a.txt"), QByteArrayLiteral("b.txt")}, "b.txt");
        fake.setReplyPayload(echoing);
        QSignalSpy listed(&svc, &fs::IFsService::listed);
        svc.open(QStringLiteral("workspace"), QString());
        QTRY_COMPARE_WITH_TIMEOUT(listed.count(), 1, 3000);
        QTest::qWait(150); // settle: a spinning loop would keep issuing requests
        QCOMPARE(listed.count(), 1);
        // Page 1 (cursor recorded) + exactly one continuation (cursor did not advance -> stop).
        QCOMPARE(fake.requestCount(), 2);
        const auto entries = qvariant_cast<QList<fs::FsEntry>>(listed.at(0).at(2));
        QCOMPARE(entries.size(), 4); // both pages' items are served rather than dropped
    }

    // stat() reuses the fs_list page decode but stays a single-shot: one statResult, no loop.
    void statStaysSingleShot() {
        const QString path = sock(QStringLiteral("fss.sock"));
        WireMuxServer fake;
        QVERIFY2(fake.start(path), "listen");
        DaemonTransport transport;
        transport.setSocketPath(path);
        NodeApiClient client(&transport);
        DaemonCacheStore cache(db(QStringLiteral("fss.db")));
        DaemonFsService svc(&client, &cache);

        // Even a page claiming `next` must not make stat loop (single-shot by contract).
        fake.setReplyPayload(fsListPageResp({QByteArrayLiteral("a.txt")}, "a.txt"));
        QSignalSpy statSpy(&svc, &fs::IFsService::statResult);
        svc.stat(QStringLiteral("workspace"), QStringLiteral("a.txt"));
        QTRY_COMPARE_WITH_TIMEOUT(statSpy.count(), 1, 3000);
        QVERIFY(statSpy.at(0).at(3).toBool()); // ok
        const auto entry = qvariant_cast<fs::FsEntry>(statSpy.at(0).at(2));
        QCOMPARE(entry.name, QStringLiteral("a.txt"));
        QTest::qWait(100); // settle: a looping stat would issue a second request
        QCOMPARE(fake.requestCount(), 1);
    }

    // decodeFsList surfaces the page's resume cursor: set when `next` is present, cleared on the
    // last page (and the null arm decodes as "no more pages" too).
    void decodeFsListSurfacesNextCursor() {
        using daemonapp::daemon::NodeApiCodec;
        QList<daemonapp::daemon::DecodedFsEntry> entries;
        QString next;
        QVERIFY(NodeApiCodec::decodeFsList(fsListPageResp({QByteArrayLiteral("a.txt")}, "a.txt"),
                                           &entries, &next));
        QCOMPARE(entries.size(), 1);
        QCOMPARE(next, QStringLiteral("a.txt"));

        QVERIFY(NodeApiCodec::decodeFsList(fsListPageResp({QByteArrayLiteral("a.txt")}), &entries,
                                           &next));
        QVERIFY(next.isEmpty());
    }

    // The lossless watch path: a page with reset=true forces a changed() so the consumer re-lists.
    void watchResetForcesChanged() {
        const QString path = sock(QStringLiteral("fsw.sock"));
        WireMuxServer fake;
        QVERIFY2(fake.start(path), "listen");
        DaemonTransport transport;
        transport.setSocketPath(path);
        NodeApiClient client(&transport);
        DaemonCacheStore cache(db(QStringLiteral("fsw.db")));
        DaemonFsService svc(&client, &cache);

        fake.setReplyPayload(fsWatchResp(7, /*reset=*/true));
        QSignalSpy changed(&svc, &fs::IFsService::changed);
        svc.watch(QStringLiteral("workspace"), QString()); // primes + polls immediately
        QTRY_VERIFY_WITH_TIMEOUT(changed.count() >= 1, 3000);
        QCOMPARE(changed.at(0).at(0).toString(), QStringLiteral("workspace"));
        svc.unwatch(QStringLiteral("workspace"), QString());
    }
};

QTEST_GUILESS_MAIN(TestDaemonFsService)
#include "tst_daemon_fs_service.moc"
