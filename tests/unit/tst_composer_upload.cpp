// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "composer_upload_controller.h"

#include <QDateTime>
#include <QTest>
#include <QTimeZone>

// The workspace upload-path helper (W7): uploads/<utc-timestamp>-<sanitized-name>. Pure logic, so
// it verifies here without a browser picker or an fs service. The upload wiring itself (pick ->
// IFsService write -> chip) is browser-only and exercised by the wasm e2e.
class TstComposerUpload : public QObject {
    Q_OBJECT

private slots:
    static void prefixAndTimestamp() {
        const QDateTime when(QDate(2026, 7, 4), QTime(9, 6, 1, 123), QTimeZone::UTC);
        const QString path =
            ComposerUploadController::uploadPathFor(QStringLiteral("photo.png"), when);
        QVERIFY(path.startsWith(QStringLiteral("uploads/")));
        QCOMPARE(path, QStringLiteral("uploads/20260704T090601123-photo.png"));
    }

    static void usesUtc() {
        // A non-UTC input is normalized to UTC before stamping.
        const QDateTime local(QDate(2026, 1, 2), QTime(3, 4, 5, 6),
                              QTimeZone::fromSecondsAheadOfUtc(2 * 3600));
        const QString path =
            ComposerUploadController::uploadPathFor(QStringLiteral("a.txt"), local);
        // 03:04:05 at +02:00 is 01:04:05 UTC.
        QCOMPARE(path, QStringLiteral("uploads/20260102T010405006-a.txt"));
    }

    static void sanitizesUnsafeCharacters() {
        const QDateTime when(QDate(2026, 7, 4), QTime(0, 0, 0), QTimeZone::UTC);
        const QString path =
            ComposerUploadController::uploadPathFor(QStringLiteral("my report (final)!.PNG"), when);
        QCOMPARE(path, QStringLiteral("uploads/20260704T000000000-my_report__final__.PNG"));
    }

    static void basenamesPathSeparators() {
        const QDateTime when(QDate(2026, 7, 4), QTime(0, 0, 0), QTimeZone::UTC);
        // A browser may report a relative path (webkitdirectory) or Windows separators.
        QCOMPARE(ComposerUploadController::uploadPathFor(QStringLiteral("sub/dir/image.jpg"), when),
                 QStringLiteral("uploads/20260704T000000000-image.jpg"));
        QCOMPARE(
            ComposerUploadController::uploadPathFor(QStringLiteral("C:\\Users\\x\\note.txt"), when),
            QStringLiteral("uploads/20260704T000000000-note.txt"));
    }

    static void emptyNameFallsBackToFile() {
        const QDateTime when(QDate(2026, 7, 4), QTime(0, 0, 0), QTimeZone::UTC);
        QCOMPARE(ComposerUploadController::uploadPathFor(QString(), when),
                 QStringLiteral("uploads/20260704T000000000-file"));
        // A name that sanitizes to nothing meaningful still yields a leaf (all-separators case).
        QCOMPARE(ComposerUploadController::uploadPathFor(QStringLiteral("///"), when),
                 QStringLiteral("uploads/20260704T000000000-file"));
    }
};

QTEST_MAIN(TstComposerUpload)
#include "tst_composer_upload.moc"
