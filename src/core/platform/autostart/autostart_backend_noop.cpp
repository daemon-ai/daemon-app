// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// wasm/mobile: no user-login autostart concept the app could manage. State is
// Unsupported and both settings surfaces hide the row.

#include "platform/autostart/autostart_backend.h"

namespace autostart::backend {

Status query(const QString& /*program*/) {
    return {State::Unsupported, Detail::None, {}};
}

Status apply(const QString& /*program*/, bool /*enable*/) {
    return {State::Unsupported, Detail::None, {}};
}

bool repair(const QString& /*program*/) {
    return false;
}

void reregister(const QString& /*program*/) {}

void openApprovalSettings() {}

} // namespace autostart::backend
