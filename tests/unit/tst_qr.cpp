// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// [integrations wire v38] A3 app/auth-component: the shared QR module-matrix seam that BOTH auth
// surfaces render from (GUI Kit.QrCode paints it; the TUI auth panel projects it to unicode
// half-blocks). Two independently-verifiable pieces, no front end required:
//   1. QrMatrix::encode() — a known-vector check against Nayuki qrcodegen's reference behavior: a
//      fixed alphanumeric payload lands at the expected module dimensions (version 1, 21x21) and
//      the structural modules the QR spec fixes for EVERY code (the three finder patterns + their
//      separators, the timing tracks, the always-dark module) sit exactly where the spec puts them.
//   2. renderHalfBlocks() — the terminal projection: a fixed matrix maps to the exact half-block
//      glyph rows (two modules per character cell), and the quiet zone frames it correctly.

#include "qr/qr_matrix.h"

#include <QtTest/QtTest>

using qr::QrMatrix;

namespace {
// The three half-block glyphs the projection emits (a dark module is the FILLED half).
const QChar kUpper(0x2580); // ▀ top module dark, bottom light
const QChar kLower(0x2584); // ▄ top light, bottom module dark
const QChar kFull(0x2588);  // █ both modules dark
} // namespace

class TstQr : public QObject {
    Q_OBJECT

private slots:
    // --- QrMatrix::encode(): known-vector against qrcodegen reference behavior -------------
    void encodeKnownVectorDimensions() {
        const QrMatrix m = QrMatrix::encode(QStringLiteral("HELLO WORLD"));
        QVERIFY(m.isValid());
        // 11 alphanumeric chars fit QR version 1 at error-correction MEDIUM => a 21x21 grid.
        QCOMPARE(m.size(), 21);
        // Structural: every QR is an odd square of the form 4*version + 17.
        QVERIFY(m.size() % 2 == 1);
        QVERIFY((m.size() - 17) % 4 == 0);
    }

    void encodeFinderPatternsAreSpecExact() {
        const QrMatrix m = QrMatrix::encode(QStringLiteral("HELLO WORLD"));
        QVERIFY(m.isValid());
        const int n = m.size();

        // Top-left finder: dark 7x7 outer ring, light inner ring, dark 3x3 center.
        QVERIFY(m.module(0, 0));
        QVERIFY(m.module(6, 0));
        QVERIFY(m.module(0, 6));
        QVERIFY(m.module(6, 6));
        QVERIFY(!m.module(1, 1));
        QVERIFY(m.module(3, 3));
        // Its light separator (column 7 / row 7 around the finder).
        QVERIFY(!m.module(7, 0));
        QVERIFY(!m.module(0, 7));

        // Top-right finder (columns n-7..n-1).
        QVERIFY(m.module(n - 1, 0));
        QVERIFY(m.module(n - 7, 0));
        QVERIFY(m.module(n - 1, 6));
        // Bottom-left finder (rows n-7..n-1).
        QVERIFY(m.module(0, n - 1));
        QVERIFY(m.module(0, n - 7));
        QVERIFY(m.module(6, n - 1));

        // Timing tracks (row 6 / column 6) alternate dark on even coordinates.
        QVERIFY(m.module(8, 6));
        QVERIFY(!m.module(9, 6));
        QVERIFY(m.module(6, 8));
        QVERIFY(!m.module(6, 9));

        // The always-dark module at (8, 4*version + 9) = (8, n - 8).
        QVERIFY(m.module(8, n - 8));
    }

    void encodeEmptyPayloadIsInvalid() {
        const QrMatrix m = QrMatrix::encode(QString());
        QVERIFY(!m.isValid());
        QCOMPARE(m.size(), 0);
        // Out-of-range reads are light, never a crash.
        QVERIFY(!m.module(0, 0));
        QVERIFY(!m.module(-1, -1));
    }

    // --- QrMatrix::fromModules(): direct grid + bounds -------------------------------------
    void fromModulesRoundTrips() {
        const QrMatrix m = QrMatrix::fromModules({{true, false}, {false, true}});
        QVERIFY(m.isValid());
        QCOMPARE(m.size(), 2);
        QVERIFY(m.module(0, 0));
        QVERIFY(!m.module(1, 0));
        QVERIFY(!m.module(0, 1));
        QVERIFY(m.module(1, 1));
        // Out-of-range is light.
        QVERIFY(!m.module(2, 0));
        QVERIFY(!m.module(0, 2));
    }

    void fromModulesRejectsRagged() {
        const QrMatrix m = QrMatrix::fromModules({{true, false}, {true}});
        QVERIFY(!m.isValid());
    }

    // --- renderHalfBlocks(): the terminal projection ---------------------------------------
    void halfBlocksExactGlyphs() {
        // modules[y][x]: (0,0) dark + (1,1) dark on the diagonal.
        const QrMatrix m = QrMatrix::fromModules({{true, false}, {false, true}});
        const QStringList rows = qr::renderHalfBlocks(m, /*quietZone=*/0);
        QCOMPARE(rows.size(), 1); // two module rows share one character row
        // column 0: top dark, bottom light => upper half; column 1: top light, bottom dark =>
        // lower.
        QString expected;
        expected += kUpper;
        expected += kLower;
        QCOMPARE(rows.first(), expected);
    }

    void halfBlocksFullAndEmptyCells() {
        // Row 0 all dark, row 1 all light => a full-block over a light row in one character cell.
        const QrMatrix m = QrMatrix::fromModules({{true, true}, {false, false}});
        const QStringList rows = qr::renderHalfBlocks(m, /*quietZone=*/0);
        QCOMPARE(rows.size(), 1);
        QString expected;
        expected += kUpper; // top dark, bottom light
        expected += kUpper;
        QCOMPARE(rows.first(), expected);

        const QrMatrix full = QrMatrix::fromModules({{true, true}, {true, true}});
        const QStringList fullRows = qr::renderHalfBlocks(full, /*quietZone=*/0);
        QString bothDark;
        bothDark += kFull;
        bothDark += kFull;
        QCOMPARE(fullRows.first(), bothDark);
    }

    void halfBlocksQuietZoneFrames() {
        const QrMatrix m = QrMatrix::encode(QStringLiteral("HELLO WORLD"));
        QVERIFY(m.isValid());
        const int qz = 4;
        const int dim = m.size() + 2 * qz; // 21 + 8 = 29 modules per side (incl. quiet zone)
        const QStringList rows = qr::renderHalfBlocks(m, qz);
        QCOMPARE(rows.size(), (dim + 1) / 2); // 15 character rows
        for (const QString& row : rows) {
            QCOMPARE(row.size(), dim);
        }
        // The top character row covers padded module-rows 0 and 1 — both quiet zone — so it is a
        // solid light band (all spaces).
        QCOMPARE(rows.first(), QString(dim, QLatin1Char(' ')));
    }

    void halfBlocksInvalidMatrixIsEmpty() {
        QVERIFY(qr::renderHalfBlocks(QrMatrix(), 4).isEmpty());
    }
};

QTEST_APPLESS_MAIN(TstQr)
#include "tst_qr.moc"
