// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "transports/itransport_registry.h"

namespace transports {

// Inert mock: advertises the two adapter families that exist today (Matrix, the internal Rooms
// loopback) for the "Add channel" picker, and reports no configured instances. Replaced by a daemon
// adapter decoding the node's `transport_adapters` / `transport_instances` ops.
class MockTransportRegistry : public ITransportRegistry {
    Q_OBJECT

public:
    using ITransportRegistry::ITransportRegistry;

    [[nodiscard]] QVariantList availableAdapters() const override;
    [[nodiscard]] QVariantList instances() const override;
};

} // namespace transports
