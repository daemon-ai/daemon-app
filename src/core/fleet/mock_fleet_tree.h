// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "fleet/ifleet_tree.h"
#include "uimodels/variant_list_model.h"

namespace daemonnet {
class MockFleetSource;
}

namespace fleet {

class MockFleetTree : public IFleetTree {
    Q_OBJECT

public:
    // Derives its rows from the MockFleetSource `fleet()` projection (the mock seed);
    // `pause`/`resume` mutate the local copy. `src` may be null (then the tree is empty).
    explicit MockFleetTree(daemonnet::MockFleetSource* src, QObject* parent = nullptr);

    [[nodiscard]] QObject* nodes() const override;

    void pause(const QString& id) override;
    void resume(const QString& id) override;

private:
    void setStatus(const QString& id, const QString& status);
    uimodels::VariantListModel* m_nodes = nullptr;
};

} // namespace fleet
