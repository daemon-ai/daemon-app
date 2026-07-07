// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "feedback/mock_feedback.h"

#include "appcache/json_cache.h"

namespace feedback {

namespace {
const QString kCacheFile = QStringLiteral("feedback_consent.json");
const QString kConsentKey = QStringLiteral("telemetryEnabled");
} // namespace

MockFeedback::MockFeedback(QObject* parent) : IFeedback(parent) {
    // Restore the last-known consent (default off on a first run), so the seam is
    // the source of truth for the settings toggle and remembers it across restarts
    // exactly like the config mock it replaces did.
    const QJsonObject cached = appcache::loadObject(kCacheFile);
    m_telemetryEnabled = cached.value(kConsentKey).toBool(false);
}

void MockFeedback::persist() const {
    QJsonObject obj;
    obj.insert(kConsentKey, m_telemetryEnabled);
    appcache::saveObject(kCacheFile, obj);
}

void MockFeedback::submitMessageFeedback(const QString& sessionId, const QVariantMap& anchor,
                                         int rating, const QString& comment, bool includeContent) {
    m_messageRecords.push_back(MessageRecord{sessionId, anchor, rating, comment, includeContent});
    emit submitted(QStringLiteral("message"));
}

void MockFeedback::submitAppFeedback(const QString& category, const QString& comment,
                                     bool includeDiagnostics, bool alsoEnableTelemetry) {
    m_appRecords.push_back(AppRecord{category, comment, includeDiagnostics, alsoEnableTelemetry});
    // The dialog's explicit, default-unchecked opt-in is the only feedback path
    // allowed to enable telemetry (never a silent flip).
    if (alsoEnableTelemetry && !m_telemetryEnabled) {
        setTelemetryEnabled(true);
    }
    emit submitted(QStringLiteral("app"));
}

void MockFeedback::setTelemetryEnabled(bool enabled) {
    if (m_telemetryEnabled == enabled) {
        return;
    }
    m_telemetryEnabled = enabled;
    persist();
    emit telemetryEnabledChanged(m_telemetryEnabled);
}

} // namespace feedback
