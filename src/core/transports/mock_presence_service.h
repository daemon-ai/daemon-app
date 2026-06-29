// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "transports/ipresence_service.h"

namespace transports {

// Inert mock: reports every instance offline / unknown until a daemon adapter feeds real state.
class MockPresenceService : public IPresenceService {
    Q_OBJECT

public:
    using IPresenceService::IPresenceService;

    [[nodiscard]] QString connectionState(const QString& transport) const override;
    [[nodiscard]] QString presence(const QString& transport) const override;
};

} // namespace transports
