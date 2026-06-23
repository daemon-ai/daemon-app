#pragma once

#include "fleet/isession_roster.h"

#include "uimodels/variant_list_model.h"

namespace fleet {

class MockSessionRoster : public ISessionRoster {
    Q_OBJECT

public:
    explicit MockSessionRoster(QObject* parent = nullptr);

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
