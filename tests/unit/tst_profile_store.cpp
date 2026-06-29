// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "cache_test_support.h"
#include "profiles/mock_profile_store.h"
#include "uimodels/variant_list_model.h"

#include <QtTest/QtTest>

using profiles::MockProfileStore;
using uimodels::VariantListModel;

// Guards the profiles mock: seeded profiles + catalogs, create, field/skill
// updates, default switching (single default), and removal default-reassignment.
class TestProfileStore : public QObject {
    Q_OBJECT

    static VariantListModel* asModel(QObject* o) { return qobject_cast<VariantListModel*>(o); }

private slots:
    void init() { resetMockCache(); }

    void seededAndCatalogs() {
        MockProfileStore s;
        QCOMPARE(asModel(s.profiles())->count(), 3);
        QCOMPARE(s.defaultProfileId(), QStringLiteral("prof-1"));
        QVERIFY(!s.availableSkills().isEmpty());
        QVERIFY(!s.availableTools().isEmpty());
    }

    void profileNamesMirrorsProfiles() {
        MockProfileStore s;
        const QStringList names = s.profileNames();
        QCOMPARE(names.size(), asModel(s.profiles())->count());
        for (const QVariantMap& row : asModel(s.profiles())->rows()) {
            QVERIFY(names.contains(row.value(QStringLiteral("name")).toString()));
        }
    }

    void createAndUpdate() {
        MockProfileStore s;
        const QString id = s.createProfile(QStringLiteral("Tester"));
        QVERIFY(!id.isEmpty());
        s.updateProfile(id, {{QStringLiteral("model"), QStringLiteral("mistral-7b-instruct")},
                             {QStringLiteral("skills"), QStringList{QStringLiteral("translate")}}});
        const QVariantMap p = s.profile(id);
        QCOMPARE(p.value(QStringLiteral("model")).toString(),
                 QStringLiteral("mistral-7b-instruct"));
        QCOMPARE(p.value(QStringLiteral("skills")).toStringList(),
                 QStringList{QStringLiteral("translate")});
    }

    void setDefaultIsExclusive() {
        MockProfileStore s;
        s.setDefault(QStringLiteral("prof-2"));
        QCOMPARE(s.defaultProfileId(), QStringLiteral("prof-2"));
        QVERIFY(s.profile(QStringLiteral("prof-2")).value(QStringLiteral("isDefault")).toBool());
        QVERIFY(!s.profile(QStringLiteral("prof-1")).value(QStringLiteral("isDefault")).toBool());
    }

    void removeReassignsDefault() {
        MockProfileStore s;
        s.remove(QStringLiteral("prof-1"));
        QVERIFY(s.profile(QStringLiteral("prof-1")).isEmpty());
        QVERIFY(!s.defaultProfileId().isEmpty());
        QVERIFY(s.defaultProfileId() != QStringLiteral("prof-1"));
    }

    // Tier 3: a created profile + default switch survive across construction via
    // the last-known on-disk cache.
    void profilesPersistAcrossRestart() {
        QString id;
        {
            MockProfileStore s;
            id = s.createProfile(QStringLiteral("Persisted"));
            s.setDefault(id);
        }

        MockProfileStore reborn;
        QVERIFY(!reborn.profile(id).isEmpty());
        QCOMPARE(reborn.defaultProfileId(), id);
        QVERIFY(reborn.profileNames().contains(QStringLiteral("Persisted")));
    }
};

QTEST_MAIN(TestProfileStore)
#include "tst_profile_store.moc"
