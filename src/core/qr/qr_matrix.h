// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

// [integrations wire v38] The QR module matrix seam shared by BOTH auth surfaces. A QrMatrix is a
// square grid of dark/light modules decoupled from any encoder: QrMatrix::encode() fills it via the
// vendored Nayuki qrcodegen (the only place that touches the upstream files), while fromModules()
// builds one directly (tests + a node-supplied matrix). The GUI Kit.QrCode paints it; the TUI auth
// panel projects it to unicode half-blocks — both from the SAME matrix, so the two front ends
// render an identical, scannable code with nothing protocol-specific baked in.
namespace qr {

class QrMatrix {
public:
    QrMatrix() = default;

    // Encode `payload` as a QR Code (error-correction MEDIUM, minimal fitting version). An empty or
    // un-encodable (too-long) payload yields an invalid matrix (isValid() == false, size() == 0).
    [[nodiscard]] static QrMatrix encode(const QString& payload);

    // Build directly from a row-major grid (modules[y][x]; true = dark). A ragged or empty grid
    // yields an invalid matrix. Used by tests and any node-supplied module grid.
    [[nodiscard]] static QrMatrix fromModules(const QVector<QVector<bool>>& modules);

    [[nodiscard]] bool isValid() const { return m_size > 0; }
    // Module count per side, EXCLUDING the quiet zone (0 when invalid). Always odd (21..177) and of
    // the QR form 4*version + 17 for a real encode().
    [[nodiscard]] int size() const { return m_size; }
    // The module at (x, y): true = dark. Any out-of-range coordinate (including the quiet zone)
    // reads as light (false).
    [[nodiscard]] bool module(int x, int y) const;

private:
    int m_size = 0;
    QVector<bool> m_modules; // m_size * m_size, row-major (index = y * m_size + x)
};

// Project `matrix` to unicode half-block rows for terminal rendering: two vertical modules share
// one character cell (U+2580 ▀ upper, U+2584 ▄ lower, U+2588 █ both, space neither; a DARK module
// is the FILLED half), padded on every side with `quietZone` light modules (QR spec default 4).
// Painting the rows with a dark foreground on a light background is scanner-safe. An invalid matrix
// yields {}.
[[nodiscard]] QStringList renderHalfBlocks(const QrMatrix& matrix, int quietZone = 4);

} // namespace qr
