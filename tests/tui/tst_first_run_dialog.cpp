// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// TUI first-run dialog parity tests (Wave 1). They drive a real FirstRunDialog bound to the
// shared FirstRunModel inside an off-screen Tui terminal and lock the two W3 parity fixes:
//   - agent naming: the inference phase shows a name input prefilled from the chosen provider's
//     label (lowercased, until the user edits it), and Finish passes it to applyInferenceChoice
//     so the wizard mints a NAMED agent exactly like the GUI;
//   - the hoisted key gate (FIX 4): Finish stays blocked, with the model's visible reason line,
//     until the entered key is PROVEN by a non-empty authenticated ProviderModels listing.

#include "accounts/mock_accounts_service.h"
#include "cache_test_support.h"
#include "connection/mock_connection_service.h"
#include "dialogs/first_run_dialog.h"
#include "firstrun/first_run_model.h"
#include "models/iprovider_catalog.h"
#include "profiles/mock_profile_store.h"
#include "settings/qt_settings_store.h"

#include <QStandardPaths>
#include <QTemporaryFile>
#include <QtTest>
#include <QVariantList>
#include <QVariantMap>
#include <Tui/ZButton.h>
#include <Tui/ZImage.h>
#include <Tui/ZInputBox.h>
#include <Tui/ZLabel.h>
#include <Tui/ZListView.h>
#include <Tui/ZTerminal.h>
#include <Tui/ZWidget.h>

namespace {

// Flatten the off-screen terminal frame to one string per row (cluster-aware) so we can assert
// on the visible glyphs, like the other TUI widget tests.
QString frameText(Tui::ZTerminal& terminal) {
    const Tui::ZImage img = terminal.grabCurrentImage();
    QString out;
    for (int y = 0; y < img.height(); ++y) {
        for (int x = 0; x < img.width(); ++x) {
            int left = 0;
            int right = 0;
            const QString cell = img.peekText(x, y, &left, &right);
            out += (left == x && !cell.isEmpty()) ? cell : QStringLiteral(" ");
        }
        out += QLatin1Char('\n');
    }
    return out;
}

void settle(int ms = 20) {
    QEventLoop loop;
    QTimer::singleShot(ms, &loop, &QEventLoop::quit);
    loop.exec();
}

// A controllable provider catalog: the test decides the descriptors and how many models each
// provider's (authenticated) LIST resolves with - resolveModels(id, n) is "a ProviderModels
// reply landed", the seam both the dialog's model list and the shared key gate are driven off.
class FakeProviderCatalog : public models::IProviderCatalog {
public:
    QVariantList descriptors;
    QHash<QString, QVariantList> offered;

    static QVariantMap descriptor(const QString& id, const QString& name, bool requiresKey) {
        QVariantMap d;
        d[QStringLiteral("id")] = id;
        d[QStringLiteral("name")] = name;
        d[QStringLiteral("requiresKey")] = requiresKey;
        return d;
    }
    [[nodiscard]] QVariantList providers() const override { return descriptors; }
    [[nodiscard]] QVariantMap descriptorFor(const QString& providerId) const override {
        for (const QVariant& v : descriptors) {
            const QVariantMap d = v.toMap();
            if (d.value(QStringLiteral("id")).toString() == providerId) {
                return d;
            }
        }
        return {};
    }
    void refreshModels(const QString& providerId, const QString& = QString(),
                       const QString& = QString()) override {
        emit offeredModelsChanged(providerId);
    }
    [[nodiscard]] QVariantList offeredModels(const QString& providerId) const override {
        return offered.value(providerId);
    }
    void resolveModels(const QString& providerId, int count) {
        QVariantList rows;
        for (int i = 0; i < count; ++i) {
            QVariantMap row;
            row[QStringLiteral("id")] = QStringLiteral("m-%1").arg(i);
            row[QStringLiteral("name")] = QStringLiteral("model-%1").arg(i);
            row[QStringLiteral("kind")] = QStringLiteral("model");
            rows.append(row);
        }
        offered.insert(providerId, rows);
        emit offeredModelsChanged(providerId);
    }
};

} // namespace

class TestFirstRunDialog : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() { QStandardPaths::setTestModeEnabled(true); }

    void init() {
        // Pristine mock seeds + an un-completed setup for every case.
        resetMockCache();
        settings::QtSettingsStore s;
        s.setSetupComplete(false);
    }

    // The full inference-phase parity flow: name prefill from the provider label, the key gate
    // blocking Finish with its visible reason line until an authenticated LIST proves the key,
    // and Finish handing the (edited) name to applyInferenceChoice so a NAMED agent is minted.
    void inferencePhaseNamesAgentAndBlocksOnKeyGate() {
        settings::QtSettingsStore settings;
        connection::MockConnectionService conn;
        FakeProviderCatalog catalog;
        catalog.descriptors = {
            FakeProviderCatalog::descriptor(QStringLiteral("acme"), QStringLiteral("Acme"), true)};
        profiles::MockProfileStore profileStore;
        // Fresh-node state (the A2 wizard gate only opens while the ACTIVE profile is
        // unconfigured): blank the mock's active profile like on a first boot.
        QVariantMap blank;
        blank[QStringLiteral("model")] = QString();
        profileStore.updateProfile(profileStore.defaultProfileId(), blank);
        accounts::MockAccountsService accountsSvc;
        firstrun::FirstRunModel model(&settings, &conn, nullptr, &profileStore, &accountsSvc,
                                      &catalog);
        model.begin();

        Tui::ZTerminal terminal{Tui::ZTerminal::OffScreen{90, 30}};
        Tui::ZWidget root;
        terminal.setMainWidget(&root);
        auto* dialog = new FirstRunDialog(&model, &conn, &settings, &catalog, nullptr,
                                          QStringLiteral("/tmp/x"), &root);

        auto* agentName = dialog->findChild<Tui::ZInputBox*>(QStringLiteral("firstRunAgentName"));
        auto* key = dialog->findChild<Tui::ZInputBox*>(QStringLiteral("setupKey"));
        auto* gateMsg = dialog->findChild<Tui::ZLabel*>(QStringLiteral("setupKeyGateMsg"));
        auto* modelList = dialog->findChild<Tui::ZListView*>(QStringLiteral("setupModelList"));
        auto* primary = dialog->findChild<Tui::ZButton*>(QStringLiteral("firstRunPrimary"));
        QVERIFY(agentName != nullptr);
        QVERIFY(key != nullptr);
        QVERIFY(gateMsg != nullptr);
        QVERIFY(modelList != nullptr);
        QVERIFY(primary != nullptr);

        // The first provider populate already seeded the agent name from the catalog label,
        // lowercased - exactly how the GUI wizard prefills agentNameField.
        QCOMPARE(agentName->text(), QStringLiteral("acme"));

        // Drive the shared model to the inference phase over the mock connection seam.
        QTemporaryFile socketStandIn;
        QVERIFY(socketStandIn.open());
        conn.connectTo(QStringLiteral("local"), socketStandIn.fileName());
        QVERIFY(
            QTest::qWaitFor([&] { return model.phase() == QStringLiteral("inference"); }, 3000));
        settle();

        // The inference frame shows the name row (parity with the GUI wizard) and Finish.
        const QString frame = frameText(terminal);
        QVERIFY2(frame.contains(QStringLiteral("Agent name")), qPrintable(frame));
        QVERIFY2(frame.contains(QStringLiteral("acme")), qPrintable(frame));
        QCOMPARE(primary->text(), QStringLiteral("Finish"));

        // Nothing proven/selected yet: Finish is blocked, with no reason line (the gate only
        // explains itself after a FAILED validation, like the GUI).
        QVERIFY(!primary->isEnabled());
        QVERIFY(!gateMsg->isVisible());

        // Type a key and land an authenticated LIST that comes back EMPTY: the key gate blocks
        // Finish and surfaces the shared model's actionable reason line.
        key->setText(QStringLiteral("sk-wrong"));
        catalog.resolveModels(QStringLiteral("acme"), 0);
        settle();
        QVERIFY(!primary->isEnabled());
        QVERIFY(gateMsg->isVisible());
        QVERIFY(gateMsg->text().contains(QStringLiteral("Acme")));
        QVERIFY2(frameText(terminal).contains(QStringLiteral("Couldn't verify")),
                 qPrintable(frameText(terminal)));

        // A corrected key + a non-empty authenticated LIST passes the gate and clears the
        // reason; picking a concrete model then satisfies Finish (name is prefilled non-empty).
        key->setText(QStringLiteral("sk-right"));
        catalog.resolveModels(QStringLiteral("acme"), 2);
        QVERIFY(!gateMsg->isVisible());
        QVERIFY(!primary->isEnabled()); // still no model selected
        modelList->enterPressed(0);     // the selection-only model pick
        QVERIFY(primary->isEnabled());

        // Clearing the agent name blocks Finish (GUI parity: the name is required); an edited
        // name is the user's own and is what the commit hands to the model.
        agentName->setText(QString());
        QVERIFY(!primary->isEnabled());
        agentName->setText(QStringLiteral("scout"));
        QVERIFY(primary->isEnabled());

        // Finish: applyInferenceChoice persists the spec under the chosen NAME (one full-spec
        // create + select + placeholder delete on the node's reflections) and completes setup.
        const QString placeholder = profileStore.defaultProfileId();
        primary->clicked();
        QVERIFY(QTest::qWaitFor([&] { return model.phase() == QStringLiteral("done"); }, 3000));
        const QString newDefault = profileStore.defaultProfileId();
        QVERIFY(!newDefault.isEmpty());
        QVERIFY(newDefault != placeholder);
        QCOMPARE(profileStore.profile(newDefault).value(QStringLiteral("name")).toString(),
                 QStringLiteral("scout"));
        QVERIFY(profileStore.profile(placeholder).isEmpty()); // the placeholder was dropped
        QVERIFY(settings.setupComplete());
        settle(); // let DeleteOnClose flush before the terminal tears down
    }

    // Provider switching re-seeds the prefill only while the user has not edited the name - the
    // exact ownership rule the GUI wizard applies to agentNameField.
    void agentNameReseedsUntilUserEdits() {
        settings::QtSettingsStore settings;
        connection::MockConnectionService conn;
        FakeProviderCatalog catalog;
        catalog.descriptors = {
            FakeProviderCatalog::descriptor(QStringLiteral("acme"), QStringLiteral("Acme"), true),
            FakeProviderCatalog::descriptor(QStringLiteral("beta"), QStringLiteral("Beta"), true)};
        firstrun::FirstRunModel model(&settings, &conn, nullptr, nullptr, nullptr, &catalog);
        model.begin();

        Tui::ZTerminal terminal{Tui::ZTerminal::OffScreen{90, 30}};
        Tui::ZWidget root;
        terminal.setMainWidget(&root);
        auto* dialog = new FirstRunDialog(&model, &conn, &settings, &catalog, nullptr,
                                          QStringLiteral("/tmp/x"), &root);

        auto* agentName = dialog->findChild<Tui::ZInputBox*>(QStringLiteral("firstRunAgentName"));
        auto* providerList =
            dialog->findChild<Tui::ZListView*>(QStringLiteral("setupProviderList"));
        QVERIFY(agentName != nullptr);
        QVERIFY(providerList != nullptr);

        QCOMPARE(agentName->text(), QStringLiteral("acme")); // seeded from row 0
        providerList->enterPressed(1);                       // switch provider: re-seed
        QCOMPARE(agentName->text(), QStringLiteral("beta"));

        agentName->setText(QStringLiteral("my agent")); // the user takes ownership
        providerList->enterPressed(0);                  // provider changes stop re-seeding
        QCOMPARE(agentName->text(), QStringLiteral("my agent"));
    }
};

QTEST_MAIN(TestFirstRunDialog)
#include "tst_first_run_dialog.moc"
