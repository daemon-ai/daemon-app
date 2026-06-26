#pragma once

#include "fleet/iapprovals_inbox.h"
#include "uimodels/variant_list_model.h"

namespace fleet {

class MockApprovalsInbox : public IApprovalsInbox {
    Q_OBJECT

public:
    explicit MockApprovalsInbox(QObject* parent = nullptr);

    [[nodiscard]] QObject* pending() const override;
    [[nodiscard]] int count() const override;

    void approve(const QString& id) override;
    void deny(const QString& id) override;

private:
    uimodels::VariantListModel* m_pending = nullptr;
};

} // namespace fleet
