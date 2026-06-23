#pragma once

#include "fleet/ifleet_tree.h"

#include "uimodels/variant_list_model.h"

namespace fleet {

class MockFleetTree : public IFleetTree {
    Q_OBJECT

public:
    explicit MockFleetTree(QObject* parent = nullptr);

    [[nodiscard]] QObject* nodes() const override;

    void pause(const QString& id) override;
    void resume(const QString& id) override;

private:
    void setStatus(const QString& id, const QString& status);
    uimodels::VariantListModel* m_nodes = nullptr;
};

} // namespace fleet
