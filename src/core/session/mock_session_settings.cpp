#include "session/mock_session_settings.h"

namespace session {

MockSessionSettings::MockSessionSettings(QObject* parent) : ISessionSettings(parent) {}

const MockSessionSettings::Settings& MockSessionSettings::entry() const {
    return m_bySession[m_sessionId];
}

MockSessionSettings::Settings& MockSessionSettings::entry() {
    return m_bySession[m_sessionId];
}

void MockSessionSettings::setSessionId(const QString& id) {
    if (m_sessionId == id) {
        return;
    }
    m_sessionId = id;
    m_bySession[id]; // materialize defaults for a new chat
    emit sessionIdChanged();
    // The active override set changed wholesale; refresh every bound value.
    emit changed();
}

void MockSessionSettings::setProfile(const QString& profile) {
    if (entry().profile == profile) {
        return;
    }
    entry().profile = profile;
    emit changed();
}

void MockSessionSettings::setEffort(const QString& effort) {
    if (entry().effort == effort) {
        return;
    }
    entry().effort = effort;
    emit changed();
}

void MockSessionSettings::setFast(bool on) {
    if (entry().fast == on) {
        return;
    }
    entry().fast = on;
    emit changed();
}

void MockSessionSettings::setVerbose(bool on) {
    if (entry().verbose == on) {
        return;
    }
    entry().verbose = on;
    emit changed();
}

void MockSessionSettings::setApprovalMode(const QString& mode) {
    if (entry().approvalMode == mode) {
        return;
    }
    entry().approvalMode = mode;
    emit changed();
}

QStringList MockSessionSettings::approvalModeOptions() const {
    // Mirrors the daemon's approval-mode vocabulary (daemon-api approval-mode).
    return {QStringLiteral("ask"), QStringLiteral("accept_edits"), QStringLiteral("auto_allow"),
            QStringLiteral("deny")};
}

QStringList MockSessionSettings::effortOptions() const {
    // One reasoning-effort vocabulary across the whole app: the same off/low/
    // medium/high levels the composer's ModelPickerOverlay + ComposerSessionController
    // use, so the per-session popover and the inline modes never disagree.
    return {QStringLiteral("off"), QStringLiteral("low"), QStringLiteral("medium"),
            QStringLiteral("high")};
}

} // namespace session
