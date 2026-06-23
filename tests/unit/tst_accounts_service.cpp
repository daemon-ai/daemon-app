#include "accounts/mock_accounts_service.h"
#include "uimodels/variant_list_model.h"

#include "cache_test_support.h"

#include <QSignalSpy>
#include <QtTest/QtTest>

using accounts::MockAccountsService;
using uimodels::VariantListModel;

// Guards the accounts mock: seeded accounts, API-key add (masked), the simulated
// OAuth round-trip (pending -> connected + busy toggles), and removal.
class TestAccountsService : public QObject {
    Q_OBJECT

    static VariantListModel* asModel(QObject* o) { return qobject_cast<VariantListModel*>(o); }

private slots:
    void init() { resetMockCache(); }

    void seededAndProviders()
    {
        MockAccountsService a;
        QCOMPARE(asModel(a.accounts())->count(), 2);
        QVERIFY(!a.availableProviders().isEmpty());
    }

    void addApiKeyMasksAndStores()
    {
        MockAccountsService a;
        const int before = asModel(a.accounts())->count();
        a.addApiKey(QStringLiteral("openrouter"), QStringLiteral("My key"),
                    QStringLiteral("secret-abcd"), QString());
        QCOMPARE(asModel(a.accounts())->count(), before + 1);
        const QVariantMap row = asModel(a.accounts())->rows().last();
        QCOMPARE(row.value(QStringLiteral("label")).toString(), QStringLiteral("My key"));
        QVERIFY(row.value(QStringLiteral("detail")).toString().contains(QStringLiteral("abcd")));
        QVERIFY(!row.value(QStringLiteral("detail")).toString().contains(QStringLiteral("secret")));
    }

    void oauthRoundTrip()
    {
        MockAccountsService a;
        QSignalSpy busy(&a, &accounts::IAccountsService::busyChanged);
        QSignalSpy done(&a, &accounts::IAccountsService::oauthCompleted);

        a.beginOAuth(QStringLiteral("google"));
        QVERIFY(a.busy());
        // A pending row is inserted immediately.
        QCOMPARE(asModel(a.accounts())->rows().last().value(QStringLiteral("status")).toString(),
                 QStringLiteral("pending"));

        QVERIFY(QTest::qWaitFor([&] { return done.count() == 1; }, 4000));
        QVERIFY(!a.busy());
        QCOMPARE(asModel(a.accounts())->rows().last().value(QStringLiteral("status")).toString(),
                 QStringLiteral("connected"));
        QVERIFY(busy.count() >= 2); // on + off
    }

    void removeDrops()
    {
        MockAccountsService a;
        a.remove(QStringLiteral("acc-1"));
        QVERIFY(asModel(a.accounts())->indexOfId(QStringLiteral("acc-1")) < 0);
    }

    void renameUpdatesLabel()
    {
        MockAccountsService a;
        a.rename(QStringLiteral("acc-2"), QStringLiteral("Work OpenAI"));
        const int row = asModel(a.accounts())->indexOfId(QStringLiteral("acc-2"));
        QVERIFY(row >= 0);
        QCOMPARE(asModel(a.accounts())->at(row).value(QStringLiteral("label")).toString(),
                 QStringLiteral("Work OpenAI"));
    }

    void reauthApiKeyIsInstant()
    {
        MockAccountsService a;
        // acc-2 is an API-key account; reauth resolves synchronously to connected.
        a.reauth(QStringLiteral("acc-2"));
        QVERIFY(!a.busy());
        const int row = asModel(a.accounts())->indexOfId(QStringLiteral("acc-2"));
        QCOMPARE(asModel(a.accounts())->at(row).value(QStringLiteral("status")).toString(),
                 QStringLiteral("connected"));
    }

    void reauthOAuthRoundTrips()
    {
        MockAccountsService a;
        QSignalSpy done(&a, &accounts::IAccountsService::oauthCompleted);
        // acc-1 is an OAuth account; reauth flips to pending then back to connected.
        a.reauth(QStringLiteral("acc-1"));
        QVERIFY(a.busy());
        const int row = asModel(a.accounts())->indexOfId(QStringLiteral("acc-1"));
        QCOMPARE(asModel(a.accounts())->at(row).value(QStringLiteral("status")).toString(),
                 QStringLiteral("pending"));
        QVERIFY(QTest::qWaitFor([&] { return done.count() == 1; }, 4000));
        QVERIFY(!a.busy());
        QCOMPARE(asModel(a.accounts())->at(row).value(QStringLiteral("status")).toString(),
                 QStringLiteral("connected"));
    }

    // Tier 3: an added account survives across construction via the last-known
    // on-disk cache, and a removed seeded account stays removed.
    void accountsPersistAcrossRestart()
    {
        {
            MockAccountsService a;
            a.addApiKey(QStringLiteral("openrouter"), QStringLiteral("Persisted key"),
                        QStringLiteral("secret-wxyz"), QString());
            a.remove(QStringLiteral("acc-1"));
        }

        MockAccountsService reborn;
        auto* rows = asModel(reborn.accounts());
        QVERIFY(rows->indexOfId(QStringLiteral("acc-1")) < 0);
        bool found = false;
        for (const QVariantMap& r : rows->rows()) {
            if (r.value(QStringLiteral("label")).toString() == QStringLiteral("Persisted key")) {
                found = true;
            }
        }
        QVERIFY(found);
    }
};

QTEST_MAIN(TestAccountsService)
#include "tst_accounts_service.moc"
