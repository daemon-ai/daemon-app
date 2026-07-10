// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <cstdint>
#include <QString>

namespace mirror {

// Mint an RFC 9562 UUIDv7 op-id (spec 09 §6.2): a 48-bit big-endian Unix-millisecond timestamp,
// the 4-bit version nibble (7), 12 bits of randomness, the 2-bit variant (0b10), then 62 more
// random bits, rendered as the canonical lowercase 8-4-4-4-12 string. Time-ordered so a lane's
// FIFO order survives export/debugging, and collision-free without coordination.
[[nodiscard]] QString generateUuidV7();

// Testable overload: mint with an explicit Unix-millisecond timestamp so the time-ordering
// guarantee can be asserted deterministically.
[[nodiscard]] QString generateUuidV7(quint64 unixMs);

} // namespace mirror
