// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "transports/mock_persons_service.h"

namespace transports {

// ===== STUB (red) — implemented in the green commit =====

MockPersonsService::MockPersonsService(QObject* parent) : IPersonsService(parent) {}

QVariantList MockPersonsService::persons() const {
    return m_persons;
}

void MockPersonsService::refresh() {}

} // namespace transports
