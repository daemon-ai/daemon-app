// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

// The TUI profile editor (parity with the GUI's ProfileEditor.qml): a modal
// hub dialog listing the editable fields of one profile; Enter on a row opens
// the matching sub-flow (text prompt / palette pick / multi-line editor /
// toggle checklist). Save commits ONE IProfileStore::updateProfile carrying
// exactly the GUI editor's field map, so daemon-mode round-trips are identical
// in both frontends; Esc/Cancel discards the working copy.

#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <Tui/ZButton.h>
#include <Tui/ZDialog.h>
#include <Tui/ZLabel.h>
#include <Tui/ZListView.h>

class PaletteDialog;

namespace models {
class IProviderCatalog;
}
namespace profiles {
class IProfileStore;
}

// A modal checklist over a {id, name, description} catalog: Enter or Space
// toggles the highlighted row, Esc/Done closes. Emits toggled(id) on every
// flip so the owner's working array stays live - the TUI analog of the GUI
// curator's CuratorRow toggles (commit still happens on the editor's Save).
class ProfileToggleListDialog : public Tui::ZDialog {
    Q_OBJECT

public:
    ProfileToggleListDialog(const QString& title, QVariantList catalog, QStringList enabled,
                            Tui::ZWidget* parent);

signals:
    void toggled(const QString& id);

protected:
    void keyEvent(Tui::ZKeyEvent* event) override;

private:
    void toggleRow(int row);
    void rebuildItems(int keepRow);

    QVariantList m_catalog;
    QStringList m_enabled;
    Tui::ZListView* m_list = nullptr;
};

// The profile editor ('e' on the Profiles page / a per-agent Profile tab).
// Field set == ProfileEditor.qml: name, provider (the node's ProviderCatalog,
// selection-only), base URL (cloud providers only), model (the provider's
// offered list; never free-text), description, multi-line system prompt, and
// the skills/tools curators. Engine is displayed read-only: engine choice is
// create-time (NewAgentDialog) in both frontends, and ProfileUpdate carries no
// engine change. The working copy commits on Save via the SAME updateProfile
// field names/semantics as the GUI save() path.
class ProfileEditorDialog : public Tui::ZDialog {
    Q_OBJECT

public:
    ProfileEditorDialog(profiles::IProfileStore* profiles, models::IProviderCatalog* catalog,
                        QString profileId, Tui::ZWidget* parent);

    // Working-copy state (read by the offscreen tests).
    [[nodiscard]] QString profileId() const { return m_profileId; }
    [[nodiscard]] QString wName() const { return m_wName; }
    [[nodiscard]] QString wProvider() const { return m_wProvider; }
    [[nodiscard]] QString wProviderId() const { return m_wProviderId; }
    [[nodiscard]] QString wBaseUrl() const { return m_wBaseUrl; }
    [[nodiscard]] QString wModel() const { return m_wModel; }
    [[nodiscard]] QString wDescription() const { return m_wDescription; }
    [[nodiscard]] QString wSystemPrompt() const { return m_wSystemPrompt; }
    [[nodiscard]] QStringList wSkills() const { return m_wSkills; }
    [[nodiscard]] QStringList wTools() const { return m_wTools; }

    // Working-copy mutators: the row sub-flows commit through these, and the
    // offscreen tests drive the same paths.
    void setName(const QString& name);
    // Pick a provider by ProviderCatalog descriptor id. Mirrors the GUI
    // dropdown's onActivated: persist the row's wireSelector (NOT the id),
    // invalidate the model, seed an empty base URL with the provider default
    // (cloud only), and re-list the provider's models with the profile's
    // stored credential.
    void selectProviderById(const QString& providerId);
    void setBaseUrl(const QString& baseUrl);
    // Select an offered model row; the local "Discover More Models" row emits
    // modelDiscoverRequested instead of selecting.
    void selectModel(const QString& modelId);
    void setDescription(const QString& description);
    void setSystemPrompt(const QString& prompt);
    void toggleSkill(const QString& id);
    void toggleTool(const QString& id);
    // One updateProfile with the GUI editor's exact field map, then saved() + close.
    void save();

    // Per-row edit flows (Enter on the matching row opens them).
    void openNamePrompt();
    void openProviderPalette();
    void openBaseUrlPrompt();
    void openModelPalette();
    void openDescriptionPrompt();
    void openPromptEditor();
    void openSkillsToggle();
    void openToolsToggle();

signals:
    void saved(const QString& profileId);
    // A local provider's "Discover More Models" row was activated: hand off to
    // the host's download flow (the GUI editor's Nav.open("models","discover")
    // analog is RootWidget::openModelDownload).
    void modelDiscoverRequested();

private:
    void load();
    void syncRowLabels();
    [[nodiscard]] QVariantMap providerDescriptor() const;
    [[nodiscard]] bool providerIsCloud() const;
    [[nodiscard]] QString providerDisplayName() const;
    void rebuildModelPaletteItems();
    void openToggleList(bool skills);

    profiles::IProfileStore* m_profiles = nullptr;
    models::IProviderCatalog* m_catalog = nullptr;
    QString m_profileId;

    // Working copy (ProfileEditor.qml parity). m_wProvider is the persisted
    // ProviderSelector wire string; m_wProviderId is the ProviderCatalog
    // descriptor id the picker/discovery keys on (all genai vendors share the
    // "genai" selector and are disambiguated by the id).
    QString m_wName;
    QString m_wProvider;
    QString m_wProviderId;
    QString m_wBaseUrl;
    QString m_wModel;
    QString m_wDescription;
    QString m_wSystemPrompt;
    QStringList m_wSkills;
    QStringList m_wTools;
    // Read-only engine binding ("Core"/"" = native; "Foreign" = foreign agent).
    QString m_engineKind;
    QString m_engineAgent;

    Tui::ZLabel* m_engineLabel = nullptr;
    Tui::ZButton* m_nameRow = nullptr;
    Tui::ZButton* m_providerRow = nullptr;
    Tui::ZButton* m_baseUrlRow = nullptr;
    Tui::ZButton* m_modelRow = nullptr;
    Tui::ZButton* m_descriptionRow = nullptr;
    Tui::ZButton* m_promptRow = nullptr;
    Tui::ZButton* m_skillsRow = nullptr;
    Tui::ZButton* m_toolsRow = nullptr;
    PaletteDialog* m_providerPalette = nullptr;
    PaletteDialog* m_modelPalette = nullptr;
};
