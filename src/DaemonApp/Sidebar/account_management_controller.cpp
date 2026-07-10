// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "account_management_controller.h"

#include "transports/itransport_registry.h"
#include "uimodels/variant_list_model.h"

// RED stub: the API compiles and the tests run, but no fields are seeded and no write routes — the
// assertions in tst_integrations_tree fail. GREEN fills in the schema projection + merge-edit.

AccountManagementController::AccountManagementController(QObject* parent)
    : QObject(parent), m_fields(new uimodels::VariantListModel(this)) {}

QObject* AccountManagementController::registry() const {
    return m_registry;
}

void AccountManagementController::setRegistry(QObject* registry) {
    m_registry = qobject_cast<transports::ITransportRegistry*>(registry);
    emit registryChanged();
}

QString AccountManagementController::mode() const {
    return m_mode;
}
QString AccountManagementController::transport() const {
    return m_transport;
}
QString AccountManagementController::family() const {
    return m_family;
}
QObject* AccountManagementController::fieldsModel() const {
    return m_fields;
}
QVariantList AccountManagementController::availableFamilies() const {
    return {};
}
QVariantList AccountManagementController::fields() const {
    QVariantList out;
    for (const QVariantMap& r : m_fields->rows()) {
        out.append(r);
    }
    return out;
}

void AccountManagementController::beginAdd(const QString& /*family*/) {}
void AccountManagementController::beginEdit(const QString& /*transport*/) {}
void AccountManagementController::cancel() {}
void AccountManagementController::submitAdd(const QVariantMap& /*values*/) {}
void AccountManagementController::submitEdit(const QVariantMap& /*values*/) {}
void AccountManagementController::removeAccount(const QString& /*transport*/) {}

QVariantList AccountManagementController::schemaFor(const QString& /*family*/) const {
    return {};
}
void AccountManagementController::setFieldRows(const QVariantList& /*rows*/) {}
void AccountManagementController::onSettingsChanged(const QString& /*transport*/,
                                                    const QVariantMap& /*values*/) {}
QSet<QString> AccountManagementController::maskedKeys() const {
    return {};
}
