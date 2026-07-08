// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "accounts/iaccounts_service.h"

#include <QHash>

namespace uimodels {
class VariantListModel;
}
namespace daemonapp::daemon {
class CredentialRepository;
class ProfileRepository;
} // namespace daemonapp::daemon

namespace accounts {

// Daemon-backed accounts seam (CON-4): API keys map to the node's profile-keyed credential store
// over the wire. addApiKey -> CredentialSet on the active profile; the redacted CredentialList
// projects into the account rows; remove -> CredentialRemove. OAuth is not in this slice (it
// reports oauthFailed). Credentials are stored under the node's ACTIVE profile (discovered via
// ProfileList), so inference - which acquires for that same profile - picks the key up; there is no
// dependence on a launch-configured profile name (the host serves credentials per-profile).
//
// Lives under src/core/daemon (not accounts/) because it depends on the daemon repositories, which
// link the accounts interface - keeping it here avoids a library cycle.
class DaemonAccountsService : public IAccountsService {
    Q_OBJECT

public:
    DaemonAccountsService(daemonapp::daemon::CredentialRepository* credentials,
                          daemonapp::daemon::ProfileRepository* profiles,
                          QObject* parent = nullptr);

    [[nodiscard]] QObject* accounts() const override;
    [[nodiscard]] bool busy() const override { return false; }

    [[nodiscard]] QVariantList availableProviders() const override;

    void addApiKey(const QString& provider, const QString& label, const QString& key,
                   const QString& baseUrl = {}) override;
    void addApiKeyForProfile(const QString& profileId, const QString& provider,
                             const QString& label, const QString& key,
                             const QString& baseUrl = {}) override;
    [[nodiscard]] QVariantMap credentialFor(const QString& profileId) const override;
    void beginOAuth(const QString& provider) override;
    void rename(const QString& accountId, const QString& label) override;
    void reauth(const QString& accountId) override;
    void remove(const QString& accountId) override;

private:
    // Rebuild the account rows from the repository's redacted credential list, enriched with the
    // provider the user chose when adding the key (the wire returns profile/present/hint/label but
    // no provider). [acct-mgmt] wire v35: the label now comes from the wire
    // (credential-info.label), no longer from the client-local map.
    void rebuildAccounts();
    // The profile a new credential is stored under: the node's active default (ProfileList), or a
    // sensible fallback when the list is unknown.
    [[nodiscard]] QString credentialProfile() const;

    daemonapp::daemon::CredentialRepository* m_credentials = nullptr;
    daemonapp::daemon::ProfileRepository* m_profiles = nullptr;
    uimodels::VariantListModel* m_accounts = nullptr;

    // [acct-mgmt] wire v35: the label moved to the wire (credential-info.label); this map now only
    // carries the client-chosen provider (the redacted CredentialList reports no provider).
    QHash<QString, QString> m_providerFor; // profile -> chosen provider, for display enrichment
};

} // namespace accounts
