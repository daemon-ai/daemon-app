// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// Offscreen tests for the interactive TUI Settings page. They drive the same
// two layers the shell composes:
//   - TuiPageHub: j/k row selection + Space/Enter in-place toggles, asserting
//     the ISettingsStore / IDaemonConfig seams reflect the edit (the exact
//     calls the GUI sections make), and
//   - TuiSettingsEditor: the dialog flows (list pick via PaletteDialog, text
//     prompt via TextPromptDialog) driven through a real offscreen terminal,
//     plus the theme/zen rows routing through the RootWidget live-apply hooks.

#include "appcache/json_cache.h"
#include "config/mock_daemon_config.h"
#include "feedback/mock_feedback.h"
#include "settings/isettings_store.h"
#include "settings_editor.h"
#include "tab_model.h"
#include "tui_page_hub.h"

#include <QEventLoop>
#include <QFile>
#include <QTimer>
#include <QtTest>
#include <Tui/ZEvent.h>
#include <Tui/ZRoot.h>
#include <Tui/ZTerminal.h>
#include <Tui/ZTest.h>

namespace {

// In-memory ISettingsStore (no QSettings I/O), so assertions read exactly what
// the page wrote and nothing leaks into the developer's real config.
class FakeSettingsStore : public settings::ISettingsStore {
public:
    using ISettingsStore::ISettingsStore;

    [[nodiscard]] QVariant value(const QString& key, const QVariant& fallback = {}) const override {
        return m_values.value(key, fallback);
    }
    void setValue(const QString& key, const QVariant& value) override {
        m_values.insert(key, value);
        emit changed(key);
        emit changedAny();
    }

    QHash<QString, QVariant> m_values;
};

int rowIndexOf(const QList<QVariantMap>& rows, const QString& id) {
    for (int i = 0; i < rows.size(); ++i) {
        if (rows.at(i).value(QStringLiteral("id")).toString() == id) {
            return i;
        }
    }
    return -1;
}

// Walk the hub selection from the top to `id` with the real 'j' key handling.
int selectRow(TuiPageHub& hub, const QString& id) {
    const int target = rowIndexOf(hub.pageActionRows(TabModel::Settings), id);
    if (target < 0) {
        return -1;
    }
    for (int i = 0; i < target; ++i) {
        Tui::ZKeyEvent down(Qt::Key_unknown, Qt::NoModifier, QStringLiteral("j"));
        if (!hub.handlePageActionKey(TabModel::Settings, &down)) {
            return -1;
        }
    }
    return target;
}

void settle() {
    QEventLoop loop;
    QTimer::singleShot(20, &loop, &QEventLoop::quit);
    loop.exec();
}

} // namespace

class TestSettingsPage : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        // Isolate the MockDaemonConfig's appcache persistence from the real user
        // data, and start each run from the seeded defaults.
        QStandardPaths::setTestModeEnabled(true);
        QFile::remove(appcache::path(QStringLiteral("daemon_config.json")));
    }

    // j/k moves the ▸ selection across the editable rows, and Space flips the
    // selected toggle through the ISettingsStore seam (the same
    // AppSettings.setValue("notify/turnDone", ...) call the GUI makes).
    void spaceTogglesSettingsStoreRow() {
        FakeSettingsStore store;
        config::MockDaemonConfig config;
        TuiPageHub::Dependencies deps{};
        deps.daemonConfig = &config;
        deps.settings = &store;
        TuiPageHub hub(deps);

        const int target = selectRow(hub, QStringLiteral("notify/turnDone"));
        QVERIFY(target > 0);

        // The rendered page marks the selected row.
        const QString marked = hub.pageMarkdownForKind(TabModel::Settings);
        QVERIFY2(marked.contains(QStringLiteral("▸ Notify when a turn finishes: off")),
                 qPrintable(marked));

        Tui::ZKeyEvent space(Qt::Key_Space, Qt::NoModifier, QStringLiteral(" "));
        QVERIFY(hub.handlePageActionKey(TabModel::Settings, &space));
        QCOMPARE(store.value(QStringLiteral("notify/turnDone"), false).toBool(), true);
        QVERIFY(hub.pageMarkdownForKind(TabModel::Settings)
                    .contains(QStringLiteral("▸ Notify when a turn finishes: on")));

        // Space again restores it (a plain in-place flip, not a one-way latch).
        Tui::ZKeyEvent again(Qt::Key_Space, Qt::NoModifier, QStringLiteral(" "));
        QVERIFY(hub.handlePageActionKey(TabModel::Settings, &again));
        QCOMPARE(store.value(QStringLiteral("notify/turnDone"), false).toBool(), false);
    }

    // Enter on a daemon-config toggle flips it through IDaemonConfig::setValue.
    void enterTogglesDaemonConfigRow() {
        FakeSettingsStore store;
        config::MockDaemonConfig config;
        TuiPageHub::Dependencies deps{};
        deps.daemonConfig = &config;
        deps.settings = &store;
        TuiPageHub hub(deps);

        QVERIFY(selectRow(hub, QStringLiteral("chat/streaming")) > 0);
        const bool before = config.value(QStringLiteral("chat/streaming"), true).toBool();
        Tui::ZKeyEvent enter(Qt::Key_Enter, Qt::NoModifier, QString());
        QVERIFY(hub.handlePageActionKey(TabModel::Settings, &enter));
        QCOMPARE(config.value(QStringLiteral("chat/streaming"), true).toBool(), !before);
    }

    // Enter on an enumerated daemon-config row opens the list pick; filtering +
    // Enter commits the choice through IDaemonConfig::setValue (the same call
    // the GUI's ComboRow makes) and re-renders the page.
    void listPickCommitsDaemonConfigChoice() {
        Tui::ZTerminal terminal{Tui::ZTerminal::OffScreen{90, 28}};
        Tui::ZRoot root;
        terminal.setMainWidget(&root);

        FakeSettingsStore store;
        config::MockDaemonConfig config;
        TuiPageHub::Dependencies deps{};
        deps.daemonConfig = &config;
        deps.settings = &store;
        TuiPageHub hub(deps);

        bool refreshed = false;
        TuiSettingsEditor editor(&hub, &root,
                                 TuiSettingsEditor::Hooks{
                                     {},
                                     {},
                                     [&refreshed] { refreshed = true; },
                                     {},
                                 });

        QVERIFY(selectRow(hub, QStringLiteral("model/effort")) > 0);
        QVERIFY(config.value(QStringLiteral("model/effort")).toString() !=
                QStringLiteral("Thorough"));

        // The hub declines Enter on a choice row; the editor opens the picker.
        Tui::ZKeyEvent enter(Qt::Key_Enter, Qt::NoModifier, QString());
        QVERIFY(!hub.handlePageActionKey(TabModel::Settings, &enter));
        QVERIFY(editor.handleKey(&enter));
        settle();

        // Type to filter the palette to "Thorough", then Enter commits it.
        Tui::ZTest::sendText(&terminal, QStringLiteral("Thorough"), Qt::NoModifier);
        Tui::ZTest::sendKey(&terminal, Qt::Key_Enter, Qt::NoModifier);
        settle();

        QCOMPARE(config.value(QStringLiteral("model/effort")).toString(),
                 QStringLiteral("Thorough"));
        QVERIFY(refreshed);
        QVERIFY(hub.pageMarkdownForKind(TabModel::Settings)
                    .contains(QStringLiteral("Reasoning effort: **Thorough**")));
    }

    // Render honesty (#1): safety posture is node-owned. There is no editable
    // approval-policy row (that lives in the per-session session settings), and
    // the filesystem/network rows are read-only "managed" rows: selecting one and
    // pressing Space/Enter neither toggles nor opens an editor, and nothing is
    // written to the config seam. The page annotates them as node-controlled.
    void safetyRowsAreReadOnlyNodeControlled() {
        Tui::ZTerminal terminal{Tui::ZTerminal::OffScreen{90, 28}};
        Tui::ZRoot root;
        terminal.setMainWidget(&root);

        FakeSettingsStore store;
        config::MockDaemonConfig config;
        TuiPageHub::Dependencies deps{};
        deps.daemonConfig = &config;
        deps.settings = &store;
        TuiPageHub hub(deps);
        TuiSettingsEditor editor(&hub, &root, TuiSettingsEditor::Hooks{});

        const QList<QVariantMap> rows = hub.pageActionRows(TabModel::Settings);
        // The cosmetic global approval-policy duplicate is gone.
        QCOMPARE(rowIndexOf(rows, QStringLiteral("safety/approvalPolicy")), -1);
        // Sandbox + network survive as non-editable, node-controlled rows.
        for (const QString& id :
             {QStringLiteral("safety/sandbox"), QStringLiteral("safety/allowNetwork")}) {
            const int idx = rowIndexOf(rows, id);
            QVERIFY(idx >= 0);
            QCOMPARE(rows.at(idx).value(QStringLiteral("type")).toString(),
                     QStringLiteral("managed"));
        }

        // Selecting the sandbox row and pressing Space/Enter is inert: neither the
        // hub nor the editor claims the key, and no config write happens.
        QVERIFY(selectRow(hub, QStringLiteral("safety/sandbox")) > 0);
        Tui::ZKeyEvent space(Qt::Key_Space, Qt::NoModifier, QStringLiteral(" "));
        QVERIFY(!hub.handlePageActionKey(TabModel::Settings, &space));
        Tui::ZKeyEvent enter(Qt::Key_Enter, Qt::NoModifier, QString());
        QVERIFY(!hub.handlePageActionKey(TabModel::Settings, &enter));
        QVERIFY(!editor.handleKey(&enter));
        QVERIFY(!config.value(QStringLiteral("safety/sandbox")).isValid());

        // The page reads honestly: node-controlled value + the per-session pointer.
        const QString md = hub.pageMarkdownForKind(TabModel::Settings);
        QVERIFY2(md.contains(QStringLiteral("Filesystem access: _node-controlled_")),
                 qPrintable(md));
        QVERIFY2(md.contains(QStringLiteral("enforced by the node")), qPrintable(md));
    }

    // Enter on a free-text row opens the prompt seeded with the current value;
    // OK commits the edited text through IDaemonConfig::setValue.
    void textPromptCommitsDaemonConfigText() {
        Tui::ZTerminal terminal{Tui::ZTerminal::OffScreen{90, 28}};
        Tui::ZRoot root;
        terminal.setMainWidget(&root);

        FakeSettingsStore store;
        config::MockDaemonConfig config;
        TuiPageHub::Dependencies deps{};
        deps.daemonConfig = &config;
        deps.settings = &store;
        TuiPageHub hub(deps);
        TuiSettingsEditor editor(&hub, &root, TuiSettingsEditor::Hooks{});

        QVERIFY(selectRow(hub, QStringLiteral("workspace/root")) > 0);
        const QString seed = config.value(QStringLiteral("workspace/root")).toString();

        Tui::ZKeyEvent enter(Qt::Key_Enter, Qt::NoModifier, QString());
        QVERIFY(editor.handleKey(&enter));
        settle();

        // The input holds the seed with the caret at the end; append and accept.
        Tui::ZTest::sendText(&terminal, QStringLiteral("/sub"), Qt::NoModifier);
        Tui::ZTest::sendKey(&terminal, Qt::Key_Enter, Qt::NoModifier);
        settle();

        QCOMPARE(config.value(QStringLiteral("workspace/root")).toString(),
                 seed + QStringLiteral("/sub"));

        // Numeric rows commit ints (GUI parity: parseInt(text) || 0).
        const QList<QVariantMap> rows = hub.pageActionRows(TabModel::Settings);
        const int contextRow = rowIndexOf(rows, QStringLiteral("memory/contextWindow"));
        QVERIFY(contextRow > 0);
        QVERIFY(hub.applySettingsValue(rows.at(contextRow), QStringLiteral("4096").toInt()));
        QCOMPARE(config.value(QStringLiteral("memory/contextWindow")).toInt(), 4096);
    }

    // The theme row routes through the applyTheme hook (RootWidget's live F8
    // machinery, which owns persistence) rather than a seam write; the zen row
    // routes through the toggle hook the same way.
    void themeAndZenRowsUseLiveApplyHooks() {
        Tui::ZTerminal terminal{Tui::ZTerminal::OffScreen{90, 28}};
        Tui::ZRoot root;
        terminal.setMainWidget(&root);

        FakeSettingsStore store;
        config::MockDaemonConfig config;
        TuiPageHub::Dependencies deps{};
        deps.daemonConfig = &config;
        deps.settings = &store;
        TuiPageHub hub(deps);

        theme::ThemeName applied = theme::ThemeName::Light;
        bool zenToggled = false;
        TuiSettingsEditor editor(&hub, &root,
                                 TuiSettingsEditor::Hooks{
                                     [&applied](theme::ThemeName name) { applied = name; },
                                     [&zenToggled] { zenToggled = true; },
                                     {},
                                     {},
                                 });

        // Theme is the first row; Enter opens the picker, pick Midnight.
        QCOMPARE(selectRow(hub, QStringLiteral("ui/theme")), 0);
        Tui::ZKeyEvent enter(Qt::Key_Enter, Qt::NoModifier, QString());
        QVERIFY(editor.handleKey(&enter));
        settle();
        Tui::ZTest::sendText(&terminal, QStringLiteral("Midnight"), Qt::NoModifier);
        Tui::ZTest::sendKey(&terminal, Qt::Key_Enter, Qt::NoModifier);
        settle();
        QCOMPARE(applied, theme::ThemeName::Midnight);
        // No seam write: the hook path owns ui/theme persistence.
        QVERIFY(!store.m_values.contains(QStringLiteral("ui/theme")));

        // Zen: the hub declines the toggle (live path), the editor's hook flips it.
        QVERIFY(selectRow(hub, QStringLiteral("ui/distractionFree")) > 0);
        Tui::ZKeyEvent space(Qt::Key_Space, Qt::NoModifier, QStringLiteral(" "));
        QVERIFY(!hub.handlePageActionKey(TabModel::Settings, &space));
        QVERIFY(editor.handleKey(&space));
        QVERIFY(zenToggled);
    }

    // The language row persists the shared ui/language pref through the settings
    // store (the GUI's UiSettings.language key) when picked.
    void languagePickPersistsSharedPref() {
        Tui::ZTerminal terminal{Tui::ZTerminal::OffScreen{90, 28}};
        Tui::ZRoot root;
        terminal.setMainWidget(&root);

        FakeSettingsStore store;
        config::MockDaemonConfig config;
        TuiPageHub::Dependencies deps{};
        deps.daemonConfig = &config;
        deps.settings = &store;
        TuiPageHub hub(deps);
        TuiSettingsEditor editor(&hub, &root, TuiSettingsEditor::Hooks{});

        QVERIFY(selectRow(hub, QStringLiteral("ui/language")) > 0);
        Tui::ZKeyEvent enter(Qt::Key_Enter, Qt::NoModifier, QString());
        QVERIFY(editor.handleKey(&enter));
        settle();
        Tui::ZTest::sendText(&terminal, QStringLiteral("English"), Qt::NoModifier);
        Tui::ZTest::sendKey(&terminal, Qt::Key_Enter, Qt::NoModifier);
        settle();
        QCOMPARE(store.value(QStringLiteral("ui/language")).toString(), QStringLiteral("en"));
    }

    // Guardrail for the Wave-2 merge: the editable inventory mirrors exactly the
    // GUI sections that make real seam-backed writes today. A row added here
    // must land in the GUI (or this list) consciously - and vice versa.
    void editableRowInventoryMatchesGuiSections() {
        FakeSettingsStore store;
        config::MockDaemonConfig config;
        TuiPageHub::Dependencies deps{};
        deps.daemonConfig = &config;
        deps.settings = &store;
        feedback::MockFeedback feedback;
        deps.feedback = &feedback;
        TuiPageHub hub(deps);

        QStringList ids;
        for (const QVariantMap& row : hub.pageActionRows(TabModel::Settings)) {
            ids << row.value(QStringLiteral("id")).toString();
        }
        const QStringList expected{
            QStringLiteral("ui/theme"),
            QStringLiteral("ui/language"),
            QStringLiteral("ui/distractionFree"),
            QStringLiteral("notify/gates"),
            QStringLiteral("notify/turnDone"),
            QStringLiteral("conn/managedLocalDaemon"),
            QStringLiteral("conn/managedDaemonShutdownOnExit"),
            QStringLiteral("model/effort"),
            QStringLiteral("model/fast"),
            QStringLiteral("chat/streaming"),
            QStringLiteral("chat/sendOnEnter"),
            QStringLiteral("chat/showTokenCounts"),
            // Safety posture is node-owned: approval policy lives in the
            // per-session session settings, and sandbox/network are shown
            // read-only (managed rows), so no editable safety/approvalPolicy row.
            QStringLiteral("safety/sandbox"),
            QStringLiteral("safety/allowNetwork"),
            // [wave2:app-delegation] F7/DEL-7: read-only node delegation guardrail ceilings.
            QStringLiteral("safety/delegationDepth"),
            QStringLiteral("safety/delegationFanout"),
            // [phase H] agent-authored-profile ceilings (advisory caps display).
            QStringLiteral("safety/maxComposedProfiles"),
            QStringLiteral("safety/maxEphemeralPerSession"),
            QStringLiteral("memory/contextWindow"),
            QStringLiteral("memory/autoCompact"),
            QStringLiteral("memory/persistMemory"),
            QStringLiteral("workspace/root"),
            QStringLiteral("workspace/followGitignore"),
            QStringLiteral("voice/enabled"),
            QStringLiteral("voice/model"),
            QStringLiteral("advanced/logLevel"),
            QStringLiteral("advanced/telemetry"),
            QStringLiteral("advanced/crashReports"),
            QStringLiteral("advanced/experimentalTools"),
        };
        QCOMPARE(ids, expected);
    }
};

QTEST_GUILESS_MAIN(TestSettingsPage)
#include "tst_settings_page.moc"
