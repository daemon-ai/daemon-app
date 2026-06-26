#include "fs/local_disk_fs_service.h"

#include <filesystem>
#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

class TestFsService : public QObject {
    Q_OBJECT

private slots:
    void staleWriteIsRejected() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = dir.path() + QStringLiteral("/note.txt");
        {
            QFile f(path);
            QVERIFY(f.open(QIODevice::WriteOnly));
            QCOMPARE(f.write("one"), qint64(3));
        }

        fs::LocalDiskFsService svc(dir.path());
        QSignalSpy readSpy(&svc, &fs::IFsService::fileRead);
        svc.read(QStringLiteral("workspace"), QStringLiteral("note.txt"));
        QVERIFY(readSpy.wait());
        const QString base = readSpy.takeFirst().at(3).toString();

        {
            QFile f(path);
            QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
            QCOMPARE(f.write("twenty"), qint64(6));
        }

        QSignalSpy writeSpy(&svc, &fs::IFsService::writeResult);
        svc.write(QStringLiteral("workspace"), QStringLiteral("note.txt"), QByteArray("three"),
                  base);
        QVERIFY(writeSpy.wait());
        const QList<QVariant> args = writeSpy.takeFirst();
        QCOMPARE(args.at(2).toBool(), false);
        QVERIFY(args.at(4).toString().contains(QStringLiteral("stale")));
    }

    void symlinkEscapeIsRejected() {
        QTemporaryDir root;
        QTemporaryDir outside;
        QVERIFY(root.isValid());
        QVERIFY(outside.isValid());
        {
            QFile f(outside.path() + QStringLiteral("/secret.txt"));
            QVERIFY(f.open(QIODevice::WriteOnly));
            QCOMPARE(f.write("secret"), qint64(6));
        }
        std::filesystem::create_symlink(
            (outside.path() + QStringLiteral("/secret.txt")).toStdString(),
            (root.path() + QStringLiteral("/link.txt")).toStdString());

        fs::LocalDiskFsService svc(root.path());
        QSignalSpy errorSpy(&svc, &fs::IFsService::error);
        QSignalSpy readSpy(&svc, &fs::IFsService::fileRead);
        svc.read(QStringLiteral("workspace"), QStringLiteral("link.txt"));
        QVERIFY(errorSpy.wait());
        QCOMPARE(readSpy.count(), 0);
    }

    void forceWriteOverridesStaleRevision() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = dir.path() + QStringLiteral("/note.txt");
        {
            QFile f(path);
            QVERIFY(f.open(QIODevice::WriteOnly));
            QCOMPARE(f.write("one"), qint64(3));
        }

        fs::LocalDiskFsService svc(dir.path());
        QSignalSpy readSpy(&svc, &fs::IFsService::fileRead);
        svc.read(QStringLiteral("workspace"), QStringLiteral("note.txt"));
        QVERIFY(readSpy.wait());
        const QString staleBase = readSpy.takeFirst().at(3).toString();

        {
            QFile f(path);
            QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
            QCOMPARE(f.write("external"), qint64(8));
        }

        QSignalSpy writeSpy(&svc, &fs::IFsService::writeResult);
        svc.write(QStringLiteral("workspace"), QStringLiteral("note.txt"), QByteArray("forced"),
                  staleBase, true);
        QVERIFY(writeSpy.wait());
        const QList<QVariant> args = writeSpy.takeFirst();
        QCOMPARE(args.at(2).toBool(), true);

        QFile f(path);
        QVERIFY(f.open(QIODevice::ReadOnly));
        QCOMPARE(f.readAll(), QByteArray("forced"));
    }
};

QTEST_MAIN(TestFsService)
#include "tst_fs_service.moc"
