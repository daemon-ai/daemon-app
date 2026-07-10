// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "account_management_controller.h"

#include "transports/itransport_registry.h"
#include "uimodels/variant_list_model.h"

using transports::ITransportRegistry;

AccountManagementController::AccountManagementController(QObject* parent)
    : QObject(parent), m_fields(new uimodels::VariantListModel(this)) {}

QObject* AccountManagementController::registry() const {
    return m_registry;
}

void AccountManagementController::setRegistry(QObject* registry) {
    auto* reg = qobject_cast<ITransportRegistry*>(registry);
    if (m_registry == reg) {
        return;
    }
    if (m_registry != nullptr) {
        // ITransportRegistry declares a `disconnect(QString)` verb that shadows
        // QObject::disconnect; use the explicit static overload to drop our connections.
        QObject::disconnect(m_registry, nullptr, this, nullptr);
    }
    m_registry = reg;
    if (m_registry != nullptr) {
        connect(m_registry, &ITransportRegistry::settingsChanged, this,
                &AccountManagementController::onSettingsChanged);
    }
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
    QVariantList out;
    if (m_registry == nullptr) {
        return out;
    }
    for (const QVariant& a : m_registry->availableAdapters()) {
        const QVariantMap row = a.toMap();
        QVariantMap fam;
        fam[QStringLiteral("family")] = row.value(QStringLiteral("family"));
        fam[QStringLiteral("displayName")] = row.value(QStringLiteral("displayName"));
        out.append(fam);
    }
    return out;
}

QVariantList AccountManagementController::fields() const {
    QVariantList out;
    for (const QVariantMap& r : m_fields->rows()) {
        out.append(r);
    }
    return out;
}

QVariantList AccountManagementController::schemaFor(const QString& family) const {
    if (m_registry == nullptr) {
        return {};
    }
    for (const QVariant& a : m_registry->availableAdapters()) {
        const QVariantMap row = a.toMap();
        if (row.value(QStringLiteral("family")).toString() == family) {
            return row.value(QStringLiteral("schema")).toList();
        }
    }
    return {};
}

QSet<QString> AccountManagementController::maskedKeys() const {
    QSet<QString> out;
    for (const QVariantMap& f : m_fields->rows()) {
        if (f.value(QStringLiteral("kind")).toString() == QStringLiteral("Password")) {
            out.insert(f.value(QStringLiteral("key")).toString());
        }
    }
    return out;
}

// Seed the field rows from a schema, enriching each descriptor with a `value` (the default for a
// non-secret field, blank for a secret — the client never holds the secret).
void AccountManagementController::setFieldRows(const QVariantList& schema) {
    QList<QVariantMap> rows;
    rows.reserve(schema.size());
    for (const QVariant& s : schema) {
        QVariantMap f = s.toMap();
        // The row's `id` (VariantListModel upsert key) is the field key.
        f[QStringLiteral("id")] = f.value(QStringLiteral("key"));
        const bool secret =
            f.value(QStringLiteral("kind")).toString() == QStringLiteral("Password");
        f[QStringLiteral("value")] = secret ? QString() : f.value(QStringLiteral("default"));
        rows.append(f);
    }
    m_fields->setRows(rows);
    emit fieldsChanged();
}

void AccountManagementController::beginAdd(const QString& family) {
    m_mode = QStringLiteral("add");
    m_transport.clear();
    m_family = family;
    setFieldRows(schemaFor(family));
    emit stateChanged();
}

void AccountManagementController::beginEdit(const QString& transport) {
    if (m_registry == nullptr) {
        return;
    }
    // Resolve the family from the configured instance.
    QString family;
    for (const QVariant& i : m_registry->instances()) {
        const QVariantMap inst = i.toMap();
        if (inst.value(QStringLiteral("transport")).toString() == transport) {
            family = inst.value(QStringLiteral("family")).toString();
            break;
        }
    }
    m_mode = QStringLiteral("edit");
    m_transport = transport;
    m_family = family;
    setFieldRows(schemaFor(family));
    emit stateChanged();
    // Fold in any already-known non-secret values, then ask for a live re-read (settingsChanged
    // patches them in when it lands).
    onSettingsChanged(transport, m_registry->settings(transport));
    m_registry->refreshSettings(transport);
}

void AccountManagementController::onSettingsChanged(const QString& transport,
                                                    const QVariantMap& values) {
    if (m_mode != QStringLiteral("edit") || transport != m_transport || values.isEmpty()) {
        return;
    }
    // Patch the current non-secret values onto the matching fields in place.
    for (const QVariantMap& row : m_fields->rows()) {
        const QString key = row.value(QStringLiteral("key")).toString();
        if (values.contains(key)) {
            QVariantMap patched = row;
            patched[QStringLiteral("value")] = values.value(key);
            m_fields->upsert(patched);
        }
    }
    emit fieldsChanged();
}

void AccountManagementController::submitAdd(const QVariantMap& values) {
    if (m_mode != QStringLiteral("add")) {
        return;
    }
    // A fresh account is a sign-in: hand the collected fields to the interactive auth flow (A3) —
    // never configure directly.
    const QString family = m_family;
    emit authFlowRequested(family, values);
    m_mode = QStringLiteral("idle");
    emit stateChanged();
}

void AccountManagementController::submitEdit(const QVariantMap& values) {
    if (m_mode != QStringLiteral("edit") || m_registry == nullptr) {
        return;
    }
    const QSet<QString> masked = maskedKeys();
    // Merge-edit: send every field EXCEPT an untouched masked (Password) field. A masked field left
    // blank means "keep the stored secret"; a non-empty masked field is a deliberate change.
    QVariantMap out;
    for (auto it = values.constBegin(); it != values.constEnd(); ++it) {
        if (masked.contains(it.key()) && it.value().toString().isEmpty()) {
            continue; // omit the untouched secret
        }
        out.insert(it.key(), it.value());
    }
    const QString transport = m_transport;
    m_registry->configure(transport, out);
    emit editSubmitted(transport, out);
    // The node validates + reconnects and re-emits stored values; re-read to reflect them.
    m_registry->refreshSettings(transport);
}

void AccountManagementController::removeAccount(const QString& transport) {
    if (m_registry != nullptr) {
        m_registry->remove(transport);
    }
    if (m_transport == transport) {
        m_mode = QStringLiteral("idle");
        emit stateChanged();
    }
    emit accountRemoved(transport);
}

void AccountManagementController::cancel() {
    m_mode = QStringLiteral("idle");
    m_transport.clear();
    m_family.clear();
    emit stateChanged();
}
