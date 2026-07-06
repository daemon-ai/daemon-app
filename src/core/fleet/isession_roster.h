// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

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
    // The listing scope (F6/DEL-6): "active" (the default roster) or "archived" (archived
    // sessions with a per-row Restore). Setting it re-projects the rows; the daemon seam also
    // re-fetches the archived scope from the node (the TopLevel roster excludes it).
    Q_PROPERTY(QString scope READ scope WRITE setScope NOTIFY scopeChanged)

public:
    using QObject::QObject;
    ~ISessionRoster() override = default;

    [[nodiscard]] virtual QObject* sessions() const = 0;
    [[nodiscard]] virtual int count() const = 0;
    [[nodiscard]] virtual QString scope() const { return QStringLiteral("active"); }
    virtual void setScope(const QString& scope) { Q_UNUSED(scope) }

    Q_INVOKABLE virtual void suspend(const QString& id) = 0;
    Q_INVOKABLE virtual void resume(const QString& id) = 0;
    Q_INVOKABLE virtual void close(const QString& id) = 0;
    // Restore an archived session (archived: false). Default no-op for seams without archive.
    Q_INVOKABLE virtual void restore(const QString& id) { Q_UNUSED(id) }

    // --- Operator steer/cancel (F4/DEL-4) -----------------------------------------------------
    // Session-addressable Submit to ANY session id (delegated children included). The caller
    // picks the verb from its row state: steer() nudges a RUNNING turn (Submit{Steer});
    // startTurn() starts one on an idle session (Submit{StartTurn}); interrupt() cancels
    // (Submit{Interrupt}). Defaults no-op: only the daemon seam has the wire; a rejection
    // surfaces via operationFailed.
    Q_INVOKABLE virtual void steer(const QString& sessionId, const QString& text) {
        Q_UNUSED(sessionId)
        Q_UNUSED(text)
    }
    Q_INVOKABLE virtual void startTurn(const QString& sessionId, const QString& text) {
        Q_UNUSED(sessionId)
        Q_UNUSED(text)
    }
    Q_INVOKABLE virtual void interrupt(const QString& sessionId) { Q_UNUSED(sessionId) }

signals:
    void changed();
    void scopeChanged();
    // An operator steer/interrupt (or restore) was rejected by the node / failed at the
    // transport. The UI toasts it - never a silent no-op.
    void operationFailed(const QString& message);
};

} // namespace fleet
