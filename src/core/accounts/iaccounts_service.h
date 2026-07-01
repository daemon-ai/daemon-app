// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

namespace accounts {

// Accounts/auth seam backing the Accounts manager + the Add-account wizard. The
// mock simulates OAuth round-trips and API-key storage; a daemon adapter later
// maps these to the node's credential endpoints. Connected accounts are exposed
// as a live VariantListModel (rows: id, provider, label, kind, status, detail).
class IAccountsService : public QObject {
    Q_OBJECT
    Q_PROPERTY(QObject* accounts READ accounts CONSTANT)
    // True while an OAuth round-trip is in flight (drives the wizard spinner).
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)

public:
    using QObject::QObject;
    ~IAccountsService() override = default;

    [[nodiscard]] virtual QObject* accounts() const = 0;
    [[nodiscard]] virtual bool busy() const = 0;

    // Providers offered by the Add-account wizard. Each map: id, name, kinds
    // (QStringList of "oauth"/"apikey"), needsBaseUrl (bool).
    [[nodiscard]] Q_INVOKABLE virtual QVariantList availableProviders() const = 0;

    // Add an API-key (or token) account immediately.
    Q_INVOKABLE virtual void addApiKey(const QString& provider, const QString& label,
                                       const QString& key, const QString& baseUrl = {}) = 0;

    // Profile-scoped API key: store the credential under `profileId` (not the active profile), for
    // the ProfileEditor's per-profile credential row. Default routes to the active-profile
    // addApiKey so seams without profile-scoped storage still work; the daemon seam overrides it
    // with a CredentialSet on `profileId`.
    Q_INVOKABLE virtual void addApiKeyForProfile(const QString& profileId, const QString& provider,
                                                 const QString& label, const QString& key,
                                                 const QString& baseUrl = {}) {
        Q_UNUSED(profileId)
        addApiKey(provider, label, key, baseUrl);
    }

    // The credential state for `profileId` as {present: bool, hint: string}, read from the redacted
    // CredentialList. Default = unknown/absent (empty map); the daemon seam overrides it.
    [[nodiscard]] Q_INVOKABLE virtual QVariantMap credentialFor(const QString& profileId) const {
        Q_UNUSED(profileId)
        return {};
    }

    // Begin a simulated OAuth flow; resolves asynchronously to a connected account
    // (oauthCompleted) or an error (oauthFailed).
    Q_INVOKABLE virtual void beginOAuth(const QString& provider) = 0;

    // Rename an existing account's display label.
    Q_INVOKABLE virtual void rename(const QString& accountId, const QString& label) = 0;

    // Re-authenticate an existing account: an OAuth account re-runs the handshake
    // (pending -> connected), an API-key account is simply re-validated. Drives the
    // "Reconnect" affordance when a credential is stale/expired.
    Q_INVOKABLE virtual void reauth(const QString& accountId) = 0;

    Q_INVOKABLE virtual void remove(const QString& accountId) = 0;

signals:
    void busyChanged();
    void oauthCompleted(const QString& accountId);
    void oauthFailed(const QString& provider, const QString& reason);
    // The credential set changed (a CredentialList refresh landed / a key was set/removed), so
    // profile-scoped credential status bindings (ProfileEditor) can re-query credentialFor().
    void credentialsChanged();
};

} // namespace accounts
