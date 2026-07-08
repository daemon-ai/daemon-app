// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemon/daemon_accounts_service.h"

#include "daemon/repositories.h"
#include "uimodels/variant_list_model.h"

namespace accounts {
namespace {

QVariantMap mkAccount(const QString& id, const QString& provider, const QString& label,
                      const QString& status, const QString& detail) {
    QVariantMap m;
    m[QStringLiteral("id")] = id;
    m[QStringLiteral("provider")] = provider;
    m[QStringLiteral("label")] = label;
    m[QStringLiteral("kind")] = QStringLiteral("apikey");
    m[QStringLiteral("status")] = status; // "connected" | "error"
    m[QStringLiteral("detail")] = detail;
    return m;
}

} // namespace

DaemonAccountsService::DaemonAccountsService(daemonapp::daemon::CredentialRepository* credentials,
                                             daemonapp::daemon::ProfileRepository* profiles,
                                             QObject* parent)
    : IAccountsService(parent), m_credentials(credentials), m_profiles(profiles),
      m_accounts(new uimodels::VariantListModel(this)) {
    if (m_credentials != nullptr) {
        connect(m_credentials, &daemonapp::daemon::CredentialRepository::listRefreshed, this,
                &DaemonAccountsService::rebuildAccounts);
    }
}

QObject* DaemonAccountsService::accounts() const {
    return m_accounts;
}

QString DaemonAccountsService::credentialProfile() const {
    const QString active = m_profiles != nullptr ? m_profiles->activeProfileId() : QString();
    return active.isEmpty() ? QStringLiteral("default") : active;
}

QVariantList DaemonAccountsService::availableProviders() const {
    const auto mkp = [](const QString& id, const QString& name, bool needsBaseUrl) {
        QVariantMap m;
        m[QStringLiteral("id")] = id;
        m[QStringLiteral("name")] = name;
        // API-key only in this slice; interactive OAuth (CON-5) is a later phase.
        m[QStringLiteral("kinds")] = QStringList{QStringLiteral("apikey")};
        m[QStringLiteral("needsBaseUrl")] = needsBaseUrl;
        return QVariant(m);
    };
    return {
        mkp(QStringLiteral("anthropic"), QStringLiteral("Anthropic"), false),
        mkp(QStringLiteral("openai"), QStringLiteral("OpenAI"), false),
        mkp(QStringLiteral("openrouter"), QStringLiteral("OpenRouter"), false),
        mkp(QStringLiteral("google"), QStringLiteral("Google"), false),
        mkp(QStringLiteral("groq"), QStringLiteral("Groq"), false),
        mkp(QStringLiteral("custom"), QStringLiteral("Custom (OpenAI-compatible)"), true),
    };
}

void DaemonAccountsService::addApiKey(const QString& provider, const QString& label,
                                      const QString& key, const QString& baseUrl) {
    Q_UNUSED(baseUrl)
    if (m_credentials == nullptr || key.isEmpty()) {
        return;
    }
    const QString profile = credentialProfile();
    // Remember the provider for display: the redacted CredentialList only returns the profile + a
    // masked hint, not which provider the key is for.
    m_providerFor.insert(profile, provider);
    m_credentials->setCredential(profile, key);
    // [acct-mgmt] wire v35: persist the user's chosen label node-side (the node owns it now); the
    // account row then renders credential-info.label after the refetch.
    if (!label.isEmpty()) {
        m_credentials->setCredentialLabel(profile, /*hasLabel=*/true, label);
    }
}

void DaemonAccountsService::addApiKeyForProfile(const QString& profileId, const QString& provider,
                                                const QString& label, const QString& key,
                                                const QString& baseUrl) {
    Q_UNUSED(baseUrl)
    if (m_credentials == nullptr || profileId.isEmpty() || key.isEmpty()) {
        return;
    }
    // Store the credential under the edited profile (not the active one), so an inactive agent's
    // key is set where inference will look it up. Leaving the profile's credential_ref empty means
    // the node defaults it to the profile id.
    m_providerFor.insert(profileId, provider);
    m_credentials->setCredential(profileId, key);
    // [acct-mgmt] wire v35: persist the chosen label node-side.
    if (!label.isEmpty()) {
        m_credentials->setCredentialLabel(profileId, /*hasLabel=*/true, label);
    }
}

QVariantMap DaemonAccountsService::credentialFor(const QString& profileId) const {
    if (m_credentials == nullptr || profileId.isEmpty()) {
        return {};
    }
    for (const daemonapp::daemon::DecodedCredentialInfo& ci : m_credentials->credentials()) {
        if (ci.profile == profileId) {
            QVariantMap m;
            m[QStringLiteral("present")] = ci.present;
            m[QStringLiteral("hint")] = ci.hint;
            return m;
        }
    }
    QVariantMap m;
    m[QStringLiteral("present")] = false;
    return m;
}

void DaemonAccountsService::beginOAuth(const QString& provider) {
    // Interactive provider OAuth (CON-5) is not part of this phase.
    emit oauthFailed(provider, tr("OAuth sign-in is not available yet; add an API key instead."));
}

void DaemonAccountsService::rename(const QString& accountId, const QString& label) {
    if (m_credentials == nullptr) {
        return;
    }
    // [acct-mgmt] wire v35: the node owns the label. Persist it over the wire (CredentialSetLabel;
    // an empty label clears it to null); on Ok the repo re-fetches and rebuildAccounts renders the
    // wire label. No client-local mutation — never optimistic.
    m_credentials->setCredentialLabel(accountId, /*hasLabel=*/!label.isEmpty(), label);
}

void DaemonAccountsService::reauth(const QString& accountId) {
    Q_UNUSED(accountId)
    // A stored API key is valid until replaced; re-listing reflects current state.
    if (m_credentials != nullptr) {
        m_credentials->refreshList();
    }
}

void DaemonAccountsService::remove(const QString& accountId) {
    if (m_credentials != nullptr) {
        m_credentials->removeCredential(accountId); // accountId is the credential profile
    }
    m_providerFor.remove(accountId);
}

void DaemonAccountsService::rebuildAccounts() {
    if (m_credentials == nullptr) {
        return;
    }
    QList<QVariantMap> rows;
    for (const daemonapp::daemon::DecodedCredentialInfo& ci : m_credentials->credentials()) {
        // Provider is the client-chosen enrichment (default to the profile id when unknown, e.g. a
        // credential set by another client). [acct-mgmt] wire v35: the label is the node-persisted
        // credential-info.label; fall back to the provider for display when the node reported none.
        const QString provider = m_providerFor.value(ci.profile, ci.profile);
        const QString label = ci.label.isEmpty() ? provider : ci.label;
        rows.append(mkAccount(ci.profile, provider, label,
                              ci.present ? QStringLiteral("connected") : QStringLiteral("error"),
                              ci.hint));
    }
    m_accounts->setRows(rows);
    // Let profile-scoped credential-status bindings (ProfileEditor) re-query credentialFor().
    emit credentialsChanged();
}

} // namespace accounts
