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

    // C2 (locked refinement): the ProfileEditor provider picker offers ONLY real providers - Mock
    // is never a default or a user-selectable option (it is reachable only under the mock service
    // flag).
    void pickerProvidersExcludeMock() {
        MockProfileStore s;
        const QVariantList picks = s.pickerProviders();
        QCOMPARE(picks.size(), 4);
        QStringList ids;
        for (const QVariant& v : picks) {
            ids << v.toMap().value(QStringLiteral("id")).toString();
        }
        QVERIFY(ids.contains(QStringLiteral("daemon_api")));
        QVERIFY(ids.contains(QStringLiteral("genai")));
        QVERIFY(ids.contains(QStringLiteral("llama_cpp")));
        QVERIFY(ids.contains(QStringLiteral("mistral_rs")));
        QVERIFY(!ids.contains(QStringLiteral("mock")));
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

    // Persona (SOUL.md) seam: soul() reads the seeded persona (a multi-line
    // SOUL.md-flavored document), and an unknown id resolves to empty.
    void soulReadsSeededPersona() {
        MockProfileStore s;
        const QString persona = s.soul(QStringLiteral("prof-1"));
        QVERIFY(persona.startsWith(QStringLiteral("You are a helpful, concise assistant.")));
        QVERIFY(persona.contains(QLatin1Char('\n')));
        QCOMPARE(s.soul(QStringLiteral("missing")), QString());
    }

    // setSoul() persists the persona (surviving a restart via the on-disk cache)
    // and announces the fresh content via soulChanged(profileId).
    void setSoulPersistsAndSignals() {
        QString id;
        {
            MockProfileStore s;
            QSignalSpy soulSpy(&s, &profiles::IProfileStore::soulChanged);
            id = s.createProfile(QStringLiteral("Persona"));
            s.setSoul(id, QStringLiteral("# Soul\n\nBe kind."));
            QCOMPARE(soulSpy.count(), 1);
            QCOMPARE(soulSpy.last().first().toString(), id);
            QCOMPARE(s.soul(id), QStringLiteral("# Soul\n\nBe kind."));
            // An unknown id is a no-op: no row invented, no signal.
            s.setSoul(QStringLiteral("missing"), QStringLiteral("x"));
            QCOMPARE(soulSpy.count(), 1);
            QVERIFY(s.profile(QStringLiteral("missing")).isEmpty());
        }
        MockProfileStore reborn;
        QCOMPARE(reborn.soul(id), QStringLiteral("# Soul\n\nBe kind."));
    }

    // requestSoul() is inert on the live in-memory store: soul() already answers
    // synchronously, so no signal fires and nothing changes (the daemon store
    // overrides it with a SoulGet fetch that answers via soulChanged).
    void requestSoulIsInert() {
        MockProfileStore s;
        QSignalSpy soulSpy(&s, &profiles::IProfileStore::soulChanged);
        const QString before = s.soul(QStringLiteral("prof-1"));
        s.requestSoul(QStringLiteral("prof-1"));
        QCOMPARE(soulSpy.count(), 0);
        QCOMPARE(s.soul(QStringLiteral("prof-1")), before);
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
