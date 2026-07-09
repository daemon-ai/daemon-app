// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "agent_setup_view.h"

#include "models/iprovider_catalog.h"
#include "setup/agent_setup_model.h"

#include <QAbstractItemModel>
#include <QRegularExpression>
#include <Tui/ZVBoxLayout.h>

namespace {
// The synthetic custom-endpoint provider row id (A1 / CON-10) — client-side only, never a catalog
// descriptor; mirrors AgentInferencePicker.qml's kCustomId and the first-run dialog's row.
const QString kCustomProviderId = QStringLiteral("__custom__");
} // namespace

AgentSetupView::AgentSetupView(setup::AgentSetupModel* setup, models::IProviderCatalog* catalog,
                               Tui::ZWidget* parent)
    : Tui::ZWidget(parent), m_setup(setup), m_catalog(catalog) {
    auto* layout = new Tui::ZVBoxLayout();
    setLayout(layout);

    // --- Backend section (foreign only) ---------------------------------------------------------
    m_backendLabel = new Tui::ZLabel(tr("Backend"), this);
    layout->addWidget(m_backendLabel);
    m_agentNativeBtn = new Tui::ZButton(QString(), this);
    layout->addWidget(m_agentNativeBtn);
    m_nodeProviderBtn = new Tui::ZButton(QString(), this);
    layout->addWidget(m_nodeProviderBtn);
    m_gatewayLabel = new Tui::ZLabel(tr("The node's provider gateway is disabled."), this);
    layout->addWidget(m_gatewayLabel);
    m_gatewayEnableBtn = new Tui::ZButton(tr("Enable gateway"), this);
    layout->addWidget(m_gatewayEnableBtn);
    m_modelHintLabel = new Tui::ZLabel(tr("Model hint (optional)"), this);
    layout->addWidget(m_modelHintLabel);
    m_modelHint = new Tui::ZInputBox(QString(), this);
    m_modelHint->setObjectName(QStringLiteral("setupModelHint"));
    layout->addWidget(m_modelHint);

    // --- Inference section (Core OR NodeProvider) -----------------------------------------------
    m_providerLabel = new Tui::ZLabel(tr("Provider"), this);
    layout->addWidget(m_providerLabel);
    m_providerList = new Tui::ZListView(this);
    m_providerList->setObjectName(QStringLiteral("setupProviderList"));
    layout->addWidget(m_providerList);
    m_key = new Tui::ZInputBox(QString(), this);
    m_key->setObjectName(QStringLiteral("setupKey"));
    m_key->setEchoMode(Tui::ZInputBox::Password);
    layout->addWidget(m_key);
    m_listModelsBtn = new Tui::ZButton(tr("List models"), this);
    layout->addWidget(m_listModelsBtn);
    m_keyGateMsg = new Tui::ZLabel(QString(), this);
    m_keyGateMsg->setObjectName(QStringLiteral("setupKeyGateMsg"));
    layout->addWidget(m_keyGateMsg);
    m_modelLabel = new Tui::ZLabel(tr("Model"), this);
    layout->addWidget(m_modelLabel);
    m_modelList = new Tui::ZListView(this);
    m_modelList->setObjectName(QStringLiteral("setupModelList"));
    layout->addWidget(m_modelList);
    m_baseUrlLabel = new Tui::ZLabel(tr("Base URL (e.g. https://…)"), this);
    layout->addWidget(m_baseUrlLabel);
    m_baseUrl = new Tui::ZInputBox(QString(), this);
    m_baseUrl->setObjectName(QStringLiteral("setupBaseUrl"));
    layout->addWidget(m_baseUrl);
    m_customModelLabel = new Tui::ZLabel(tr("Model id"), this);
    layout->addWidget(m_customModelLabel);
    m_customModel = new Tui::ZInputBox(QString(), this);
    m_customModel->setObjectName(QStringLiteral("setupCustomModel"));
    layout->addWidget(m_customModel);
    m_saveCustomBtn = new Tui::ZButton(tr("Save as reusable provider"), this);
    m_saveCustomBtn->setObjectName(QStringLiteral("setupSaveCustomProvider"));
    layout->addWidget(m_saveCustomBtn);

    // --- Wiring ---------------------------------------------------------------------------------
    connect(m_agentNativeBtn, &Tui::ZButton::clicked, this, [this] {
        if (m_setup != nullptr) {
            m_setup->chooseBackend(QStringLiteral("AgentNative"));
        }
        emit selectionChanged();
    });
    connect(m_nodeProviderBtn, &Tui::ZButton::clicked, this, [this] {
        if (m_setup != nullptr) {
            m_setup->chooseBackend(QStringLiteral("NodeProvider"));
        }
        emit selectionChanged();
    });
    connect(m_gatewayEnableBtn, &Tui::ZButton::clicked, this, [this] {
        if (m_setup != nullptr) {
            m_setup->setGatewayEnabled(true); // phase-F gateway surface owns the real GatewaySet
        }
    });
    connect(m_modelHint, &Tui::ZInputBox::textChanged, this, [this](const QString& text) {
        if (m_syncing) {
            return;
        }
        if (m_setup != nullptr) {
            m_setup->setAgentNativeModel(text);
        }
        emit selectionChanged();
    });

    connect(m_providerList, &Tui::ZListView::enterPressed, this, &AgentSetupView::selectProvider);
    connect(m_modelList, &Tui::ZListView::enterPressed, this, &AgentSetupView::selectModel);
    connect(m_key, &Tui::ZInputBox::textChanged, this, [this](const QString&) {
        if (m_syncing) {
            return;
        }
        if (m_setup != nullptr) {
            m_setup->setInferenceSelection(m_providerId, m_key->text());
        }
        pushInference();
        refreshInferenceControls();
        emit selectionChanged();
    });
    connect(m_listModelsBtn, &Tui::ZButton::clicked, this, [this] {
        if (m_catalog != nullptr && !m_providerId.isEmpty() && !customSelected()) {
            m_catalog->refreshModels(m_providerId, QString(), m_key->text());
        }
    });
    connect(m_saveCustomBtn, &Tui::ZButton::clicked, this, [this] {
        if (m_catalog == nullptr || m_baseUrl == nullptr) {
            return;
        }
        const QString base = m_baseUrl->text().trimmed();
        if (base.isEmpty()) {
            return;
        }
        // Derive a stable, user-namespaced id from the base URL (mirrors AgentInferencePicker).
        QString slug = base;
        slug.remove(QRegularExpression(QStringLiteral("^https?://")));
        slug.replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9._-]+")), QStringLiteral("-"));
        slug.remove(QRegularExpression(QStringLiteral("^-+|-+$")));
        slug = slug.toLower();
        if (slug.isEmpty()) {
            slug = QStringLiteral("endpoint");
        }
        QVariantMap fields;
        fields[QStringLiteral("id")] = QStringLiteral("custom/") + slug;
        fields[QStringLiteral("displayName")] = base;
        fields[QStringLiteral("baseUrl")] = base;
        fields[QStringLiteral("requiresKey")] = m_key != nullptr && !m_key->text().isEmpty();
        fields[QStringLiteral("credentialRef")] = QString();
        m_catalog->createCustom(fields);
    });
    connect(m_baseUrl, &Tui::ZInputBox::textChanged, this, [this](const QString&) {
        if (m_syncing) {
            return;
        }
        pushInference();
        refreshInferenceControls();
        emit selectionChanged();
    });
    connect(m_customModel, &Tui::ZInputBox::textChanged, this, [this](const QString& text) {
        if (m_syncing) {
            return;
        }
        m_selectedModel = text.trimmed(); // the typed model IS the selection on the custom path
        pushInference();
        refreshInferenceControls();
        emit selectionChanged();
    });

    if (m_catalog != nullptr) {
        connect(m_catalog, &models::IProviderCatalog::providersChanged, this,
                &AgentSetupView::rebuildProviderList);
        connect(m_catalog, &models::IProviderCatalog::offeredModelsChanged, this,
                [this](const QString& providerId) {
                    if (providerId == m_providerId) {
                        rebuildModelList();
                    }
                });
    }
    if (m_setup != nullptr) {
        connect(m_setup, &setup::AgentSetupModel::stateChanged, this,
                &AgentSetupView::syncFromModel);
        connect(m_setup, &setup::AgentSetupModel::keyGateChanged, this,
                &AgentSetupView::refreshInferenceControls);
    }

    rebuildProviderList();
    syncFromModel();
}

void AgentSetupView::setBackendAllowed(bool allowed) {
    m_backendAllowed = allowed;
    syncFromModel();
}

void AgentSetupView::setInferenceAllowed(bool allowed) {
    m_inferenceAllowed = allowed;
    syncFromModel();
}

void AgentSetupView::refresh() {
    rebuildProviderList();
    syncFromModel();
}

QVariantMap AgentSetupView::currentProviderDescriptor() const {
    if (m_catalog == nullptr || m_providerId.isEmpty() || customSelected()) {
        return {};
    }
    return m_catalog->descriptorFor(m_providerId);
}

bool AgentSetupView::customSelected() const {
    return m_providerId == kCustomProviderId;
}

void AgentSetupView::rebuildProviderList() {
    if (m_catalog == nullptr || m_providerList == nullptr) {
        return;
    }
    m_providerRows = m_catalog->providers();
    QVariantMap customRow;
    customRow[QStringLiteral("id")] = kCustomProviderId;
    customRow[QStringLiteral("name")] = tr("Custom endpoint…");
    customRow[QStringLiteral("kind")] = QStringLiteral("custom");
    m_providerRows.append(customRow);
    QStringList names;
    for (const QVariant& v : m_providerRows) {
        names << v.toMap().value(QStringLiteral("name")).toString();
    }
    m_providerList->setItems(names);
    // Default the selection to Daemon Cloud (keyless, works out of the box) on first populate.
    if (m_providerId.isEmpty() && !m_providerRows.isEmpty()) {
        int pick = 0;
        for (int i = 0; i < m_providerRows.size(); ++i) {
            const QVariantMap m = m_providerRows.at(i).toMap();
            if (m.value(QStringLiteral("id")).toString() == QStringLiteral("daemon_cloud") ||
                m.value(QStringLiteral("kind")).toString() == QStringLiteral("daemon_cloud")) {
                pick = i;
                break;
            }
        }
        selectProvider(pick);
    }
}

void AgentSetupView::selectProvider(int row) {
    if (row < 0 || row >= m_providerRows.size()) {
        return;
    }
    if (m_providerList->model() != nullptr) {
        m_providerList->setCurrentIndex(m_providerList->model()->index(row, 0));
    }
    m_providerId = m_providerRows.at(row).toMap().value(QStringLiteral("id")).toString();
    m_selectedModel.clear();
    if (m_setup != nullptr) {
        m_setup->setInferenceSelection(m_providerId, m_key != nullptr ? m_key->text() : QString());
    }
    pushInference();
    if (m_catalog != nullptr && !customSelected()) {
        m_catalog->refreshModels(m_providerId, QString(),
                                 m_key != nullptr ? m_key->text() : QString());
    }
    rebuildModelList();
    refreshInferenceControls();
    emit selectionChanged();
}

void AgentSetupView::rebuildModelList() {
    if (m_catalog == nullptr || m_modelList == nullptr) {
        return;
    }
    m_modelRows = m_catalog->offeredModels(m_providerId);
    QStringList names;
    for (const QVariant& v : m_modelRows) {
        const QVariantMap m = v.toMap();
        const bool discover =
            m.value(QStringLiteral("kind")).toString() == QStringLiteral("discover");
        names << (discover ? QStringLiteral("+ ") : QString()) +
                     m.value(QStringLiteral("name")).toString();
    }
    m_modelList->setItems(names);
    refreshInferenceControls();
}

void AgentSetupView::selectModel(int row) {
    if (row < 0 || row >= m_modelRows.size()) {
        return;
    }
    if (m_modelList->model() != nullptr) {
        m_modelList->setCurrentIndex(m_modelList->model()->index(row, 0));
    }
    const QVariantMap m = m_modelRows.at(row).toMap();
    if (m.value(QStringLiteral("kind")).toString() == QStringLiteral("discover")) {
        emit modelDiscoverRequested();
        return;
    }
    m_selectedModel = m.value(QStringLiteral("id")).toString();
    pushInference();
    refreshInferenceControls();
    emit selectionChanged();
}

void AgentSetupView::pushInference() {
    if (m_setup == nullptr) {
        return;
    }
    const bool custom = customSelected();
    const QString model =
        custom && m_customModel != nullptr ? m_customModel->text().trimmed() : m_selectedModel;
    const QString base = custom && m_baseUrl != nullptr ? m_baseUrl->text().trimmed() : QString();
    m_setup->setInference(m_providerId, model, m_key != nullptr ? m_key->text() : QString(), base);
}

bool AgentSetupView::inferenceComplete() const {
    if (m_setup == nullptr || !m_setup->needsInference()) {
        return true; // AgentNative: the agent brings its own model
    }
    if (customSelected()) {
        return m_baseUrl != nullptr && !m_baseUrl->text().trimmed().isEmpty() &&
               m_customModel != nullptr && !m_customModel->text().trimmed().isEmpty();
    }
    const bool requiresKey =
        currentProviderDescriptor().value(QStringLiteral("requiresKey")).toBool();
    const bool keyOk = !requiresKey || (m_key != nullptr && !m_key->text().isEmpty());
    return !m_providerId.isEmpty() && !m_selectedModel.isEmpty() && keyOk;
}

void AgentSetupView::syncFromModel() {
    m_syncing = true;
    const bool foreign = m_setup != nullptr && m_setup->needsBackendChoice();
    const bool showBackend = m_backendAllowed && foreign;
    const QString mode =
        m_setup != nullptr ? m_setup->backendMode() : QStringLiteral("AgentNative");
    const bool agentNative = mode != QStringLiteral("NodeProvider");

    if (m_backendLabel != nullptr) {
        m_backendLabel->setVisible(showBackend);
    }
    if (m_agentNativeBtn != nullptr) {
        m_agentNativeBtn->setVisible(showBackend);
        m_agentNativeBtn->setText((agentNative ? QStringLiteral("(o) ") : QStringLiteral("( ) ")) +
                                  tr("Agent's own backend"));
    }
    if (m_nodeProviderBtn != nullptr) {
        m_nodeProviderBtn->setVisible(showBackend);
        m_nodeProviderBtn->setText((agentNative ? QStringLiteral("( ) ") : QStringLiteral("(o) ")) +
                                   tr("Node provider (gateway)"));
    }
    const bool gatewayReq = m_setup != nullptr && m_setup->gatewayRequired();
    if (m_gatewayLabel != nullptr) {
        m_gatewayLabel->setVisible(showBackend && gatewayReq);
    }
    if (m_gatewayEnableBtn != nullptr) {
        m_gatewayEnableBtn->setVisible(showBackend && gatewayReq);
    }
    // The optional AgentNative model hint (reflect the model's value without re-driving it).
    if (m_modelHint != nullptr) {
        const bool showHint = showBackend && agentNative;
        m_modelHintLabel->setVisible(showHint);
        m_modelHint->setVisible(showHint);
        const QString hint = m_setup != nullptr ? m_setup->agentNativeModel() : QString();
        if (m_modelHint->text() != hint) {
            m_modelHint->setText(hint);
        }
    }

    refreshInferenceControls();
    m_syncing = false;
}

void AgentSetupView::refreshInferenceControls() {
    const bool needsInference =
        m_inferenceAllowed && m_setup != nullptr && m_setup->needsInference();
    const bool custom = customSelected();
    const QVariantMap desc = currentProviderDescriptor();
    const bool requiresKey = desc.value(QStringLiteral("requiresKey")).toBool();

    if (m_providerLabel != nullptr) {
        m_providerLabel->setVisible(needsInference);
    }
    if (m_providerList != nullptr) {
        m_providerList->setVisible(needsInference);
    }
    if (m_key != nullptr) {
        m_key->setVisible(needsInference && (requiresKey || custom));
    }
    if (m_listModelsBtn != nullptr) {
        m_listModelsBtn->setVisible(needsInference && requiresKey);
    }
    if (m_modelLabel != nullptr) {
        m_modelLabel->setVisible(needsInference && !custom);
    }
    if (m_modelList != nullptr) {
        m_modelList->setVisible(needsInference && !custom);
    }
    if (m_baseUrlLabel != nullptr) {
        m_baseUrlLabel->setVisible(needsInference && custom);
    }
    if (m_baseUrl != nullptr) {
        m_baseUrl->setVisible(needsInference && custom);
    }
    if (m_customModelLabel != nullptr) {
        m_customModelLabel->setVisible(needsInference && custom);
    }
    if (m_customModel != nullptr) {
        m_customModel->setVisible(needsInference && custom);
    }
    if (m_saveCustomBtn != nullptr) {
        // Offer persistence only once the custom endpoint has a base URL to save.
        const bool haveBase = m_baseUrl != nullptr && !m_baseUrl->text().trimmed().isEmpty();
        m_saveCustomBtn->setVisible(needsInference && custom && haveBase);
    }
    if (m_keyGateMsg != nullptr) {
        const QString reason = m_setup != nullptr ? m_setup->keyGateMessage() : QString();
        m_keyGateMsg->setText(reason);
        m_keyGateMsg->setVisible(needsInference && requiresKey && !reason.isEmpty());
    }
}
