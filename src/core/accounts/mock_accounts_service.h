#pragma once

#include "accounts/iaccounts_service.h"
#include "uimodels/variant_list_model.h"

namespace accounts {

// In-memory IAccountsService: seeded with a couple of connected accounts and a
// simulated OAuth handshake (a short timer) so the wizard is fully interactive.
class MockAccountsService : public IAccountsService {
    Q_OBJECT

public:
    explicit MockAccountsService(QObject* parent = nullptr);

    [[nodiscard]] QObject* accounts() const override;
    [[nodiscard]] bool busy() const override { return m_busy; }

    [[nodiscard]] QVariantList availableProviders() const override;

    void addApiKey(const QString& provider, const QString& label, const QString& key,
                   const QString& baseUrl) override;
    void beginOAuth(const QString& provider) override;
    void rename(const QString& accountId, const QString& label) override;
    void reauth(const QString& accountId) override;
    void remove(const QString& accountId) override;

private:
    [[nodiscard]] QString providerName(const QString& providerId) const;
    // Persist the account rows + next-id to the last-known on-disk cache.
    void save() const;

    uimodels::VariantListModel* m_accounts = nullptr;
    bool m_busy = false;
    int m_nextId = 1;
};

} // namespace accounts
