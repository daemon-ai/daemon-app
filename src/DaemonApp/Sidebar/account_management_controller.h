// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QObject>
#include <QSet>
#include <QString>
#include <QtQml/qqmlregistration.h>
#include <QVariantList>
#include <QVariantMap>

namespace uimodels {
class VariantListModel;
}

namespace transports {
class ITransportRegistry;
}

// [integrations wire v38] The schema-driven account add/edit/remove controller (work package A2):
// the ONE Pidgin-style form the GUI + TUI both bind for BOTH adding a new integration and editing
// an existing one. The node owns the field schema (AdapterInfo.account_schema) and the persisted
// non-secret values; this controller only projects them and routes the write.
//
//  - ADD (a fresh sign-in): the picker collects the protocol choice, the schema form collects the
//    account fields, then the controller hands OFF to the interactive auth flow (A3 owns the
//    challenge UI) via authFlowRequested — it never configures a brand-new account directly.
//  - EDIT: reads the current non-secret values (TransportSettings) and applies a MERGE edit
//    (TransportConfigure) — a masked (Password) field the user left untouched is OMITTED from the
//    write (secrets never ride these ops; the node keeps the stored secret). The node validates +
//    reconnects and re-emits the stored values.
//  - REMOVE: tears the account down fully (the node sequences it).
//
// Every field carries the enriched descriptor shape {key, label, required, kind, default,
// placeholder, choices} so the form masks on kind=="Password", shows a numeric affordance on
// "Number", and a dropdown on "Choice".
class AccountManagementController : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QObject* registry READ registry WRITE setRegistry NOTIFY registryChanged)
    // "idle" | "add" | "edit": drives which form the surface presents (and its title).
    Q_PROPERTY(QString mode READ mode NOTIFY stateChanged)
    Q_PROPERTY(QString transport READ transport NOTIFY stateChanged)
    Q_PROPERTY(QString family READ family NOTIFY stateChanged)
    // The form fields (VariantListModel of {key,label,required,kind,default,placeholder,choices,
    // value}); the GUI/TUI bind this directly.
    Q_PROPERTY(QObject* fields READ fieldsModel NOTIFY fieldsChanged)
    // The adapter families available to add (the protocol picker): {family, displayName}.
    Q_PROPERTY(QVariantList availableFamilies READ availableFamilies NOTIFY registryChanged)

public:
    explicit AccountManagementController(QObject* parent = nullptr);

    [[nodiscard]] QObject* registry() const;
    void setRegistry(QObject* registry);
    [[nodiscard]] QString mode() const;
    [[nodiscard]] QString transport() const;
    [[nodiscard]] QString family() const;
    [[nodiscard]] QObject* fieldsModel() const;
    [[nodiscard]] QVariantList availableFamilies() const;
    // The current field descriptors + values (test/introspection convenience; same rows as
    // fieldsModel()).
    [[nodiscard]] Q_INVOKABLE QVariantList fields() const;

    // Begin adding a new account of `family`: seed the schema fields (defaults/placeholders,
    // secrets empty) and enter "add" mode.
    Q_INVOKABLE void beginAdd(const QString& family);
    // Begin editing an existing account: seed the schema fields, request the current non-secret
    // values (settingsChanged patches them in), and enter "edit" mode.
    Q_INVOKABLE void beginEdit(const QString& transport);
    // Cancel the in-flight add/edit (back to "idle").
    Q_INVOKABLE void cancel();

    // Commit the ADD form: hand off to the auth flow (A3) with the collected account fields — does
    // NOT configure directly (a fresh account is a sign-in). No-op unless in "add" mode.
    Q_INVOKABLE void submitAdd(const QVariantMap& values);
    // Commit the EDIT form: apply a merge-edit (TransportConfigure) omitting any untouched masked
    // (Password) field, then re-read. No-op unless in "edit" mode.
    Q_INVOKABLE void submitEdit(const QVariantMap& values);
    // Remove an account fully (the node sequences the teardown).
    Q_INVOKABLE void removeAccount(const QString& transport);

signals:
    void registryChanged();
    void stateChanged();
    void fieldsChanged();
    // ADD hand-off: the collected account fields for `family` are ready for the interactive auth
    // flow (A3). The parent/shell routes this to the auth-flow entry seam.
    void authFlowRequested(const QString& family, const QVariantMap& values);
    // A configure (edit) write was dispatched (the node validates/reconnects and re-emits values).
    void editSubmitted(const QString& transport, const QVariantMap& values);
    // A remove was dispatched.
    void accountRemoved(const QString& transport);

private:
    // The account_schema field descriptors for `family` from availableAdapters(), enriched with a
    // per-field `value` (defaults for add, blanks for edit until settingsChanged patches them).
    [[nodiscard]] QVariantList schemaFor(const QString& family) const;
    void setFieldRows(const QVariantList& schema);
    void onSettingsChanged(const QString& transport, const QVariantMap& values);
    // The key set of Password-kind fields in the current schema (for masked-field omission on
    // save).
    [[nodiscard]] QSet<QString> maskedKeys() const;

    transports::ITransportRegistry* m_registry = nullptr;
    uimodels::VariantListModel* m_fields = nullptr;
    QString m_mode = QStringLiteral("idle");
    QString m_transport;
    QString m_family;
};
