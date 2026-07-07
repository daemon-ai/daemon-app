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

// [waveB:app-v31] The SHARED interactive-auth view-model (AuthFlowSheet GUI / auth panel TUI bind
// the SAME instance): a challenge/response state machine over the IAuthFlowService seam. A flow is
// a sequence of challenges — Redirect (open a URL + capture the callback), Form (collect fields),
// Qr (render a payload/image + poll) or Message (show text + poll) — each answered with an
// AuthStepInput (Callback / Fields / Poll) until the node returns a completion.
//
//   idle -> beginning -> challenge <-> stepping -> success
//                 \          |    \                 \-> failed (retryable)
//                  \-> failed \    \-> (cancel) -> cancelled
//                              \-> failed (TTL expired client-side)
//
// The redirect sink is best-effort: when the loopback listener cannot start (wasm, sandboxes,
// offscreen tests — which must never bind ports, so tests pass useSink=false), a Redirect challenge
// runs URL-only and the user pastes the redirect via submitCallback(). Qr/Message challenges
// auto-poll (submitStep::Poll) on their advertised interval. Presentation-only: every decision (URL
// minting, field validation, exchange, credential storage) is the node's.
class AuthFlowController : public QObject {
    Q_OBJECT
    // "idle" | "beginning" | "challenge" | "stepping" | "success" | "failed" | "cancelled"
    Q_PROPERTY(QString phase READ phase NOTIFY phaseChanged)
    Q_PROPERTY(bool active READ active NOTIFY phaseChanged)
    // The current challenge kind: "" | "redirect" | "form" | "qr" | "message".
    Q_PROPERTY(QString challengeKind READ challengeKind NOTIFY challengeChanged)
    // Redirect: the URL to open in a browser.
    Q_PROPERTY(QString authorizationUrl READ authorizationUrl NOTIFY challengeChanged)
    // Form: a human title + the fields to collect ([{ key, label, required }]).
    Q_PROPERTY(QString formTitle READ formTitle NOTIFY challengeChanged)
    Q_PROPERTY(QVariantList formFields READ formFields NOTIFY challengeChanged)
    // Qr: the payload to render, an optional pre-rendered image (as a data: URI for Image.source;
    // empty = render `qrPayload` yourself), and the client re-poll cadence (ms).
    Q_PROPERTY(QString qrPayload READ qrPayload NOTIFY challengeChanged)
    Q_PROPERTY(QString qrImageSource READ qrImageSource NOTIFY challengeChanged)
    Q_PROPERTY(int pollIntervalMs READ pollIntervalMs NOTIFY challengeChanged)
    // Message: the text to show.
    Q_PROPERTY(QString messageText READ messageText NOTIFY challengeChanged)
    // Flow TTL (unix seconds; 0 = none advertised).
    Q_PROPERTY(quint64 expiresAt READ expiresAt NOTIFY challengeChanged)
    // Whether the loopback sink is live (false => the paste field is the only Redirect completion
    // path).
    Q_PROPERTY(bool sinkListening READ sinkListening NOTIFY challengeChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    // Success facts (empty until phase == "success").
    Q_PROPERTY(QString accountLabel READ accountLabel NOTIFY completedChanged)
    Q_PROPERTY(QString credentialRef READ credentialRef NOTIFY completedChanged)

public:
    explicit AuthFlowController(IAuthFlowService* service, QObject* parent = nullptr);

    [[nodiscard]] QString phase() const { return m_phase; }
    [[nodiscard]] bool active() const {
        return m_phase != QStringLiteral("idle") && m_phase != QStringLiteral("success") &&
               m_phase != QStringLiteral("cancelled") && m_phase != QStringLiteral("failed");
    }
    [[nodiscard]] QString challengeKind() const { return m_challengeKind; }
    [[nodiscard]] QString authorizationUrl() const { return m_authorizationUrl; }
    [[nodiscard]] QString formTitle() const { return m_formTitle; }
    [[nodiscard]] QVariantList formFields() const { return m_formFields; }
    [[nodiscard]] QString qrPayload() const { return m_qrPayload; }
    [[nodiscard]] QString qrImageSource() const { return m_qrImageSource; }
    [[nodiscard]] int pollIntervalMs() const { return m_pollIntervalMs; }
    [[nodiscard]] QString messageText() const { return m_messageText; }
    [[nodiscard]] quint64 expiresAt() const { return m_expiresAt; }
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
    // Open the current Redirect challenge's authorization URL in the system browser (GUI hand-off).
    // No-op outside a Redirect challenge.
    Q_INVOKABLE void openInBrowser();

    // The three submitStep entry points — each maps UI input onto one AuthStepInput arm and drives
    // the service's step():
    //   submitCallback -> AuthStepInput::Callback (answers a Redirect; also the sink's capture
    //   sink) submitFields   -> AuthStepInput::Fields   (answers a Form) poll           ->
    //   AuthStepInput::Poll     (answers a Qr / Message)
    Q_INVOKABLE void submitCallback(const QString& callback);
    Q_INVOKABLE void submitFields(const QVariantMap& values);
    Q_INVOKABLE void poll();

    // Abandon the flow: drops the parked node flow (best-effort) and tears the sink down.
    Q_INVOKABLE void cancel();
    // Back to idle after success/failed/cancelled (the sheet's dismiss).
    Q_INVOKABLE void reset();

signals:
    void phaseChanged();
    void challengeChanged();
    void errorChanged();
    void completedChanged();
    // Convenience for imperative consumers (the wizard advances on it; the TUI notifies).
    void succeeded(const QString& credentialRef, const QString& accountLabel,
                   const QString& transportInstance, const QString& boundProfile);

private:
    void setPhase(const QString& phase);
    void setError(const QString& error);
    void teardownFlow(bool cancelRemote);
    // Apply an incoming challenge map (from begun/challenged): populate the per-kind fields, arm
    // the auto-poll for Qr/Message, and move to the "challenge" phase.
    void applyChallenge(const QVariantMap& challenge);
    void clearChallenge();
    // True while a begin/step round-trip is in flight or a challenge is being presented.
    [[nodiscard]] bool inFlight() const;

    IAuthFlowService* m_service = nullptr;
    RedirectSink* m_sink = nullptr;
    QTimer* m_ttl = nullptr;  // fires when the parked flow's expires_at passes client-side
    QTimer* m_poll = nullptr; // drives the Poll step for Qr/Message challenges

    QString m_phase = QStringLiteral("idle");
    QString m_error;
    QString m_flowId;
    quint64 m_expiresAt = 0;
    // Current challenge state.
    QString m_challengeKind;
    QString m_authorizationUrl;
    QString m_formTitle;
    QVariantList m_formFields;
    QString m_qrPayload;
    QString m_qrImageSource;
    int m_pollIntervalMs = 0;
    QString m_messageText;
    // Success facts.
    QString m_accountLabel;
    QString m_credentialRef;
};

} // namespace auth
