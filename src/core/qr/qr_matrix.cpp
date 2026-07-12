// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "qr/qr_matrix.h"

#include <QByteArray>
#include <qrcodegen.hpp>

namespace qr {

namespace {
// The unicode half-block glyphs (a dark module is the FILLED half).
constexpr char16_t kUpperHalf = u'\u2580'; // ▀ top module dark, bottom light
constexpr char16_t kLowerHalf = u'\u2584'; // ▄ top light, bottom module dark
constexpr char16_t kFullBlock = u'\u2588'; // █ both modules dark
} // namespace

QrMatrix QrMatrix::encode(const QString& payload) {
    if (payload.isEmpty()) {
        return {};
    }
    // qrcodegen throws data_too_long when the payload cannot fit the largest version; fail closed
    // to an invalid matrix so callers degrade (show the copyable payload / redirect) rather than
    // crash.
    try {
        const QByteArray utf8 = payload.toUtf8();
        const qrcodegen::QrCode code =
            qrcodegen::QrCode::encodeText(utf8.constData(), qrcodegen::QrCode::Ecc::MEDIUM);
        QrMatrix matrix;
        matrix.m_size = code.getSize();
        matrix.m_modules.resize(static_cast<qsizetype>(matrix.m_size) * matrix.m_size);
        for (int y = 0; y < matrix.m_size; ++y) {
            for (int x = 0; x < matrix.m_size; ++x) {
                matrix.m_modules[(y * matrix.m_size) + x] = code.getModule(x, y);
            }
        }
        return matrix;
    } catch (...) {
        return {};
    }
}

QrMatrix QrMatrix::fromModules(const QVector<QVector<bool>>& modules) {
    const int n = static_cast<int>(modules.size());
    if (n == 0) {
        return {};
    }
    for (const QVector<bool>& row : modules) {
        if (static_cast<int>(row.size()) != n) {
            return {}; // ragged / non-square grid is not a valid module matrix
        }
    }
    QrMatrix matrix;
    matrix.m_size = n;
    matrix.m_modules.resize(static_cast<qsizetype>(n) * n);
    for (int y = 0; y < n; ++y) {
        for (int x = 0; x < n; ++x) {
            matrix.m_modules[(y * n) + x] = modules[y][x];
        }
    }
    return matrix;
}

bool QrMatrix::module(int x, int y) const {
    if (x < 0 || y < 0 || x >= m_size || y >= m_size) {
        return false; // outside the matrix (incl. the quiet zone) reads as light
    }
    return m_modules[(y * m_size) + x];
}

QStringList renderHalfBlocks(const QrMatrix& matrix, int quietZone) {
    if (!matrix.isValid()) {
        return {};
    }
    const int n = matrix.size();
    const int qz = quietZone > 0 ? quietZone : 0;
    const int dim = n + (2 * qz); // modules per side, including the quiet zone

    // A module in padded (quiet-zone-inclusive) coordinates: dark only inside the real matrix.
    const auto darkAt = [&](int px, int py) -> bool {
        const int mx = px - qz;
        const int my = py - qz;
        return matrix.module(mx, my);
    };

    QStringList rows;
    rows.reserve((dim + 1) / 2);
    for (int py = 0; py < dim; py += 2) {
        QString row;
        row.reserve(dim);
        for (int px = 0; px < dim; ++px) {
            const bool top = darkAt(px, py);
            const bool bottom = (py + 1 < dim) && darkAt(px, py + 1);
            if (top && bottom) {
                row.append(QChar(kFullBlock));
            } else if (top) {
                row.append(QChar(kUpperHalf));
            } else if (bottom) {
                row.append(QChar(kLowerHalf));
            } else {
                row.append(QLatin1Char(' '));
            }
        }
        rows.append(row);
    }
    return rows;
}

} // namespace qr
