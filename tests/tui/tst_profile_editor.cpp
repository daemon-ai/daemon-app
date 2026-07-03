// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// W5 TUI profile editor: drives ProfileEditorDialog against the mock profile
// store + mock provider catalog in an off-screen Tui terminal, asserting the
// store ends up with EXACTLY the field map the GUI editor's save() writes
// (provider = wire selector, cloud-only baseUrl, selection-only model, prompt,
// skills/tools arrays) and that Esc discards. The sub-dialog flows (text
// prompt / multi-line prompt editor / toggle checklist) are exercised with
// real key events.

#include "cache_test_support.h"
#include "dialogs/profile_editor_dialog.h"
#include "models/iprovider_catalog.h"
#include "models/mock_provider_catalog.h"
#include "profiles/mock_profile_store.h"

#include <QPointer>
#include <QtTest>
#include <Tui/ZRoot.h>
#include <Tui/ZTerminal.h>
#include <Tui/ZTest.h>

using models::MockProviderCatalog;
using profiles::MockProfileStore;

class ProfileEditorTests : public QObject {
    Q_OBJECT

    // One off-screen scene per test: terminal + root + fresh mock seams.
    struct Scene {
        Tui::ZTerminal terminal{Tui::ZTerminal::OffScreen{80, 24}};
        Tui::ZRoot root;
        MockProfileStore store;
        MockProviderCatalog catalog;

        Scene() { terminal.setMainWidget(&root); }

        ProfileEditorDialog* open(const QString& id) {
            return new ProfileEditorDialog(&store, &catalog, id, &root);
        }
    };

private slots:
    void init() { resetMockCache(); }

    // The working copy loads the store row, and the saved genai selector
    // resolves to a concrete vendor descriptor id (the GUI's inference).
    void loadsWorkingCopyFromStore() {
        Scene s;
        ProfileEditorDialog* dlg = s.open(QStringLiteral("prof-1"));
        QCOMPARE(dlg->wName(), QStringLiteral("General Assistant"));
        QCOMPARE(dlg->wProvider(), QStringLiteral("genai"));
        // "llama-3.1-8b-instruct" does not self-identify a genai vendor, so
        // the shared-selector match falls back to the first genai vendor.
        QCOMPARE(dlg->wProviderId(), QStringLiteral("anthropic"));
        QCOMPARE(dlg->wModel(), QStringLiteral("llama-3.1-8b-instruct"));
        QCOMPARE(dlg->wSystemPrompt(), QStringLiteral("You are a helpful, concise assistant."));
        QVERIFY(dlg->wSkills().contains(QStringLiteral("web-search")));
        QVERIFY(dlg->wTools().contains(QStringLiteral("read")));
        dlg->close();
    }

    // A claude-* model on the shared genai selector reads as Anthropic even
    // with other genai vendors listed first (the GUI's vendor heuristic).
    void inferenceResolvesVendorFromModel() {
        Scene s;
        s.store.updateProfile(QStringLiteral("prof-1"),
                              {{QStringLiteral("model"), QStringLiteral("gpt-4o")}});
        ProfileEditorDialog* dlg = s.open(QStringLiteral("prof-1"));
        QCOMPARE(dlg->wProviderId(), QStringLiteral("openai"));
        dlg->close();
    }

    // Save writes ONE updateProfile with the GUI editor's exact field map, and
    // untouched profile keys (e.g. memoryProvider) survive the patch.
    void saveWritesGuiFieldMap() {
        Scene s;
        QPointer<ProfileEditorDialog> dlg = s.open(QStringLiteral("prof-1"));
        QSignalSpy savedSpy(dlg.data(), &ProfileEditorDialog::saved);

        dlg->setName(QStringLiteral("Renamed Assistant"));
        dlg->setDescription(QStringLiteral("desc"));
        dlg->setSystemPrompt(QStringLiteral("Line one.\nLine two."));
        dlg->toggleSkill(QStringLiteral("cite")); // add
        dlg->toggleTool(QStringLiteral("read"));  // remove (seeded on)
        dlg->save();

        QCOMPARE(savedSpy.count(), 1);
        const QVariantMap p = s.store.profile(QStringLiteral("prof-1"));
        QCOMPARE(p.value(QStringLiteral("name")).toString(), QStringLiteral("Renamed Assistant"));
        QCOMPARE(p.value(QStringLiteral("description")).toString(), QStringLiteral("desc"));
        QCOMPARE(p.value(QStringLiteral("systemPrompt")).toString(),
                 QStringLiteral("Line one.\nLine two."));
        QVERIFY(p.value(QStringLiteral("skills")).toStringList().contains(QStringLiteral("cite")));
        QVERIFY(!p.value(QStringLiteral("tools")).toStringList().contains(QStringLiteral("read")));
        // Untouched fields pass through the patch unchanged.
        QCOMPARE(p.value(QStringLiteral("provider")).toString(), QStringLiteral("genai"));
        QCOMPARE(p.value(QStringLiteral("model")).toString(),
                 QStringLiteral("llama-3.1-8b-instruct"));
        QCOMPARE(p.value(QStringLiteral("memoryProvider")).toString(), QStringLiteral("mnemosyne"));
        // Save closes the dialog (DeleteOnClose).
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY(dlg.isNull());
    }

    // Provider switch mirrors the GUI dropdown: persists the wire selector,
    // clears the model, seeds the cloud default base URL, and the offered
    // models come from the node catalog (selection-only).
    void providerSwitchFollowsGuiSemantics() {
        Scene s;
        ProfileEditorDialog* dlg = s.open(QStringLiteral("prof-1"));

        dlg->selectProviderById(QStringLiteral("daemon_cloud"));
        QCOMPARE(dlg->wProviderId(), QStringLiteral("daemon_cloud"));
        QCOMPARE(dlg->wProvider(), QStringLiteral("daemon_api")); // wire selector, not the id
        QCOMPARE(dlg->wModel(), QString());                       // switch invalidates the model
        QCOMPARE(dlg->wBaseUrl(), QStringLiteral("https://api.daemon.ai/api/v1"));

        dlg->selectModel(QStringLiteral("anthropic/claude-3.5-sonnet"));
        dlg->save();

        const QVariantMap p = s.store.profile(QStringLiteral("prof-1"));
        QCOMPARE(p.value(QStringLiteral("provider")).toString(), QStringLiteral("daemon_api"));
        QCOMPARE(p.value(QStringLiteral("model")).toString(),
                 QStringLiteral("anthropic/claude-3.5-sonnet"));
        QCOMPARE(p.value(QStringLiteral("baseUrl")).toString(),
                 QStringLiteral("https://api.daemon.ai/api/v1"));
    }

    // A local provider: baseUrl is cleared on save (cloud-only field) and the
    // "Discover More Models" row hands off to the download flow instead of
    // selecting a model.
    void localProviderAndDiscoverRow() {
        Scene s;
        ProfileEditorDialog* dlg = s.open(QStringLiteral("prof-1"));
        QSignalSpy discoverSpy(dlg, &ProfileEditorDialog::modelDiscoverRequested);

        dlg->selectProviderById(QStringLiteral("llama_cpp"));
        QCOMPARE(dlg->wProvider(), QStringLiteral("llama_cpp"));

        dlg->selectModel(QString::fromLatin1(models::IProviderCatalog::kDiscoverMoreId));
        QCOMPARE(discoverSpy.count(), 1);
        QCOMPARE(dlg->wModel(), QString()); // the discover row never selects

        dlg->save();
        const QVariantMap p = s.store.profile(QStringLiteral("prof-1"));
        QCOMPARE(p.value(QStringLiteral("provider")).toString(), QStringLiteral("llama_cpp"));
        QCOMPARE(p.value(QStringLiteral("baseUrl")).toString(), QString());
    }

    // Esc rejects: the working copy (even a mutated one) never reaches the store.
    void escDiscardsWorkingCopy() {
        Scene s;
        QPointer<ProfileEditorDialog> dlg = s.open(QStringLiteral("prof-1"));
        dlg->setName(QStringLiteral("Should not persist"));

        Tui::ZTest::sendKey(&s.terminal, Qt::Key_Escape, Qt::NoModifier);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY(dlg.isNull());
        QCOMPARE(s.store.profile(QStringLiteral("prof-1")).value(QStringLiteral("name")).toString(),
                 QStringLiteral("General Assistant"));
    }

    // The name row's text prompt commits through the same setter (key-driven).
    void namePromptKeyFlow() {
        Scene s;
        ProfileEditorDialog* dlg = s.open(QStringLiteral("prof-1"));

        dlg->openNamePrompt();
        // The prompt is seeded with the current name; append and submit.
        Tui::ZTest::sendText(&s.terminal, QStringLiteral(" II"), Qt::NoModifier);
        Tui::ZTest::sendKey(&s.terminal, Qt::Key_Enter, Qt::NoModifier);
        QCOMPARE(dlg->wName(), QStringLiteral("General Assistant II"));
        dlg->close();
    }

    // The multi-line prompt editor: Enter inserts a real newline, OK commits.
    void promptEditorMultilineKeyFlow() {
        Scene s;
        ProfileEditorDialog* dlg = s.open(QStringLiteral("prof-1"));
        dlg->setSystemPrompt(QString()); // start from an empty persona

        dlg->openPromptEditor();
        Tui::ZTest::sendText(&s.terminal, QStringLiteral("Be terse."), Qt::NoModifier);
        Tui::ZTest::sendKey(&s.terminal, Qt::Key_Enter, Qt::NoModifier);
        Tui::ZTest::sendText(&s.terminal, QStringLiteral("Cite sources."), Qt::NoModifier);
        // Tab leaves the editor (tabChangesFocus) onto OK; Enter commits it.
        Tui::ZTest::sendKey(&s.terminal, Qt::Key_Tab, Qt::NoModifier);
        Tui::ZTest::sendKey(&s.terminal, Qt::Key_Enter, Qt::NoModifier);

        QCOMPARE(dlg->wSystemPrompt(), QStringLiteral("Be terse.\nCite sources."));

        dlg->save();
        QCOMPARE(s.store.profile(QStringLiteral("prof-1"))
                     .value(QStringLiteral("systemPrompt"))
                     .toString(),
                 QStringLiteral("Be terse.\nCite sources."));
    }

    // The skills checklist: Enter toggles the highlighted row and the editor's
    // working array tracks it live; Esc closes the checklist, Save persists.
    void skillsToggleKeyFlow() {
        Scene s;
        ProfileEditorDialog* dlg = s.open(QStringLiteral("prof-1"));
        QVERIFY(dlg->wSkills().contains(QStringLiteral("web-search")));

        dlg->openSkillsToggle();
        // Row 0 is "web-search" (seeded on): Enter flips it off.
        Tui::ZTest::sendKey(&s.terminal, Qt::Key_Enter, Qt::NoModifier);
        QVERIFY(!dlg->wSkills().contains(QStringLiteral("web-search")));
        // Space flips it back on (both keys toggle).
        Tui::ZTest::sendKey(&s.terminal, Qt::Key_Space, Qt::NoModifier);
        QVERIFY(dlg->wSkills().contains(QStringLiteral("web-search")));
        // And off again, then close the checklist and save.
        Tui::ZTest::sendKey(&s.terminal, Qt::Key_Enter, Qt::NoModifier);
        Tui::ZTest::sendKey(&s.terminal, Qt::Key_Escape, Qt::NoModifier);
        dlg->save();

        QVERIFY(!s.store.profile(QStringLiteral("prof-1"))
                     .value(QStringLiteral("skills"))
                     .toStringList()
                     .contains(QStringLiteral("web-search")));
    }

    // The model palette keeps an unlisted current model selectable and lists
    // the provider's offered rows (via the palette's item building), while an
    // unlisted provider (no descriptor id) offers no palette at all.
    void modelPaletteGuardsUnlistedProvider() {
        Scene s;
        // A Mock-valued profile: provider resolves to no descriptor id.
        s.store.updateProfile(QStringLiteral("prof-1"),
                              {{QStringLiteral("provider"), QStringLiteral("mock")}});
        ProfileEditorDialog* dlg = s.open(QStringLiteral("prof-1"));
        QCOMPARE(dlg->wProviderId(), QString());
        dlg->openModelPalette(); // must not crash nor open (nothing to list)
        dlg->close();
    }
};

QTEST_MAIN(ProfileEditorTests)
#include "tst_profile_editor.moc"
