#include "accounts/mock_accounts_service.h"

#include "appcache/json_cache.h"

#include <QTimer>

namespace accounts {
namespace {

const QString kCacheFile = QStringLiteral("accounts.json");


QVariantMap mkAccount(const QString& id, const QString& provider, const QString& label,
                      const QString& kind, const QString& status, const QString& detail)
{
    QVariantMap m;
    m[QStringLiteral("id")] = id;
    m[QStringLiteral("provider")] = provider;
    m[QStringLiteral("label")] = label;
    m[QStringLiteral("kind")] = kind;     // "oauth" | "apikey"
    m[QStringLiteral("status")] = status; // "connected" | "pending" | "error"
    m[QStringLiteral("detail")] = detail;
    return m;
}

} // namespace

MockAccountsService::MockAccountsService(QObject* parent)
    : IAccountsService(parent)
    , m_accounts(new uimodels::VariantListModel(this))
{
    m_accounts->upsert(mkAccount(QStringLiteral("acc-1"), QStringLiteral("anthropic"),
                                 QStringLiteral("Anthropic"), QStringLiteral("oauth"),
                                 QStringLiteral("connected"), QStringLiteral("jane@example.com")));
    m_accounts->upsert(mkAccount(QStringLiteral("acc-2"), QStringLiteral("openai"),
                                 QStringLiteral("OpenAI"), QStringLiteral("apikey"),
                                 QStringLiteral("connected"), QStringLiteral("sk-•••• 4f2a")));
    m_nextId = 3;

    // Restore the last-known snapshot over the seed (no-op on first run). Any row
    // left mid-handshake (pending) by a crash is downgraded to error, since there
    // is no live timer to resolve it after a restart.
    const QJsonObject cached = appcache::loadObject(kCacheFile);
    if (cached.contains(QStringLiteral("rows"))) {
        QList<QVariantMap> rows = appcache::rowsFromJson(cached.value(QStringLiteral("rows")).toArray());
        for (QVariantMap& r : rows) {
            if (r.value(QStringLiteral("status")).toString() == QLatin1String("pending")) {
                r[QStringLiteral("status")] = QStringLiteral("error");
                r[QStringLiteral("detail")] = QStringLiteral("Re-authentication interrupted");
            }
        }
        m_accounts->setRows(rows);
        m_nextId = cached.value(QStringLiteral("nextId")).toInt(m_nextId);
    }
}

void MockAccountsService::save() const
{
    QJsonObject obj;
    obj[QStringLiteral("rows")] = appcache::rowsToJson(m_accounts->rows());
    obj[QStringLiteral("nextId")] = m_nextId;
    appcache::saveObject(kCacheFile, obj);
}

QObject* MockAccountsService::accounts() const { return m_accounts; }

QString MockAccountsService::providerName(const QString& providerId) const
{
    for (const QVariant& v : availableProviders()) {
        const QVariantMap m = v.toMap();
        if (m.value(QStringLiteral("id")).toString() == providerId) {
            return m.value(QStringLiteral("name")).toString();
        }
    }
    return providerId;
}

QVariantList MockAccountsService::availableProviders() const
{
    const auto mkp = [](const QString& id, const QString& name, const QStringList& kinds,
                        bool needsBaseUrl) {
        QVariantMap m;
        m[QStringLiteral("id")] = id;
        m[QStringLiteral("name")] = name;
        m[QStringLiteral("kinds")] = kinds;
        m[QStringLiteral("needsBaseUrl")] = needsBaseUrl;
        return QVariant(m);
    };
    return {
        mkp(QStringLiteral("anthropic"), QStringLiteral("Anthropic"),
            { QStringLiteral("oauth"), QStringLiteral("apikey") }, false),
        mkp(QStringLiteral("openai"), QStringLiteral("OpenAI"), { QStringLiteral("apikey") }, false),
        mkp(QStringLiteral("openrouter"), QStringLiteral("OpenRouter"),
            { QStringLiteral("apikey") }, false),
        mkp(QStringLiteral("google"), QStringLiteral("Google"),
            { QStringLiteral("oauth"), QStringLiteral("apikey") }, false),
        mkp(QStringLiteral("azure"), QStringLiteral("Azure OpenAI"), { QStringLiteral("apikey") },
            true),
        mkp(QStringLiteral("custom"), QStringLiteral("Custom (OpenAI-compatible)"),
            { QStringLiteral("apikey") }, true),
    };
}

void MockAccountsService::addApiKey(const QString& provider, const QString& label,
                                    const QString& key, const QString& baseUrl)
{
    Q_UNUSED(baseUrl);
    const QString id = QStringLiteral("acc-%1").arg(m_nextId++);
    const QString masked = key.size() > 4
        ? QStringLiteral("sk-•••• %1").arg(key.right(4))
        : QStringLiteral("•••• stored");
    m_accounts->upsert(mkAccount(id, provider,
                                 label.isEmpty() ? providerName(provider) : label,
                                 QStringLiteral("apikey"), QStringLiteral("connected"), masked));
    save();
}

void MockAccountsService::beginOAuth(const QString& provider)
{
    if (m_busy) {
        return;
    }
    m_busy = true;
    emit busyChanged();

    // Insert a pending row immediately so the list reflects the in-flight attempt.
    const QString id = QStringLiteral("acc-%1").arg(m_nextId++);
    m_accounts->upsert(mkAccount(id, provider, providerName(provider), QStringLiteral("oauth"),
                                 QStringLiteral("pending"),
                                 QStringLiteral("Waiting for browser…")));

    QTimer::singleShot(1200, this, [this, id, provider] {
        const int row = m_accounts->indexOfId(id);
        if (row >= 0) {
            QVariantMap acc = m_accounts->at(row);
            acc[QStringLiteral("status")] = QStringLiteral("connected");
            acc[QStringLiteral("detail")] = QStringLiteral("you@%1").arg(provider);
            m_accounts->upsert(acc);
        }
        save();
        m_busy = false;
        emit busyChanged();
        emit oauthCompleted(id);
    });
}

void MockAccountsService::rename(const QString& accountId, const QString& label)
{
    const int row = m_accounts->indexOfId(accountId);
    if (row < 0 || label.isEmpty()) {
        return;
    }
    QVariantMap acc = m_accounts->at(row);
    if (acc.value(QStringLiteral("label")).toString() == label) {
        return;
    }
    acc[QStringLiteral("label")] = label;
    m_accounts->upsert(acc);
    save();
}

void MockAccountsService::reauth(const QString& accountId)
{
    const int row = m_accounts->indexOfId(accountId);
    if (row < 0) {
        return;
    }
    QVariantMap acc = m_accounts->at(row);

    // API-key accounts re-validate instantly; nothing to round-trip.
    if (acc.value(QStringLiteral("kind")).toString() != QLatin1String("oauth")) {
        acc[QStringLiteral("status")] = QStringLiteral("connected");
        m_accounts->upsert(acc);
        save();
        return;
    }

    if (m_busy) {
        return;
    }
    m_busy = true;
    emit busyChanged();

    // Flip to pending, then resolve back to connected after the simulated handshake.
    acc[QStringLiteral("status")] = QStringLiteral("pending");
    acc[QStringLiteral("detail")] = QStringLiteral("Waiting for browser…");
    m_accounts->upsert(acc);

    const QString provider = acc.value(QStringLiteral("provider")).toString();
    QTimer::singleShot(1200, this, [this, accountId, provider] {
        const int r = m_accounts->indexOfId(accountId);
        if (r >= 0) {
            QVariantMap a = m_accounts->at(r);
            a[QStringLiteral("status")] = QStringLiteral("connected");
            a[QStringLiteral("detail")] = QStringLiteral("you@%1").arg(provider);
            m_accounts->upsert(a);
        }
        save();
        m_busy = false;
        emit busyChanged();
        emit oauthCompleted(accountId);
    });
}

void MockAccountsService::remove(const QString& accountId)
{
    m_accounts->removeById(accountId);
    save();
}

} // namespace accounts
