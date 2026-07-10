// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "uuidv7.h"

#include <array>
#include <cstdint>
#include <QDateTime>
#include <QLatin1Char>
#include <QRandomGenerator>

namespace mirror {
namespace {

// Render 16 bytes as the canonical lowercase 8-4-4-4-12 UUID string.
QString render(const std::array<quint8, 16>& b) {
    static const char* kHex = "0123456789abcdef";
    QString out;
    out.reserve(36);
    for (int i = 0; i < 16; ++i) {
        if (i == 4 || i == 6 || i == 8 || i == 10) {
            out.append(QLatin1Char('-'));
        }
        out.append(QLatin1Char(kHex[(b[static_cast<std::size_t>(i)] >> 4) & 0x0F]));
        out.append(QLatin1Char(kHex[b[static_cast<std::size_t>(i)] & 0x0F]));
    }
    return out;
}

} // namespace

QString generateUuidV7(quint64 unixMs) {
    std::array<quint8, 16> b{};

    // Bytes 0-5: 48-bit big-endian Unix-millisecond timestamp (the time-ordering prefix).
    const quint64 ms = unixMs & 0x0000FFFFFFFFFFFFULL;
    b[0] = static_cast<quint8>((ms >> 40) & 0xFF);
    b[1] = static_cast<quint8>((ms >> 32) & 0xFF);
    b[2] = static_cast<quint8>((ms >> 24) & 0xFF);
    b[3] = static_cast<quint8>((ms >> 16) & 0xFF);
    b[4] = static_cast<quint8>((ms >> 8) & 0xFF);
    b[5] = static_cast<quint8>(ms & 0xFF);

    // Bytes 6-15: cryptographically-random tail; then overlay the version + variant fields.
    QRandomGenerator* rng = QRandomGenerator::system();
    for (int i = 6; i < 16; ++i) {
        b[static_cast<std::size_t>(i)] = static_cast<quint8>(rng->bounded(256));
    }
    b[6] = static_cast<quint8>((b[6] & 0x0F) | 0x70); // version 7 in the high nibble
    b[8] = static_cast<quint8>((b[8] & 0x3F) | 0x80); // variant 0b10

    return render(b);
}

QString generateUuidV7() {
    return generateUuidV7(static_cast<quint64>(QDateTime::currentMSecsSinceEpoch()));
}

} // namespace mirror
