// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <limits>
#include <QtGlobal>

namespace daemon_app {

// Narrow a Qt signed 64-bit size/index (qsizetype) to int at an int-based API boundary
// (model rows, cursor offsets, geometry indices, indexOf/size-1 results, ...). Debug builds assert
// the value fits in int; release builds compile down to a plain static_cast (zero overhead). This
// makes every such conversion explicit and turns a future genuinely out-of-range value into a
// caught assertion instead of a silent truncation, while keeping bugprone-narrowing-conversions on.
//
// The bound is the full int range: negative results (e.g. indexOf() == -1, or size() - 1 on an
// empty container) are valid int values and must pass; only true truncation (|v| > INT_MAX) trips.
[[nodiscard]] constexpr int to_int(qsizetype v) noexcept {
    Q_ASSERT(v >= static_cast<qsizetype>(std::numeric_limits<int>::min()) &&
             v <= static_cast<qsizetype>(std::numeric_limits<int>::max()));
    return static_cast<int>(v);
}

} // namespace daemon_app
