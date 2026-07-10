// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "qr/qr_matrix.h"

namespace qr {

// RED stub: the seam exists so the tests compile against the final contract, but no encoding or
// projection is implemented yet — every path yields an invalid/empty result, so tst_qr fails until
// the GREEN pass wires the vendored Nayuki qrcodegen and the half-block projection.
QrMatrix QrMatrix::encode(const QString& /*payload*/) {
    return {};
}

QrMatrix QrMatrix::fromModules(const QVector<QVector<bool>>& /*modules*/) {
    return {};
}

bool QrMatrix::module(int /*x*/, int /*y*/) const {
    return false;
}

QStringList renderHalfBlocks(const QrMatrix& /*matrix*/, int /*quietZone*/) {
    return {};
}

} // namespace qr
