// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

class QTimer;

namespace auth {

class IAuthFlowService;
class RedirectSink;

// The SHARED interactive-auth view-model (AuthFlowSheet GUI / auth panel TUI bind the SAME
// instance): one begin -> browser hand-off -> await-redirect -> complete state machine over the
// IAuthFlowService seam. There is deliberately no "poll" op — the node parks the flow and the
// redirect (or a manual paste) resumes it; the awaiting phase is an await, not a loop.
//
//   idle -> beginning -> awaiting_browser -> completing -> success
//                 \          |    \                \-> failed (retryable)
//                  \-> failed \    \-> (cancel) -> cancelled
//                              \-> failed (TTL expired client-side)
//
// The redirect sink is best-effort: when the loopback listener cannot start (wasm, sandboxes,
// offscreen tests — which must never bind ports, so tests pass useSink=false), the flow runs
// URL-only and the user pastes the redirect via submitCallback(). Presentation-only: every
// decision (URL minting, exchange, credential storage) is the node's.
class AuthFlowController : public QObject {
    Q_OBJECT
    // "idle" | "beginning" | "awaiting_browser" | "completing" | "success" | "failed" |
    // "cancelled"
    Q_PROPERTY(QString phase READ phase NOTIFY phaseChanged)
    Q_PROPERTY(bool active READ active NOTIFY phaseChanged)
    Q_PROPERTY(QString authorizationUrl READ authorizationUrl NOTIFY flowChanged)
    Q_PROPERTY(QString flowKind READ flowKind NOTIFY flowChanged)
    // Whether the loopback sink is live (false => the paste field is the only completion path).
    Q_PROPERTY(bool sinkListening READ sinkListening NOTIFY flowChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    // Success facts (empty until phase == "success").
    Q_PROPERTY(QString accountLabel READ accountLabel NOTIFY completedChanged)
    Q_PROPERTY(QString credentialRef READ credentialRef NOTIFY completedChanged)

public:
    explicit AuthFlowController(IAuthFlowService* service, QObject* parent = nullptr);

    [[nodiscard]] QString phase() const { return m_phase; }
    [[nodiscard]] bool active() const {
        return m_phase != QStringLiteral("idle") && m_phase != QStringLiteral("success") &&
               m_phase != QStringLiteral("cancelled");
    }
    [[nodiscard]] QString authorizationUrl() const { return m_authorizationUrl; }
    [[nodiscard]] QString flowKind() const { return m_flowKind; }
    [[nodiscard]] bool sinkListening() const;
    [[nodiscard]] QString error() const { return m_error; }
    [[nodiscard]] QString accountLabel() const { return m_accountLabel; }
    [[nodiscard]] QString credentialRef() const { return m_credentialRef; }

    // The service's provider rows, passed through for the begin form (family + params schema).
    [[nodiscard]] Q_INVOKABLE QVariantList providers() const;
    Q_INVOKABLE void refreshProviders();

    // Start a flow: allocate the redirect sink (unless `useSink` is false — headless/offscreen
    // paths must never bind ports), then AuthBegin. `params` is the family-specific string map;
    // non-empty `bindProfile` binds the resulting account node-side.
    Q_INVOKABLE void start(const QString& family, const QVariantMap& params,
                           const QString& bindProfile = QString(), bool useSink = true);
    // Open the parked flow's authorization URL in the system browser (GUI hand-off). No-op
    // outside awaiting_browser.
    Q_INVOKABLE void openInBrowser();
    // Manual-callback completion (TUI / no-loopback): paste the redirect URL or query string.
    Q_INVOKABLE void submitCallback(const QString& callback);
    // Abandon the flow: drops the parked node flow (best-effort) and tears the sink down.
    Q_INVOKABLE void cancel();
    // Back to idle after success/failed/cancelled (the sheet's dismiss).
    Q_INVOKABLE void reset();

signals:
    void phaseChanged();
    void flowChanged();
    void errorChanged();
    void completedChanged();
    // Convenience for imperative consumers (the wizard advances on it; the TUI notifies).
    void succeeded(const QString& credentialRef, const QString& accountLabel,
                   const QString& transportInstance, const QString& boundProfile);

private:
    void setPhase(const QString& phase);
    void setError(const QString& error);
    void teardownFlow(bool cancelRemote);

    IAuthFlowService* m_service = nullptr;
    RedirectSink* m_sink = nullptr;
    QTimer* m_ttl = nullptr; // fires when the parked flow's expires_at passes client-side

    QString m_phase = QStringLiteral("idle");
    QString m_error;
    QString m_flowId;
    QString m_authorizationUrl;
    QString m_flowKind;
    QString m_accountLabel;
    QString m_credentialRef;
};

} // namespace auth
