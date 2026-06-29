// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QString>

namespace models {

// Human-readable byte size (binary units), e.g. 92'000'000 -> "87.7 MiB". Ported from the q1
// app's Models hub formatter so the GUI/TUI render download + file sizes consistently.
inline QString formatBytes(quint64 bytes) {
    if (bytes == 0) {
        return QStringLiteral("\u2014"); // em dash: unknown / zero
    }
    constexpr double kUnit = 1024.0;
    const char* const suffixes[] = {"B", "KiB", "MiB", "GiB", "TiB"};
    double value = static_cast<double>(bytes);
    int idx = 0;
    while (value >= kUnit && idx < 4) {
        value /= kUnit;
        ++idx;
    }
    return QStringLiteral("%1 %2")
        .arg(value, 0, 'f', idx == 0 ? 0 : (value < 10.0 ? 2 : 1))
        .arg(QLatin1String(suffixes[idx]));
}

// Compact parameter-count label, e.g. 135'000'000 -> "135M", 8'000'000'000 -> "8B". Empty for 0.
inline QString formatParams(quint64 count) {
    if (count == 0) {
        return {};
    }
    const double billions = static_cast<double>(count) / 1e9;
    if (billions >= 1.0) {
        return QStringLiteral("%1B").arg(billions, 0, 'f', billions < 10.0 ? 1 : 0);
    }
    const double millions = static_cast<double>(count) / 1e6;
    if (millions >= 1.0) {
        return QStringLiteral("%1M").arg(millions, 0, 'f', 0);
    }
    return QStringLiteral("%1K").arg(static_cast<double>(count) / 1e3, 0, 'f', 0);
}

} // namespace models
