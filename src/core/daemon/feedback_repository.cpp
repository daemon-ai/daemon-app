// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemon/feedback_repository.h"

#include "crash/crash_reporter.h"
#include "daemon/node_api_client.h"
#include "daemon/node_api_codec.h"

#include <QCoreApplication>
#include <QSysInfo>
#include <QVariant>

namespace daemonapp::daemon {

DaemonFeedback::DaemonFeedback(NodeApiClient* client, QObject* parent)
    : feedback::IFeedback(parent), m_client(client) {
    if (m_client != nullptr) {
        connect(m_client, &NodeApiClient::responseReady, this, &DaemonFeedback::handleResponse);
    }
}

void DaemonFeedback::submitMessageFeedback(const QString& sessionId, const QVariantMap& anchor,
                                           int rating, const QString& comment,
                                           bool includeContent) {
    if (m_client == nullptr) {
        return;
    }
    FeedbackSubmitInput in;
    in.isResponse = true;
    in.session = sessionId;
    // The message's wire anchor carries the rated turn's journal cursor (fallback: the turn's event
    // seq) + an optional trace id. The node reads the descriptor + rated text from the journal at
    // this cursor; an unknown/zero cursor simply yields no enrichment (best-effort).
    const QVariant cursor = anchor.value(QStringLiteral("journalCursor"));
    in.cursor = cursor.isValid() ? cursor.toULongLong()
                                 : anchor.value(QStringLiteral("turnSeq")).toULongLong();
    bool traceOk = false;
    const quint64 trace = anchor.value(QStringLiteral("traceId")).toULongLong(&traceOk);
    in.hasTrace = traceOk && trace != 0;
    in.trace = trace;
    in.rating = rating; // IFeedback::kRatingNone/Up/Down == 0/1/-1
    in.hasComment = !comment.isEmpty();
    in.comment = comment;
    in.includeContent = includeContent;
    in.surface = QStringLiteral("transcript");
    m_client->sendRequest(NodeApiCodec::encodeFeedbackSubmitRequest(in),
                          QLatin1String(kSubmitMessageCorrelation));
}

void DaemonFeedback::submitAppFeedback(const QString& category, const QString& comment,
                                       bool includeDiagnostics, bool alsoEnableTelemetry) {
    if (m_client == nullptr) {
        return;
    }
    FeedbackSubmitInput in;
    in.isResponse = false;
    in.hasComment = !comment.isEmpty();
    in.comment = comment;
    in.surface = category.isEmpty() ? QStringLiteral("app") : category;
    if (includeDiagnostics) {
        in.hasDiagnostics = true;
        in.appVersion = QCoreApplication::applicationVersion();
        in.os = QSysInfo::prettyProductName();
    }
    m_client->sendRequest(NodeApiCodec::encodeFeedbackSubmitRequest(in),
                          QLatin1String(kSubmitAppCorrelation));
    // The dialog's explicit, default-unchecked opt-in is the ONLY feedback path allowed to enable
    // telemetry — never a silent flip.
    if (alsoEnableTelemetry && !m_telemetryEnabled) {
        setTelemetryEnabled(true);
    }
}

void DaemonFeedback::setTelemetryEnabled(bool enabled) {
    if (m_client == nullptr) {
        return;
    }
    // The node echoes the resulting state in TelemetryConsent; applyConsent() emits the change when
    // it lands (no optimistic local flip).
    m_client->sendRequest(NodeApiCodec::encodeTelemetryConsentSetRequest(enabled),
                          QLatin1String(kConsentSetCorrelation));
}

void DaemonFeedback::setCrashReportingEnabled(bool enabled) {
    // Unlike telemetry, flip the LOCAL reporter + persist the cache IMMEDIATELY (no wait for a node
    // echo): the app-side Sentry client must honor the choice even before/without a node
    // round-trip, and must arm correctly at the next startup before any connection exists. Then
    // inform the node (best-effort) so future worker spawns inherit the decision.
    applyCrashConsent(enabled);
    if (m_client != nullptr) {
        m_client->sendRequest(NodeApiCodec::encodeCrashConsentSetRequest(enabled),
                              QLatin1String(kCrashConsentSetCorrelation));
    }
}

void DaemonFeedback::refreshConsent() {
    if (m_client == nullptr) {
        return;
    }
    m_client->sendRequest(NodeApiCodec::encodeTelemetryConsentGetRequest(),
                          QLatin1String(kConsentGetCorrelation));
}

void DaemonFeedback::refreshCrashConsent() {
    if (m_client == nullptr) {
        return;
    }
    m_client->sendRequest(NodeApiCodec::encodeCrashConsentGetRequest(),
                          QLatin1String(kCrashConsentGetCorrelation));
}

void DaemonFeedback::handleResponse(const QString& correlationId, const QByteArray& responseCbor) {
    if (correlationId == QLatin1String(kConsentGetCorrelation) ||
        correlationId == QLatin1String(kConsentSetCorrelation)) {
        bool enabled = m_telemetryEnabled;
        if (NodeApiCodec::decodeTelemetryConsent(responseCbor, &enabled)) {
            applyConsent(enabled);
        }
        return;
    }
    if (correlationId == QLatin1String(kCrashConsentGetCorrelation) ||
        correlationId == QLatin1String(kCrashConsentSetCorrelation)) {
        bool enabled = m_crashReportingEnabled;
        if (NodeApiCodec::decodeCrashConsent(responseCbor, &enabled)) {
            applyCrashConsent(enabled);
        }
        return;
    }
    if (correlationId == QLatin1String(kSubmitMessageCorrelation) ||
        correlationId == QLatin1String(kSubmitAppCorrelation)) {
        bool accepted = false;
        bool queued = false;
        if (NodeApiCodec::decodeFeedbackAck(responseCbor, &accepted, &queued) && accepted) {
            emit submitted(correlationId == QLatin1String(kSubmitMessageCorrelation)
                               ? QStringLiteral("message")
                               : QStringLiteral("app"));
        }
        return;
    }
}

void DaemonFeedback::applyConsent(bool enabled) {
    if (m_telemetryEnabled == enabled) {
        return;
    }
    m_telemetryEnabled = enabled;
    emit telemetryEnabledChanged(m_telemetryEnabled);
}

void DaemonFeedback::applyCrashConsent(bool enabled) {
    // Flip the live Sentry consent gate + cache it in QSettings (idempotent; also arms the next
    // startup). Done regardless of whether the tracked value changed, so a node echo that matches
    // still guarantees the local reporter + cache are in the granted/revoked state.
    crash::persistConsent(enabled);
    if (m_crashReportingEnabled == enabled) {
        return;
    }
    m_crashReportingEnabled = enabled;
    emit crashReportingEnabledChanged(m_crashReportingEnabled);
}

} // namespace daemonapp::daemon
