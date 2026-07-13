// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QObject>
#include <QString>
#include <QVariantMap>

namespace feedback {

// User-feedback seam (the "feedback over OpenTelemetry" feature). The node owns
// the telemetry-consent source of truth and receives explicit feedback events
// (thumbs up/down on an agent response, plus a general app-feedback entry). This
// is the transport-agnostic interface the UI binds; the in-repo MockFeedback is a
// dev stand-in that records calls in memory, and a daemon adapter (FeedbackSubmit
// / TelemetryConsentGet/Set, wire v31) replaces it in a later integration phase.
//
// Rating convention (kRatingUp / kRatingDown / kRatingNone): a plain int so QML
// and the TUI can pass it without an enum registration dance.
class IFeedback : public QObject {
    Q_OBJECT
    // Node-owned telemetry consent. Bound read/write by the settings consent row
    // (AdvancedSection.qml / hub_settings.cpp) and read by the app-feedback dialog
    // to decide whether to surface the "telemetry is off" disclosure. Never flip
    // it as a side effect except through the explicit opt-in on submitAppFeedback.
    Q_PROPERTY(bool telemetryEnabled READ telemetryEnabled WRITE setTelemetryEnabled NOTIFY
                   telemetryEnabledChanged)
    // Node-owned crash-reporting consent (wire v41; the dedicated "Send crash reports" toggle,
    // DISTINCT from telemetry above and default OFF). Bound read/write by the settings crash-report
    // row (AdvancedSection.qml / the TUI settings surface). On write the seam persists the node
    // value (CrashConsentSet) AND flips the local Sentry consent (crash::give/revokeConsent) +
    // caches it in QSettings so the reporter arms correctly at next startup before the node
    // connects.
    Q_PROPERTY(bool crashReportingEnabled READ crashReportingEnabled WRITE setCrashReportingEnabled
                   NOTIFY crashReportingEnabledChanged)

public:
    static constexpr int kRatingNone = 0;
    static constexpr int kRatingUp = 1;
    static constexpr int kRatingDown = -1;

    using QObject::QObject;
    ~IFeedback() override = default;

    // Per-response feedback: `anchor` is the wire reference the FeedbackController
    // resolved from the message's block metadata (session + turn seq / journal
    // cursor). `rating` is kRatingUp / kRatingDown; `comment` is optional.
    // `includeContent` is the submitter's per-event consent to attach the rated
    // response text to the exported event (default off / privacy-first) so a thumb
    // is self-describing rather than a bare anchor; the node reads the text from its
    // journal at the anchor cursor. Explicit feedback is sent even when telemetry is
    // off (per-event consent).
    Q_INVOKABLE virtual void submitMessageFeedback(const QString& sessionId,
                                                   const QVariantMap& anchor, int rating,
                                                   const QString& comment, bool includeContent) = 0;
    // General app feedback (status-bar entry). `includeDiagnostics` attaches app
    // version + OS; `alsoEnableTelemetry` is the dialog's default-unchecked opt-in
    // and is the ONLY path that may turn telemetry on from a feedback flow.
    Q_INVOKABLE virtual void submitAppFeedback(const QString& category, const QString& comment,
                                               bool includeDiagnostics,
                                               bool alsoEnableTelemetry) = 0;

    [[nodiscard]] virtual bool telemetryEnabled() const = 0;
    virtual void setTelemetryEnabled(bool enabled) = 0;

    [[nodiscard]] virtual bool crashReportingEnabled() const = 0;
    virtual void setCrashReportingEnabled(bool enabled) = 0;

signals:
    void telemetryEnabledChanged(bool enabled);
    void crashReportingEnabledChanged(bool enabled);
    // Emitted after a submission is recorded/acknowledged (`kind` is "message" or
    // "app") so the UI can show a transient acknowledgment.
    void submitted(const QString& kind);
};

} // namespace feedback
