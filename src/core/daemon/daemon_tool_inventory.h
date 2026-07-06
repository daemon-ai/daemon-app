// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "tools/itool_inventory.h"

namespace uimodels {
class VariantListModel;
}

namespace daemonapp::daemon {
class ToolRepository;

// [wave2:app-approvals-safety] D2: daemon-backed tool inventory. Projects the ToolRepository's
// decoded ToolList (ToolInfo.enabled + requires, wire v29) into the VariantListModel rows the
// Settings -> Tools surface renders. Read-only — no client toggle exists at v29.
class DaemonToolInventory : public tools::IToolInventory {
    Q_OBJECT

public:
    explicit DaemonToolInventory(ToolRepository* repository, QObject* parent = nullptr);

    [[nodiscard]] QObject* tools() const override;
    [[nodiscard]] int count() const override;
    void refresh() override;

private:
    void rebuild();

    ToolRepository* m_repository = nullptr;
    uimodels::VariantListModel* m_tools = nullptr;
};

} // namespace daemonapp::daemon
