// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "feedback/ifeedback.h"

#include <QVector>

namespace feedback {

// In-memory IFeedback stand-in: records every submission and holds the telemetry
// consent locally (persisted to a small on-disk cache so the preference survives
// a restart, mirroring the old MockDaemonConfig `advanced/telemetry` behaviour it
// takes over as the source of truth). Introspectable so unit tests can assert what
// crossed the seam. A daemon adapter replaces this later (FeedbackSubmit /
// TelemetryConsentGet/Set) without any UI change.
class MockFeedback : public IFeedback {
    Q_OBJECT

public:
    struct MessageRecord {
        QString sessionId;
        QVariantMap anchor;
        int rating = kRatingNone;
        QString comment;
        bool includeContent = false;
    };
    struct AppRecord {
        QString category;
        QString comment;
        bool includeDiagnostics = false;
        bool alsoEnableTelemetry = false;
    };

    explicit MockFeedback(QObject* parent = nullptr);

    void submitMessageFeedback(const QString& sessionId, const QVariantMap& anchor, int rating,
                               const QString& comment, bool includeContent) override;
    void submitAppFeedback(const QString& category, const QString& comment, bool includeDiagnostics,
                           bool alsoEnableTelemetry) override;

    [[nodiscard]] bool telemetryEnabled() const override { return m_telemetryEnabled; }
    void setTelemetryEnabled(bool enabled) override;

    // Test introspection.
    [[nodiscard]] const QVector<MessageRecord>& messageRecords() const { return m_messageRecords; }
    [[nodiscard]] const QVector<AppRecord>& appRecords() const { return m_appRecords; }

private:
    void persist() const;

    bool m_telemetryEnabled = false;
    QVector<MessageRecord> m_messageRecords;
    QVector<AppRecord> m_appRecords;
};

} // namespace feedback
