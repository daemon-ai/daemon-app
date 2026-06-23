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

signals:
    void changed();
};

} // namespace fleet
