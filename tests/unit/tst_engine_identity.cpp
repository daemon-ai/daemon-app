// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// [wave2:app-engines] The shared EngineIdentity facade (C3/C4/C5): session/profile -> engine
// {engine, agent, protocol, label}. Native profiles yield engine="Core" with an empty label; a
// foreign profile yields engine="Foreign" + the agent name; labelFor is the single presentation
// mapping GUI + TUI share (and is callable with wire-sourced engine values for app-delegation).

#include "daemon/engine_identity.h"
#include "profiles/iprofile_store.h"
#include "session/isession_settings.h"
#include "uimodels/variant_list_model.h"

#include <QtTest/QtTest>

using daemonapp::daemon::EngineIdentity;

namespace {

// Minimal profile-store stub: fixed rows keyed by id (engine + acpAgent), like the fleet-tree test.
class StubProfileStore : public profiles::IProfileStore {
public:
    explicit StubProfileStore(QObject* parent = nullptr)
        : profiles::IProfileStore(parent), m_model(new uimodels::VariantListModel(this)) {}

    void addProfile(const QString& id, const QString& engine, const QString& acpAgent) {
        QVariantMap row;
        row[QStringLiteral("id")] = id;
        row[QStringLiteral("engine")] = engine;
        row[QStringLiteral("acpAgent")] = acpAgent;
        auto rows = m_model->rows();
        rows.append(row);
        m_model->setRows(rows);
        emit changed();
    }
    void setDefaultId(const QString& id) { m_defaultId = id; }

    [[nodiscard]] QObject* profiles() const override { return m_model; }
    [[nodiscard]] QString defaultProfileId() const override { return m_defaultId; }
    [[nodiscard]] QVariantMap profile(const QString& id) const override {
        const int row = m_model->indexOfId(id);
        return row >= 0 ? m_model->at(row) : QVariantMap{};
    }
    [[nodiscard]] QStringList profileNames() const override { return {}; }
    [[nodiscard]] QVariantList availableSkills() const override { return {}; }
    [[nodiscard]] QVariantList availableTools() const override { return {}; }
    QString createProfile(const QString&) override { return {}; }
    QString cloneProfile(const QString&, const QString&) override { return {}; }
    void updateProfile(const QString&, const QVariantMap&) override {}
    void remove(const QString&) override {}
    void setDefault(const QString&) override {}

private:
    uimodels::VariantListModel* m_model = nullptr;
    QString m_defaultId;
};

// Minimal session-settings stub: a fixed session -> profile mapping via profileFor().
class StubSessionSettings : public session::ISessionSettings {
public:
    using session::ISessionSettings::ISessionSettings;
    QHash<QString, QString> map;

    [[nodiscard]] QString profileFor(const QString& id) const override { return map.value(id); }
    [[nodiscard]] QString sessionId() const override { return {}; }
    void setSessionId(const QString&) override {}
    [[nodiscard]] QString profile() const override { return {}; }
    void setProfile(const QString&) override {}
    [[nodiscard]] QString effort() const override { return {}; }
    void setEffort(const QString&) override {}
    [[nodiscard]] bool fast() const override { return false; }
    void setFast(bool) override {}
    [[nodiscard]] bool verbose() const override { return false; }
    void setVerbose(bool) override {}
    [[nodiscard]] QString approvalMode() const override { return {}; }
    void setApprovalMode(const QString&) override {}
    [[nodiscard]] QStringList effortOptions() const override { return {}; }
    [[nodiscard]] QStringList approvalModeOptions() const override { return {}; }
};

} // namespace

class TestEngineIdentity : public QObject {
    Q_OBJECT

private slots:
    void labelForNativeIsEmpty() {
        EngineIdentity id(nullptr, nullptr, nullptr);
        QVERIFY(id.labelFor(QStringLiteral("Core"), QString(), QString()).isEmpty());
    }

    void labelForForeignProtocols() {
        EngineIdentity id(nullptr, nullptr, nullptr);
        QCOMPARE(
            id.labelFor(QStringLiteral("Foreign"), QStringLiteral("gemini"), QStringLiteral("Acp")),
            QStringLiteral("gemini · ACP"));
        const QString sj = id.labelFor(QStringLiteral("Foreign"), QStringLiteral("claude"),
                                       QStringLiteral("StreamJson"));
        QVERIFY(sj.contains(QStringLiteral("claude")));
        QVERIFY(sj.contains(QStringLiteral("stream-json")));
        // Unknown protocol (catalog not loaded): the agent name alone, never a fabricated protocol.
        QCOMPARE(id.labelFor(QStringLiteral("Foreign"), QStringLiteral("x"), QString()),
                 QStringLiteral("x"));
    }

    void engineForProfileNativeVsForeign() {
        StubProfileStore profiles;
        profiles.addProfile(QStringLiteral("native"), QStringLiteral("Core"), QString());
        profiles.addProfile(QStringLiteral("foreign"), QStringLiteral("Foreign"),
                            QStringLiteral("gemini"));
        EngineIdentity id(&profiles, nullptr, nullptr);

        const QVariantMap nat = id.engineForProfile(QStringLiteral("native"));
        QCOMPARE(nat.value(QStringLiteral("engine")).toString(), QStringLiteral("Core"));
        QVERIFY(nat.value(QStringLiteral("label")).toString().isEmpty());

        const QVariantMap fgn = id.engineForProfile(QStringLiteral("foreign"));
        QCOMPARE(fgn.value(QStringLiteral("engine")).toString(), QStringLiteral("Foreign"));
        QCOMPARE(fgn.value(QStringLiteral("agent")).toString(), QStringLiteral("gemini"));
        // No catalog wired -> protocol unknown -> label is the bare agent name (honest).
        QCOMPARE(fgn.value(QStringLiteral("label")).toString(), QStringLiteral("gemini"));
    }

    void engineForSessionJoinsThroughProfile() {
        StubProfileStore profiles;
        profiles.addProfile(QStringLiteral("p-foreign"), QStringLiteral("Foreign"),
                            QStringLiteral("goose"));
        StubSessionSettings settings;
        settings.map.insert(QStringLiteral("s1"), QStringLiteral("p-foreign"));
        EngineIdentity id(&profiles, &settings, nullptr);

        const QVariantMap info = id.engineForSession(QStringLiteral("s1"));
        QCOMPARE(info.value(QStringLiteral("engine")).toString(), QStringLiteral("Foreign"));
        QCOMPARE(info.value(QStringLiteral("agent")).toString(), QStringLiteral("goose"));
    }

    void engineForSessionFallsBackToDefaultProfile() {
        StubProfileStore profiles;
        profiles.addProfile(QStringLiteral("def"), QStringLiteral("Core"), QString());
        profiles.setDefaultId(QStringLiteral("def"));
        StubSessionSettings settings; // no per-session override
        EngineIdentity id(&profiles, &settings, nullptr);

        const QVariantMap info = id.engineForSession(QStringLiteral("unmapped"));
        QCOMPARE(info.value(QStringLiteral("engine")).toString(), QStringLiteral("Core"));
    }
};

QTEST_MAIN(TestEngineIdentity)
#include "tst_engine_identity.moc"
