// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "auth/iauth_flow_service.h"

namespace auth {

// Canned interactive-auth seam for the mock/standalone path + offscreen tests: begin() resolves
// asynchronously to a fake authorization URL and complete() to a stored-credential echo, so the
// AuthFlowController state machine and both front ends are testable with no node, no browser and
// no sockets. `failNext*` let tests drive the failure arms deterministically.
class MockAuthFlowService : public IAuthFlowService {
    Q_OBJECT

public:
    explicit MockAuthFlowService(QObject* parent = nullptr);

    [[nodiscard]] QVariantList providers() const override { return m_providers; }
    void refreshProviders() override;

    void begin(const QString& family, const QVariantMap& params, const QString& redirectUri,
               const QString& bindProfile = QString()) override;
    void complete(const QString& flowId, const QString& callback) override;
    void cancel(const QString& flowId) override;

    // Test hooks: make the next begin()/complete() fail with `message`.
    void failNextBegin(const QString& message) { m_failBegin = message; }
    void failNextComplete(const QString& message) { m_failComplete = message; }
    // The last cancelled flow id (empty = none), so tests can assert the remote drop happened.
    [[nodiscard]] QString lastCancelled() const { return m_lastCancelled; }
    // The TTL (unix seconds) the next begun() advertises; 0 = none (the default).
    void setNextExpiresAt(quint64 unixSeconds) { m_nextExpiresAt = unixSeconds; }

private:
    QVariantList m_providers;
    QString m_failBegin;
    QString m_failComplete;
    QString m_lastCancelled;
    QString m_pendingBind;
    quint64 m_nextExpiresAt = 0;
    quint64 m_flowSerial = 0;
};

} // namespace auth
