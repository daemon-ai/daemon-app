// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QObject>

namespace uimodels {
class VariantListModel;
}

namespace tools {

// [wave2:app-approvals-safety] D2: read-only tool-inventory seam backing the Settings -> Tools
// surface. Rows (VariantListModel): name, description, enabled (bool), requires (the unmet
// prerequisite token when disabled, empty otherwise), requirementLabel (friendly copy for it), and
// requirementIsCredential (bool: the prerequisite is a credential the wave-1 auth stack owns, so
// the UI deep-links there rather than offering inline entry). There is NO enable/disable op at
// wire v29 (the node owns tool gating; ToolRegister is a register verb, Unsupported) — this seam is
// purely inventory + requirements guidance.
class IToolInventory : public QObject {
    Q_OBJECT
    Q_PROPERTY(QObject* tools READ tools CONSTANT)
    Q_PROPERTY(int count READ count NOTIFY changed)

public:
    using QObject::QObject;
    ~IToolInventory() override = default;

    [[nodiscard]] virtual QObject* tools() const = 0;
    [[nodiscard]] virtual int count() const = 0;

    // Re-query the node-wide inventory (ToolList). Call on connect-ready and on surface open.
    Q_INVOKABLE virtual void refresh() = 0;

signals:
    void changed();
};

} // namespace tools
