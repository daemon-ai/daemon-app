// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QObject>
#include <QString>

namespace fleet {

// Fleet tree seam backing the Fleet surface. The tree is flattened into a
// VariantListModel for display: rows carry depth (indent), id, name, kind
// ("orchestrator"/"worker"), status ("running"/"paused"/"idle"), and model.
class IFleetTree : public QObject {
    Q_OBJECT
    Q_PROPERTY(QObject* nodes READ nodes CONSTANT)

public:
    using QObject::QObject;
    ~IFleetTree() override = default;

    [[nodiscard]] virtual QObject* nodes() const = 0;

    Q_INVOKABLE virtual void pause(const QString& id) = 0;
    Q_INVOKABLE virtual void resume(const QString& id) = 0;
    // Scale an orchestrator unit's worker count (PRO-10). Default is a no-op so the mock seam need
    // not implement it; the daemon seam sends Scale{unit,n}.
    Q_INVOKABLE virtual void scale(const QString& id, int n) {
        Q_UNUSED(id)
        Q_UNUSED(n)
    }
    // Refresh the tree from the node (daemon seam re-fetches Tree; mock is a no-op).
    Q_INVOKABLE virtual void refresh() {}

signals:
    void changed();
    // A control op (pause/resume/scale) was rejected by the node (e.g. Unsupported on an
    // engine-leaf unit - PRO-10 control is only meaningful on orchestrator units). Mock never emits
    // this.
    void controlRejected(const QString& message);
};

} // namespace fleet
