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

// {"FsList": [ {"name":"a.txt","path":"a.txt","kind":"file","size":5,"mtime_ms":9} ]}
QByteArray fsListResp() {
    QByteArray b;
    mapHdr(b, 1);
    cborText(b, "FsList");
    arrHdr(b, 1);
    mapHdr(b, 5);
    cborText(b, "name");
    cborText(b, "a.txt");
    cborText(b, "path");
    cborText(b, "a.txt");
    cborText(b, "kind");
    cborText(b, "file");
    cborText(b, "size");
    cborUint(b, 5);
    cborText(b, "mtime_ms");
    cborUint(b, 9);
    return b;
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
