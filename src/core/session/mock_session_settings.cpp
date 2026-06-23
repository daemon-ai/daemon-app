#include "session/mock_session_settings.h"

namespace session {

MockSessionSettings::MockSessionSettings(QObject* parent)
    : ISessionSettings(parent)
{
}

const MockSessionSettings::Settings& MockSessionSettings::entry() const
{
    return m_byConversation[m_conversationId];
}

MockSessionSettings::Settings& MockSessionSettings::entry()
{
    return m_byConversation[m_conversationId];
}

void MockSessionSettings::setConversationId(int id)
{
    if (m_conversationId == id) {
        return;
    }
    m_conversationId = id;
    m_byConversation[id]; // materialize defaults for a new chat
    emit conversationIdChanged();
    // The active override set changed wholesale; refresh every bound value.
    emit changed();
}

void MockSessionSettings::setProfile(const QString& profile)
{
    if (entry().profile == profile) {
        return;
    }
    entry().profile = profile;
    emit changed();
}

void MockSessionSettings::setEffort(const QString& effort)
{
    if (entry().effort == effort) {
        return;
    }
    entry().effort = effort;
    emit changed();
}

void MockSessionSettings::setFast(bool on)
{
    if (entry().fast == on) {
        return;
    }
    entry().fast = on;
    emit changed();
}

void MockSessionSettings::setVerbose(bool on)
{
    if (entry().verbose == on) {
        return;
    }
    entry().verbose = on;
    emit changed();
}

QStringList MockSessionSettings::effortOptions() const
{
    // One reasoning-effort vocabulary across the whole app: the same off/low/
    // medium/high levels the composer's ModelPickerOverlay + ComposerSessionController
    // use, so the per-session popover and the inline modes never disagree.
    return { QStringLiteral("off"), QStringLiteral("low"), QStringLiteral("medium"),
             QStringLiteral("high") };
}

} // namespace session
