// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "fleet/isession_roster.h"
#include "uimodels/variant_list_model.h"

namespace daemonnet {
class MockFleetSource;
}

namespace fleet {

class MockSessionRoster : public ISessionRoster {
    Q_OBJECT

public:
    // Derives its rows from the MockFleetSource `sessions()` projection (the mock seed);
    // suspend/resume/close mutate the local copy. `src` may be null (then the roster is empty).
    explicit MockSessionRoster(daemonnet::MockFleetSource* src, QObject* parent = nullptr);

    [[nodiscard]] QObject* sessions() const override;
    [[nodiscard]] int count() const override;

    void suspend(const QString& id) override;
    void resume(const QString& id) override;
    void close(const QString& id) override;

private:
    void setState(const QString& id, const QString& state);
    uimodels::VariantListModel* m_sessions = nullptr;
};

} // namespace fleet
