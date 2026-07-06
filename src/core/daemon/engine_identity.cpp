// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "daemon/engine_identity.h"

#include "daemon/repositories.h"
#include "profiles/iprofile_store.h"
#include "session/isession_settings.h"

namespace daemonapp::daemon {

EngineIdentity::EngineIdentity(profiles::IProfileStore* profiles,
                               session::ISessionSettings* sessionSettings, AgentRepository* agents,
                               QObject* parent)
    : QObject(parent), m_profiles(profiles), m_sessionSettings(sessionSettings), m_agents(agents) {
    if (m_profiles != nullptr) {
        connect(m_profiles, &profiles::IProfileStore::changed, this, &EngineIdentity::changed);
    }
    if (m_sessionSettings != nullptr) {
        connect(m_sessionSettings, &session::ISessionSettings::changed, this,
                &EngineIdentity::changed);
    }
    if (m_agents != nullptr) {
        connect(m_agents, &AgentRepository::catalogRefreshed, this, &EngineIdentity::changed);
    }
}

QString EngineIdentity::protocolForAgent(const QString& agent) const {
    if (m_agents == nullptr || agent.isEmpty()) {
        return {};
    }
    for (const DecodedAgentEntry& e : m_agents->entries()) {
        if (e.name == agent) {
            return e.protocol;
        }
    }
    return {};
}

QString EngineIdentity::labelFor(const QString& engine, const QString& agent,
                                 const QString& protocol) const {
    if (engine != QStringLiteral("Foreign")) {
        return {}; // native / unknown: no chip text
    }
    const QString name = agent.isEmpty() ? tr("Foreign") : agent;
    if (protocol == QStringLiteral("StreamJson")) {
        return tr("%1 · stream-json").arg(name);
    }
    if (protocol == QStringLiteral("Acp")) {
        return tr("%1 · ACP").arg(name);
    }
    return name; // protocol unknown (catalog not loaded yet): the agent name alone
}

QVariantMap EngineIdentity::engineForProfile(const QString& profileId) const {
    QVariantMap out;
    out[QStringLiteral("engine")] = QStringLiteral("Core");
    out[QStringLiteral("agent")] = QString();
    out[QStringLiteral("protocol")] = QString();
    out[QStringLiteral("label")] = QString();
    if (m_profiles == nullptr || profileId.isEmpty()) {
        return out;
    }
    const QVariantMap p = m_profiles->profile(profileId);
    if (p.isEmpty()) {
        return out;
    }
    const QString engine = p.value(QStringLiteral("engine"), QStringLiteral("Core")).toString();
    // The presentation row key stays `acpAgent` (the foreign agent name) - see
    // daemon_profile_store.
    const QString agent = p.value(QStringLiteral("acpAgent")).toString();
    const QString protocol = protocolForAgent(agent);
    out[QStringLiteral("engine")] = engine;
    out[QStringLiteral("agent")] = agent;
    out[QStringLiteral("protocol")] = protocol;
    out[QStringLiteral("label")] = labelFor(engine, agent, protocol);
    return out;
}

QVariantMap EngineIdentity::engineForSession(const QString& sessionId) const {
    QString pid;
    if (m_sessionSettings != nullptr) {
        pid = m_sessionSettings->profileFor(sessionId);
    }
    if (pid.isEmpty() && m_profiles != nullptr) {
        pid = m_profiles->defaultProfileId();
    }
    return engineForProfile(pid);
}

} // namespace daemonapp::daemon
