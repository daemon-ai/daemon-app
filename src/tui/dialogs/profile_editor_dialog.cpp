// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "profile_editor_dialog.h"

#include "models/iprovider_catalog.h"
#include "palette_dialog.h"
#include "profiles/iprofile_store.h"
#include "tui_dialogs.h"

#include <QAbstractItemModel>
#include <Tui/ZEvent.h>
#include <Tui/ZHBoxLayout.h>
#include <Tui/ZTerminal.h>
#include <Tui/ZTextEdit.h>
#include <Tui/ZVBoxLayout.h>
#include <Tui/ZWindow.h>
#include <utility>

namespace {

// Map a (possibly namespaced) model id to its genai vendor descriptor id,
// mirroring ProfileEditor.qml's _vendorIdForModel (genai AdapterKind::from_model
// heuristics): a namespaced id carries the vendor as the "<vendor>::" prefix; a
// bare, self-identifying name is classified by prefix. "" when unclassifiable.
QString vendorIdForModel(const QString& model) {
    if (model.isEmpty()) {
        return {};
    }
    const qsizetype sep = model.indexOf(QStringLiteral("::"));
    if (sep > 0) {
        return model.left(sep);
    }
    const auto sw = [&model](const char* prefix) {
        return model.startsWith(QLatin1String(prefix));
    };
    if (sw("o3") || sw("o4") || sw("o1") || sw("chatgpt") || sw("codex") ||
        (sw("gpt") && !sw("gpt-oss")) || sw("text-embedding")) {
        return QStringLiteral("openai");
    }
    if (sw("gemini")) {
        return QStringLiteral("gemini");
    }
    if (sw("claude")) {
        return QStringLiteral("anthropic");
    }
    if (sw("grok")) {
        return QStringLiteral("xai");
    }
    if (sw("command") || sw("embed-")) {
        return QStringLiteral("cohere");
    }
    if (sw("deepseek-")) {
        return QStringLiteral("deepseek");
    }
    return {};
}

// Best-effort resolve of a saved (wireSelector, model) back to a ProviderCatalog
// descriptor id (ProfileEditor.qml's _inferProviderId): a unique selector match
// wins; genai's shared selector resolves the vendor from the model id (so a
// claude-* model reads as Anthropic, not the first genai vendor), falling back
// to the first match. Empty when nothing matches (e.g. a Mock-valued profile).
QString inferProviderId(const QVariantList& providers, const QString& selector,
                        const QString& model) {
    if (selector.isEmpty()) {
        return {};
    }
    QList<QVariantMap> matches;
    for (const QVariant& v : providers) {
        const QVariantMap row = v.toMap();
        if (row.value(QStringLiteral("wireSelector")).toString() == selector) {
            matches.append(row);
        }
    }
    if (matches.isEmpty()) {
        return {};
    }
    if (matches.size() == 1) {
        return matches.first().value(QStringLiteral("id")).toString();
    }
    QString vendorId = vendorIdForModel(model);
    if (!vendorId.isEmpty()) {
        for (const QVariantMap& row : matches) {
            if (row.value(QStringLiteral("id")).toString() == vendorId) {
                return vendorId;
            }
        }
    }
    return matches.first().value(QStringLiteral("id")).toString();
}

// Elide a row value so the fixed-width row buttons stay one line.
QString elide(const QString& text, int max) {
    return text.size() <= max ? text : text.left(max - 1) + QStringLiteral("\u2026");
}

// ZTextEdit with the composer's newline fix (see SubmitInputBox): ZDocument
// tracks a missing trailing newline implicitly, so a plain Enter at the very
// end of the document would collapse into that flag (no materialized line, no
// visual break). Clear it around the insert to force a real split, then
// restore it so text() never grows a spurious trailing "\n".
class MultilineEditor : public Tui::ZTextEdit {
public:
    using Tui::ZTextEdit::ZTextEdit;

protected:
    void keyEvent(Tui::ZKeyEvent* event) override {
        const bool isEnter = event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return ||
                             event->text() == QStringLiteral("\r");
        if (isEnter && event->modifiers() == Qt::NoModifier) {
            document()->setNewlineAfterLastLineMissing(false);
            insertText(QStringLiteral("\n"));
            document()->setNewlineAfterLastLineMissing(true);
            adjustScrollPosition();
            update();
            event->accept();
            return;
        }
        Tui::ZTextEdit::keyEvent(event);
    }
};

} // namespace

// --- ProfileToggleListDialog -----------------------------------------------

ProfileToggleListDialog::ProfileToggleListDialog(const QString& title, QVariantList catalog,
                                                 QStringList enabled, Tui::ZWidget* parent)
    : Tui::ZDialog(parent), m_catalog(std::move(catalog)), m_enabled(std::move(enabled)) {
    setOptions(Tui::ZWindow::DeleteOnClose);
    setWindowTitle(title);
    setContentsMargins({2, 1, 2, 1});

    auto* layout = new Tui::ZVBoxLayout();
    setLayout(layout);

    layout->addWidget(new Tui::ZLabel(tr("Enter / Space toggles · Esc done"), this));
    m_list = new Tui::ZListView(this);
    layout->addWidget(m_list);
    layout->addSpacing(1);

    auto* buttons = new Tui::ZHBoxLayout();
    layout->add(buttons);
    buttons->addStretch();
    auto* done = new Tui::ZButton(tr("Done"), this);
    buttons->addWidget(done);

    connect(done, &Tui::ZButton::clicked, this, &Tui::ZWindow::close);
    connect(m_list, &Tui::ZListView::enterPressed, this, [this](int row) { toggleRow(row); });

    rebuildItems(0);
    m_list->setFocus();
    setGeometry(QRect(0, 0, 58, qBound(9, static_cast<int>(m_catalog.size()) + 7, 16)));
}

void ProfileToggleListDialog::keyEvent(Tui::ZKeyEvent* event) {
    // Space toggles too; it bubbles here because the list claims only
    // arrows/Enter. (Enter is handled via the list's enterPressed.)
    if (event->modifiers() == Qt::NoModifier &&
        (event->key() == Qt::Key_Space || event->text() == QStringLiteral(" "))) {
        toggleRow(m_list != nullptr && m_list->currentIndex().isValid()
                      ? m_list->currentIndex().row()
                      : 0);
        event->accept();
        return;
    }
    Tui::ZDialog::keyEvent(event);
}

void ProfileToggleListDialog::toggleRow(int row) {
    if (row < 0 || row >= m_catalog.size()) {
        return;
    }
    const QString id = m_catalog.at(row).toMap().value(QStringLiteral("id")).toString();
    if (m_enabled.contains(id)) {
        m_enabled.removeAll(id);
    } else {
        m_enabled.append(id);
    }
    rebuildItems(row);
    emit toggled(id);
}

void ProfileToggleListDialog::rebuildItems(int keepRow) {
    if (m_list == nullptr) {
        return;
    }
    QStringList lines;
    for (const QVariant& v : m_catalog) {
        const QVariantMap entry = v.toMap();
        const bool on = m_enabled.contains(entry.value(QStringLiteral("id")).toString());
        lines << QStringLiteral("%1 %2 · %3")
                     .arg(on ? QStringLiteral("[x]") : QStringLiteral("[ ]"),
                          entry.value(QStringLiteral("name")).toString(),
                          elide(entry.value(QStringLiteral("description")).toString(), 30));
    }
    m_list->setItems(lines);
    if (m_list->model() != nullptr && !lines.isEmpty()) {
        const int row = qBound(0, keepRow, static_cast<int>(lines.size()) - 1);
        m_list->setCurrentIndex(m_list->model()->index(row, 0));
    }
}

// --- ProfileEditorDialog ----------------------------------------------------

ProfileEditorDialog::ProfileEditorDialog(profiles::IProfileStore* profiles,
                                         models::IProviderCatalog* catalog, QString profileId,
                                         Tui::ZWidget* parent)
    : Tui::ZDialog(parent), m_profiles(profiles), m_catalog(catalog),
      m_profileId(std::move(profileId)) {
    setOptions(Tui::ZWindow::DeleteOnClose);
    setWindowTitle(tr("Edit profile — %1").arg(m_profileId));
    setContentsMargins({2, 1, 2, 1});

    auto* layout = new Tui::ZVBoxLayout();
    setLayout(layout);

    layout->addWidget(
        new Tui::ZLabel(tr("Enter on a row edits it · Save commits · Esc discards"), this));
    layout->addSpacing(1);

    m_nameRow = new Tui::ZButton(QString(), this);
    m_engineLabel = new Tui::ZLabel(QString(), this);
    m_providerRow = new Tui::ZButton(QString(), this);
    m_baseUrlRow = new Tui::ZButton(QString(), this);
    m_modelRow = new Tui::ZButton(QString(), this);
    m_descriptionRow = new Tui::ZButton(QString(), this);
    m_promptRow = new Tui::ZButton(QString(), this);
    m_skillsRow = new Tui::ZButton(QString(), this);
    m_toolsRow = new Tui::ZButton(QString(), this);
    layout->addWidget(m_nameRow);
    layout->addWidget(m_engineLabel);
    layout->addWidget(m_providerRow);
    layout->addWidget(m_baseUrlRow);
    layout->addWidget(m_modelRow);
    layout->addWidget(m_descriptionRow);
    layout->addWidget(m_promptRow);
    layout->addWidget(m_skillsRow);
    layout->addWidget(m_toolsRow);
    layout->addSpacing(1);

    auto* buttons = new Tui::ZHBoxLayout();
    layout->add(buttons);
    buttons->addStretch();
    auto* saveBtn = new Tui::ZButton(tr("Save"), this);
    buttons->addWidget(saveBtn);
    auto* cancelBtn = new Tui::ZButton(tr("Cancel"), this);
    buttons->addWidget(cancelBtn);

    connect(m_nameRow, &Tui::ZButton::clicked, this, &ProfileEditorDialog::openNamePrompt);
    connect(m_providerRow, &Tui::ZButton::clicked, this, &ProfileEditorDialog::openProviderPalette);
    connect(m_baseUrlRow, &Tui::ZButton::clicked, this, &ProfileEditorDialog::openBaseUrlPrompt);
    connect(m_modelRow, &Tui::ZButton::clicked, this, &ProfileEditorDialog::openModelPalette);
    connect(m_descriptionRow, &Tui::ZButton::clicked, this,
            &ProfileEditorDialog::openDescriptionPrompt);
    connect(m_promptRow, &Tui::ZButton::clicked, this, &ProfileEditorDialog::openPromptEditor);
    connect(m_skillsRow, &Tui::ZButton::clicked, this, &ProfileEditorDialog::openSkillsToggle);
    connect(m_toolsRow, &Tui::ZButton::clicked, this, &ProfileEditorDialog::openToolsToggle);
    connect(saveBtn, &Tui::ZButton::clicked, this, &ProfileEditorDialog::save);
    connect(cancelBtn, &Tui::ZButton::clicked, this, &Tui::ZDialog::reject);

    if (m_catalog != nullptr) {
        // Live model list: repopulate the open Model palette when the node's
        // ProviderModels answer (or a local download re-offer) lands for the
        // selected provider (GUI parity: modelCombo.reload() on the signal).
        connect(m_catalog, &models::IProviderCatalog::offeredModelsChanged, this,
                [this](const QString& providerId) {
                    if (providerId == m_wProviderId && m_modelPalette != nullptr &&
                        m_modelPalette->isLocallyVisible()) {
                        rebuildModelPaletteItems();
                    }
                });
    }

    load();
    m_nameRow->setFocus();
    setGeometry(QRect(0, 0, 64, 19));
}

void ProfileEditorDialog::load() {
    const QVariantMap p = m_profiles != nullptr ? m_profiles->profile(m_profileId) : QVariantMap{};
    m_wName = p.value(QStringLiteral("name")).toString();
    // Preserve whatever provider the profile has (incl. an unlisted "mock");
    // never coerce it - save() writes the selector back verbatim.
    m_wProvider = p.value(QStringLiteral("provider")).toString();
    m_wBaseUrl = p.value(QStringLiteral("baseUrl")).toString();
    m_wModel = p.value(QStringLiteral("model")).toString();
    m_wProviderId = inferProviderId(m_catalog != nullptr ? m_catalog->providers() : QVariantList{},
                                    m_wProvider, m_wModel);
    m_wDescription = p.value(QStringLiteral("description")).toString();
    m_wSystemPrompt = p.value(QStringLiteral("systemPrompt")).toString();
    m_wSkills = p.value(QStringLiteral("skills")).toStringList();
    m_wTools = p.value(QStringLiteral("tools")).toStringList();
    m_engineKind = p.value(QStringLiteral("engine")).toString();
    m_engineAgent = p.value(QStringLiteral("acpAgent")).toString();
    // Warm the provider's model list with the profile's stored credential so
    // the Model palette offers real, selectable rows (GUI load() parity).
    if (m_catalog != nullptr && !m_wProviderId.isEmpty()) {
        m_catalog->refreshModels(m_wProviderId, m_profileId, QString());
    }
    // The curators are seam-driven; with empty catalogs (e.g. the daemon
    // store's) the rows are inert, matching the GUI's empty Repeaters.
    m_skillsRow->setEnabled(m_profiles != nullptr && !m_profiles->availableSkills().isEmpty());
    m_toolsRow->setEnabled(m_profiles != nullptr && !m_profiles->availableTools().isEmpty());
    syncRowLabels();
}

void ProfileEditorDialog::syncRowLabels() {
    const auto row = [](const QString& label, const QString& value) {
        return QStringLiteral("%1: %2").arg(label, value);
    };
    m_nameRow->setText(row(tr("Name"), elide(m_wName, 40)));
    // Engine is a create-time choice (NewAgentDialog) in both frontends;
    // ProfileUpdate carries no engine change, so it renders read-only here.
    m_engineLabel->setText(
        m_engineKind == QStringLiteral("Acp")
            ? tr("Engine: ACP agent · %1 (set at create time)").arg(m_engineAgent)
            : tr("Engine: daemon-core (native)"));
    m_providerRow->setText(row(tr("Provider"), providerDisplayName()));
    m_baseUrlRow->setText(row(tr("Base URL"), m_wBaseUrl.isEmpty() ? tr("(provider default)")
                                                                   : elide(m_wBaseUrl, 38)));
    // Base URL only applies to cloud providers (GUI parity).
    m_baseUrlRow->setVisible(providerIsCloud());
    m_modelRow->setText(
        row(tr("Model"), m_wModel.isEmpty() ? tr("(pick a model)") : elide(m_wModel, 40)));
    m_descriptionRow->setText(row(tr("Description"), elide(m_wDescription, 36)));
    m_promptRow->setText(
        row(tr("System prompt"), elide(m_wSystemPrompt.section(QLatin1Char('\n'), 0, 0), 34)));
    m_skillsRow->setText(row(tr("Skills"), elide(m_wSkills.join(QStringLiteral(", ")), 40)));
    m_toolsRow->setText(row(tr("Tools"), elide(m_wTools.join(QStringLiteral(", ")), 40)));
}

QVariantMap ProfileEditorDialog::providerDescriptor() const {
    if (m_catalog == nullptr || m_wProviderId.isEmpty()) {
        return {};
    }
    return m_catalog->descriptorFor(m_wProviderId);
}

bool ProfileEditorDialog::providerIsCloud() const {
    return providerDescriptor().value(QStringLiteral("cloud")).toBool();
}

QString ProfileEditorDialog::providerDisplayName() const {
    const QVariantMap desc = providerDescriptor();
    if (!desc.isEmpty()) {
        return desc.value(QStringLiteral("name")).toString();
    }
    if (!m_wProvider.isEmpty()) {
        // A profile whose provider is not a real, selectable one (e.g. a
        // Mock-valued profile under the mock service flag): render it verbatim
        // rather than coercing it (the GUI's read-only caption).
        return tr("%1 (test provider — not selectable)").arg(m_wProvider);
    }
    return tr("(pick a provider)");
}

void ProfileEditorDialog::setName(const QString& name) {
    m_wName = name;
    syncRowLabels();
}

void ProfileEditorDialog::selectProviderById(const QString& providerId) {
    if (m_catalog == nullptr) {
        return;
    }
    const QVariantMap desc = m_catalog->descriptorFor(providerId);
    if (desc.isEmpty()) {
        return;
    }
    m_wProviderId = providerId;
    // The profile persists the ProviderSelector (wireSelector), NOT the id:
    // all genai vendors share "genai" and are disambiguated by id at
    // discovery time.
    m_wProvider = desc.value(QStringLiteral("wireSelector")).toString();
    // Switching provider invalidates the current model (selection-only).
    m_wModel.clear();
    // Pre-fill the provider's default base URL (e.g. Daemon Cloud) for a
    // cloud provider with no base set; the node/provider default otherwise.
    if (desc.value(QStringLiteral("cloud")).toBool() && m_wBaseUrl.isEmpty()) {
        m_wBaseUrl = desc.value(QStringLiteral("defaultBaseUrl")).toString();
    }
    // Fetch this provider's models under the profile's stored credential; the
    // offeredModelsChanged hook repopulates an open Model palette on arrival.
    m_catalog->refreshModels(providerId, m_profileId, QString());
    syncRowLabels();
}

void ProfileEditorDialog::setBaseUrl(const QString& baseUrl) {
    m_wBaseUrl = baseUrl;
    syncRowLabels();
}

void ProfileEditorDialog::selectModel(const QString& modelId) {
    if (modelId == QLatin1String(models::IProviderCatalog::kDiscoverMoreId)) {
        // Local "Discover More Models": route to the shared download flow; a
        // finished download re-offers the new model (offeredModelsChanged).
        emit modelDiscoverRequested();
        return;
    }
    m_wModel = modelId;
    syncRowLabels();
}

void ProfileEditorDialog::setDescription(const QString& description) {
    m_wDescription = description;
    syncRowLabels();
}

void ProfileEditorDialog::setSystemPrompt(const QString& prompt) {
    m_wSystemPrompt = prompt;
    syncRowLabels();
}

void ProfileEditorDialog::toggleSkill(const QString& id) {
    if (m_wSkills.contains(id)) {
        m_wSkills.removeAll(id);
    } else {
        m_wSkills.append(id);
    }
    syncRowLabels();
}

void ProfileEditorDialog::toggleTool(const QString& id) {
    if (m_wTools.contains(id)) {
        m_wTools.removeAll(id);
    } else {
        m_wTools.append(id);
    }
    syncRowLabels();
}

void ProfileEditorDialog::save() {
    if (m_profiles == nullptr) {
        return;
    }
    // EXACTLY ProfileEditor.qml's save() field map: the selector is written
    // verbatim (an unlisted Mock value survives untouched); baseUrl only
    // applies to cloud providers - empty clears it (=> node default).
    QVariantMap fields;
    fields[QStringLiteral("name")] = m_wName;
    fields[QStringLiteral("provider")] = m_wProvider;
    fields[QStringLiteral("baseUrl")] = providerIsCloud() ? m_wBaseUrl : QString();
    fields[QStringLiteral("model")] = m_wModel;
    fields[QStringLiteral("description")] = m_wDescription;
    fields[QStringLiteral("systemPrompt")] = m_wSystemPrompt;
    fields[QStringLiteral("skills")] = m_wSkills;
    fields[QStringLiteral("tools")] = m_wTools;
    m_profiles->updateProfile(m_profileId, fields);
    emit saved(m_profileId);
    close();
}

void ProfileEditorDialog::openNamePrompt() {
    auto* dlg = new TextPromptDialog(tr("Profile name"), m_wName, false, this);
    connect(dlg, &TextPromptDialog::submitted, this, &ProfileEditorDialog::setName);
    connect(dlg, &QObject::destroyed, m_nameRow, [row = m_nameRow] { row->setFocus(); });
}

void ProfileEditorDialog::openBaseUrlPrompt() {
    auto* dlg =
        new TextPromptDialog(tr("Base URL (empty = provider default)"), m_wBaseUrl, false, this);
    connect(dlg, &TextPromptDialog::submitted, this, &ProfileEditorDialog::setBaseUrl);
    connect(dlg, &QObject::destroyed, m_baseUrlRow, [row = m_baseUrlRow] { row->setFocus(); });
}

void ProfileEditorDialog::openDescriptionPrompt() {
    auto* dlg = new TextPromptDialog(tr("Short description"), m_wDescription, false, this);
    connect(dlg, &TextPromptDialog::submitted, this, &ProfileEditorDialog::setDescription);
    connect(dlg, &QObject::destroyed, m_descriptionRow,
            [row = m_descriptionRow] { row->setFocus(); });
}

void ProfileEditorDialog::openProviderPalette() {
    if (m_catalog == nullptr) {
        return;
    }
    if (m_providerPalette == nullptr) {
        m_providerPalette = new PaletteDialog(tr("Provider"), this);
        connect(m_providerPalette, &PaletteDialog::activated, this, [this](const QString& id) {
            selectProviderById(id);
            m_providerRow->setFocus();
        });
    }
    QVector<PaletteDialog::Item> items;
    const QVariantList rows = m_catalog->providers();
    items.reserve(rows.size());
    for (const QVariant& v : rows) {
        const QVariantMap row = v.toMap();
        const QString id = row.value(QStringLiteral("id")).toString();
        items.push_back({id,
                         (id == m_wProviderId ? QStringLiteral("✓ ") : QStringLiteral("  ")) +
                             row.value(QStringLiteral("name")).toString(),
                         row.value(QStringLiteral("kind")).toString()});
    }
    m_providerPalette->setItems(items);
    m_providerPalette->openCentered();
}

void ProfileEditorDialog::openModelPalette() {
    // An unlisted provider (e.g. a Mock-valued profile) has no node list to
    // offer - pick a real provider first (the GUI dropdown is empty likewise).
    if (m_catalog == nullptr || m_wProviderId.isEmpty()) {
        return;
    }
    if (m_modelPalette == nullptr) {
        m_modelPalette = new PaletteDialog(tr("Model"), this);
        connect(m_modelPalette, &PaletteDialog::activated, this, [this](const QString& id) {
            selectModel(id);
            m_modelRow->setFocus();
        });
    }
    rebuildModelPaletteItems();
    m_modelPalette->openCentered();
}

void ProfileEditorDialog::rebuildModelPaletteItems() {
    if (m_modelPalette == nullptr || m_catalog == nullptr) {
        return;
    }
    const QVariantList rows = m_catalog->offeredModels(m_wProviderId);
    QVector<PaletteDialog::Item> items;
    items.reserve(rows.size() + 1);
    bool currentListed = m_wModel.isEmpty();
    for (const QVariant& v : rows) {
        const QVariantMap row = v.toMap();
        const QString id = row.value(QStringLiteral("id")).toString();
        const bool discover =
            row.value(QStringLiteral("kind")).toString() == QStringLiteral("discover");
        currentListed = currentListed || id == m_wModel;
        items.push_back(
            {id,
             discover ? QStringLiteral("+ ") + row.value(QStringLiteral("name")).toString()
                      : (id == m_wModel ? QStringLiteral("✓ ") : QStringLiteral("  ")) +
                            row.value(QStringLiteral("name")).toString(),
             discover ? tr("download") : row.value(QStringLiteral("provider")).toString()});
    }
    // An existing model absent from the offered list stays selectable so it
    // never silently drops (GUI modelCombo parity).
    if (!currentListed) {
        items.prepend({m_wModel, QStringLiteral("✓ ") + m_wModel, tr("current")});
    }
    m_modelPalette->setItems(items);
}

void ProfileEditorDialog::openPromptEditor() {
    // Multi-line persona editor: ZTextEdit (the composer's base widget) handles
    // multi-line natively - Enter inserts a newline, Tab moves focus out.
    if (terminal() == nullptr) {
        return;
    }
    auto* dlg = new Tui::ZDialog(this);
    dlg->setOptions(Tui::ZWindow::DeleteOnClose);
    dlg->setWindowTitle(tr("System prompt"));
    dlg->setContentsMargins({2, 1, 2, 1});

    auto* layout = new Tui::ZVBoxLayout();
    dlg->setLayout(layout);
    layout->addWidget(
        new Tui::ZLabel(tr("Enter inserts a newline · Tab reaches the buttons"), dlg));

    auto* edit = new MultilineEditor(terminal()->textMetrics(), dlg);
    edit->setText(m_wSystemPrompt);
    edit->setTabChangesFocus(true);
    edit->setMinimumSize(40, 6);
    layout->addWidget(edit);
    layout->addSpacing(1);

    auto* buttons = new Tui::ZHBoxLayout();
    layout->add(buttons);
    buttons->addStretch();
    auto* ok = new Tui::ZButton(tr("OK"), dlg);
    ok->setDefault(true);
    buttons->addWidget(ok);
    auto* cancel = new Tui::ZButton(tr("Cancel"), dlg);
    buttons->addWidget(cancel);

    connect(ok, &Tui::ZButton::clicked, dlg, [this, dlg, edit] {
        setSystemPrompt(edit->text());
        dlg->close();
    });
    connect(cancel, &Tui::ZButton::clicked, dlg, &Tui::ZDialog::reject);
    connect(dlg, &QObject::destroyed, m_promptRow, [row = m_promptRow] { row->setFocus(); });

    dlg->setGeometry(QRect(0, 0, 58, 14));
    edit->setFocus();
}

void ProfileEditorDialog::openToggleList(bool skills) {
    if (m_profiles == nullptr) {
        return;
    }
    const QVariantList catalog =
        skills ? m_profiles->availableSkills() : m_profiles->availableTools();
    if (catalog.isEmpty()) {
        return; // seam offers no catalog (daemon store): nothing to toggle
    }
    auto* dlg = new ProfileToggleListDialog(skills ? tr("Skills") : tr("Tools"), catalog,
                                            skills ? m_wSkills : m_wTools, this);
    connect(dlg, &ProfileToggleListDialog::toggled, this,
            skills ? &ProfileEditorDialog::toggleSkill : &ProfileEditorDialog::toggleTool);
    Tui::ZButton* row = skills ? m_skillsRow : m_toolsRow;
    connect(dlg, &QObject::destroyed, row, [row] { row->setFocus(); });
}

void ProfileEditorDialog::openSkillsToggle() {
    openToggleList(true);
}

void ProfileEditorDialog::openToolsToggle() {
    openToggleList(false);
}
