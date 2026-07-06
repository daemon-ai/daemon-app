// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

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

    void approve(const QString& id, bool allowPermanent = false) override;
    void deny(const QString& id, const QString& reason = QString()) override;

private:
    uimodels::VariantListModel* m_pending = nullptr;
};

} // namespace fleet
