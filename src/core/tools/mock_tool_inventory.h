// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "tools/itool_inventory.h"

namespace tools {

// [wave2:app-approvals-safety] D2: dev stand-in for the tool inventory. A small canned set (a few
// always-on tools + one credential-gated + one feature-gated) so the Settings -> Tools surface and
// its tests render without a live node. Never grows real behavior — the node is the authority.
class MockToolInventory : public IToolInventory {
    Q_OBJECT

public:
    explicit MockToolInventory(QObject* parent = nullptr);

    [[nodiscard]] QObject* tools() const override;
    [[nodiscard]] int count() const override;
    void refresh() override;

private:
    uimodels::VariantListModel* m_tools = nullptr;
};

} // namespace tools
