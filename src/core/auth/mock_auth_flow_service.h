// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "auth/iauth_flow_service.h"

#include <QHash>

namespace auth {

// Canned interactive-auth seam for the mock/standalone path + offscreen tests. Simulates the wire
// v31 challenge/response state machine end-to-end with no node, no browser and no sockets, so the
// AuthFlowController and both front ends exercise the real multi-step path — covering the full
// challenge matrix (redirect / form / message / qr; [integrations wire v38]):
//   - the "matrix" family is a single-step Redirect flow: begin -> Redirect challenge, then a
//     Callback step completes it (the SSO analogue).
//   - the "token" family is a multi-step flow: begin -> Form challenge (a masked token field with
//     the enriched v38 field metadata), a Fields step advances to a Message challenge ("approve on
//     your other device"), and a Poll step then completes it — exercising begun(), challenged()
//     and completed() and all three input arms.
//   - the "qr" family is a device-pairing flow: begin -> Qr challenge (payload + poll cadence),
//     and the first Poll step completes it — the offline target for A3's QR surfaces.
// `failNext*` let tests drive the failure arms deterministically.
class MockAuthFlowService : public IAuthFlowService {
    Q_OBJECT

public:
    explicit MockAuthFlowService(QObject* parent = nullptr);

    [[nodiscard]] QVariantList providers() const override { return m_providers; }
    void refreshProviders() override;

    void begin(const QString& family, const QVariantMap& params, const QString& redirectUri,
               const QString& bindProfile = QString()) override;
    void step(const QString& flowId, const StepInput& input) override;
    void cancel(const QString& flowId) override;

    // Test hooks: make the next begin()/step() fail with `message`.
    void failNextBegin(const QString& message) { m_failBegin = message; }
    void failNextStep(const QString& message) { m_failStep = message; }
    // The last cancelled flow id (empty = none), so tests can assert the remote drop happened.
    [[nodiscard]] QString lastCancelled() const { return m_lastCancelled; }
    // The TTL (unix seconds) the next begun() advertises; 0 = none (the default).
    void setNextExpiresAt(quint64 unixSeconds) { m_nextExpiresAt = unixSeconds; }

private:
    // The parked per-flow state the mock threads across steps.
    struct FlowState {
        QString family;
        QString bindProfile;
        int stepsTaken = 0;
    };

    QVariantList m_providers;
    QString m_failBegin;
    QString m_failStep;
    QString m_lastCancelled;
    quint64 m_nextExpiresAt = 0;
    quint64 m_flowSerial = 0;
    QHash<QString, FlowState> m_flows;
};

} // namespace auth
