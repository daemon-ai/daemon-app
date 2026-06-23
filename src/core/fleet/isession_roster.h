#pragma once

#include <QObject>
#include <QString>

namespace fleet {

// Session roster seam backing the Sessions surface. Rows (VariantListModel):
// id, title, profile, state ("active"/"idle"/"suspended"), lifecycle
// ("live"/"durable"), lastActivity (human string), tokens (int), rewindable.
class ISessionRoster : public QObject {
    Q_OBJECT
    Q_PROPERTY(QObject* sessions READ sessions CONSTANT)
    Q_PROPERTY(int count READ count NOTIFY changed)

public:
    using QObject::QObject;
    ~ISessionRoster() override = default;

    [[nodiscard]] virtual QObject* sessions() const = 0;
    [[nodiscard]] virtual int count() const = 0;

    Q_INVOKABLE virtual void suspend(const QString& id) = 0;
    Q_INVOKABLE virtual void resume(const QString& id) = 0;
    Q_INVOKABLE virtual void close(const QString& id) = 0;

signals:
    void changed();
};

} // namespace fleet
