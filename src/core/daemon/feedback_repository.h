// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "feedback/ifeedback.h"

#include <QByteArray>
#include <QString>
#include <QVariantMap>

namespace daemonapp::daemon {

class NodeApiClient;

// Daemon-backed IFeedback (the "feedback over OpenTelemetry" seam). Encodes FeedbackSubmit /
// TelemetryConsentGet/Set over the shared NodeApiClient and caches the node-owned consent locally
// so telemetryEnabled() stays a synchronous read for the settings toggle. Explicit feedback is
// per-event consent: it is sent even when the toggle is off (the node queues + exports it).
// Replaces MockFeedback in ServiceMode::Daemon; the UI (GUI + TUI) binds the same IFeedback
// surface, so nothing above the seam changes.
class DaemonFeedback : public feedback::IFeedback {
    Q_OBJECT

public:
    explicit DaemonFeedback(NodeApiClient* client, QObject* parent = nullptr);

    void submitMessageFeedback(const QString& sessionId, const QVariantMap& anchor, int rating,
                               const QString& comment, bool includeContent) override;
    void submitAppFeedback(const QString& category, const QString& comment, bool includeDiagnostics,
                           bool alsoEnableTelemetry) override;

    [[nodiscard]] bool telemetryEnabled() const override { return m_telemetryEnabled; }
    void setTelemetryEnabled(bool enabled) override;

    [[nodiscard]] bool crashReportingEnabled() const override { return m_crashReportingEnabled; }
    void setCrashReportingEnabled(bool enabled) override;

    // Seed the cached consent from the node (TelemetryConsentGet + CrashConsentGet). The service
    // graph calls these on connect-ready so the settings toggles reflect the node's stored state.
    void refreshConsent();
    void refreshCrashConsent();

private:
    void handleResponse(const QString& correlationId, const QByteArray& responseCbor);
    void applyConsent(bool enabled);
    void applyCrashConsent(bool enabled);

    NodeApiClient* m_client = nullptr;
    bool m_telemetryEnabled = false;
    bool m_crashReportingEnabled = false;

    static constexpr auto kSubmitMessageCorrelation = "repo/feedback-submit-message";
    static constexpr auto kSubmitAppCorrelation = "repo/feedback-submit-app";
    static constexpr auto kConsentGetCorrelation = "repo/telemetry-consent-get";
    static constexpr auto kConsentSetCorrelation = "repo/telemetry-consent-set";
    static constexpr auto kCrashConsentGetCorrelation = "repo/crash-consent-get";
    static constexpr auto kCrashConsentSetCorrelation = "repo/crash-consent-set";
};

} // namespace daemonapp::daemon
