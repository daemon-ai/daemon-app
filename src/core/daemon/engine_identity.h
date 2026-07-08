// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QObject>
#include <QString>
#include <QVariantMap>

namespace profiles {
class IProfileStore;
}
namespace session {
class ISessionSettings;
}

namespace daemonapp::daemon {

class AgentRepository;

// Shared engine-identity presentation facade (C3/C4/C5 - wave2:app-engines). Resolves a session or
// profile to its engine {engine, agent, protocol, label}, joining the profile's engine selector
// (from the profile store) with the agent's protocol (from the foreign-agent catalog). `labelFor`
// is the SINGLE source of truth for how an engine reads on a chip, so GUI (ComposerControls,
// EngineOriginChip) and TUI (composer_chrome) render identically. It is deliberately callable with
// wire-sourced engine values too (app-delegation / integration reuse it for the fleet rows once
// they source engine identity from unit_node.engine).
class EngineIdentity : public QObject {
    Q_OBJECT

public:
    EngineIdentity(profiles::IProfileStore* profiles, session::ISessionSettings* sessionSettings,
                   AgentRepository* agents, QObject* parent = nullptr);

    // {engine: "Core"|"Foreign", agent: <name|"">, protocol: "Acp"|"StreamJson"|"", label: <text>}.
    // A native / unknown profile yields engine="Core" with empty agent/protocol/label.
    [[nodiscard]] Q_INVOKABLE QVariantMap engineForProfile(const QString& profileId) const;
    // Resolve a session to its profile (per-session override, else the node's default) then engine.
    [[nodiscard]] Q_INVOKABLE QVariantMap engineForSession(const QString& sessionId) const;
    // The agent's wire protocol from the catalog ("Acp"|"StreamJson"), or "" when unknown / no
    // repo.
    [[nodiscard]] Q_INVOKABLE QString protocolForAgent(const QString& agent) const;
    // The shared chip label for an engine triple. Native/unknown -> "" (callers render "Native" or
    // nothing); Foreign -> "<agent> · ACP" / "<agent> · stream-json" / "<agent>" (protocol
    // unknown).
    [[nodiscard]] Q_INVOKABLE QString labelFor(const QString& engine, const QString& agent,
                                               const QString& protocol) const;
    // A one-line setup summary composed from the SAME label rules as labelFor (the single source of
    // truth for how an engine reads), extended with the backend + inference dimensions so the
    // AgentSetupModel and the setup surfaces render one consistent descriptor. `backendMode` is
    // "AgentNative" | "NodeProvider" (Foreign only); `provider`/`model` are the inference selection
    // (Core, or the NodeProvider gateway target; for AgentNative `model` is the optional steer
    // hint). Composition is punctuation-only over labelFor + node-provided identifiers, so it adds
    // no new translatable phrasing here — the human-facing wording ("via node", "brings own model")
    // is layered on at the render surfaces (phase D/G) where it is translated.
    [[nodiscard]] Q_INVOKABLE QString summaryFor(const QString& engine, const QString& agent,
                                                 const QString& protocol,
                                                 const QString& backendMode,
                                                 const QString& provider,
                                                 const QString& model) const;

signals:
    // Re-emitted when any input (profiles, session settings, catalog) changes, so QML bindings that
    // call the invokables re-evaluate.
    void changed();

private:
    profiles::IProfileStore* m_profiles = nullptr;
    session::ISessionSettings* m_sessionSettings = nullptr;
    AgentRepository* m_agents = nullptr;
};

} // namespace daemonapp::daemon
