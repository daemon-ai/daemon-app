// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <functional>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

class QTimer;

namespace accounts {
class IAccountsService;
}
namespace models {
class IModelCatalog;
class IProviderCatalog;
} // namespace models
namespace profiles {
class IProfileStore;
}

namespace setup {

// Shared write-side view-model for agent/profile SETUP, bound identically by the GUI and the TUI
// (Phase C). It is the single owner of the "engine + backend + inference" pipeline that used to be
// scattered across FirstRunModel, the New-agent dialog, and the profile editor:
//
//   1. Engine   - Core (native) OR a Foreign catalog agent (with a node-derived status badge).
//   2. Backend  - Foreign only: AgentNative (the agent's own login/config; the node steers only its
//                 model) OR NodeProvider (the node routes the agent through its OpenAI gateway to a
//                 node provider+model; the credential stays node-side).
//   3. Inference - Core OR NodeProvider: the shared provider/model/credential sub-form.
//
// The pivotal predicate is `needsInference = engine==Core || backend==NodeProvider`; every derived
// property below flows from it so GUI and TUI never fork the decision.
//
// Node-authoritative: the model renders node-provided fields verbatim (verification badge, engine
// label, gateway state) and never re-derives domain rules client-side. Commit goes through the
// IProfileStore seam only (ProfileCreate/Update); the node validates and stamps provenance.
//
// The foreign-agent catalog rows and the EngineIdentity summariser are INJECTED by the service
// graph (setForeignAgents / setEngineIdentity), so this lib depends only on the core interface
// seams and stays free of the daemon layer (mirroring why DaemonProfileStore lives under daemon:
// keeping the layering acyclic - FirstRunModel delegates to this model, and the daemon graph
// constructs it).
class AgentSetupModel : public QObject {
    Q_OBJECT
    // --- selection state (mutated through the intents below) ---------------------------------
    Q_PROPERTY(QString engineKind READ engineKind NOTIFY stateChanged)     // "Core" | "Foreign"
    Q_PROPERTY(QString foreignAgent READ foreignAgent NOTIFY stateChanged) // catalog name (Foreign)
    // "AgentNative" | "NodeProvider" (Foreign only)
    Q_PROPERTY(QString backendMode READ backendMode NOTIFY stateChanged)
    Q_PROPERTY(QString providerId READ providerId NOTIFY stateChanged) // Core AND NodeProvider
    Q_PROPERTY(QString model READ model NOTIFY stateChanged)
    Q_PROPERTY(QString credentialRef READ credentialRef NOTIFY stateChanged)
    Q_PROPERTY(QString baseUrl READ baseUrl NOTIFY stateChanged)
    Q_PROPERTY(QString agentNativeModel READ agentNativeModel NOTIFY stateChanged) // steer hint
    // Whether the node's OpenAI gateway is enabled. Driven by the gateway surface (phase F); until
    // then it defaults to disabled so a NodeProvider selection surfaces the enable affordance.
    Q_PROPERTY(bool gatewayEnabled READ gatewayEnabled WRITE setGatewayEnabled NOTIFY stateChanged)
    // Edit mode: loadProfile() locks the engine (an existing profile's engine cannot change).
    Q_PROPERTY(bool editing READ editing NOTIFY stateChanged)
    Q_PROPERTY(bool engineLocked READ engineLocked NOTIFY stateChanged)

    // --- derived (the predicates that shape every setup surface) -----------------------------
    // Native row + foreign catalog rows: {kind, name, protocol, installed, verification, disabled}.
    Q_PROPERTY(QVariantList engineChoices READ engineChoices NOTIFY engineChoicesChanged)
    Q_PROPERTY(bool needsBackendChoice READ needsBackendChoice NOTIFY stateChanged)
    Q_PROPERTY(bool needsInference READ needsInference NOTIFY stateChanged) // THE unification
    Q_PROPERTY(bool needsKey READ needsKey NOTIFY keyGateChanged)
    Q_PROPERTY(bool gatewayRequired READ gatewayRequired NOTIFY stateChanged)
    Q_PROPERTY(bool engineReady READ engineReady NOTIFY stateChanged)
    Q_PROPERTY(bool backendReady READ backendReady NOTIFY stateChanged)
    Q_PROPERTY(bool inferenceReady READ inferenceReady NOTIFY stateChanged)
    Q_PROPERTY(bool commitReady READ commitReady NOTIFY stateChanged)
    Q_PROPERTY(QString summary READ summary NOTIFY stateChanged)

    // --- the hoisted key gate (shared by wizard + editor; FIX 4) -----------------------------
    Q_PROPERTY(bool keyGatePassed READ keyGatePassed NOTIFY keyGateChanged)
    Q_PROPERTY(QString keyGateMessage READ keyGateMessage NOTIFY keyGateChanged)

public:
    AgentSetupModel(profiles::IProfileStore* profiles, accounts::IAccountsService* accounts,
                    models::IProviderCatalog* providerCatalog, models::IModelCatalog* modelCatalog,
                    QObject* parent = nullptr);

    [[nodiscard]] QString engineKind() const { return m_engineKind; }
    [[nodiscard]] QString foreignAgent() const { return m_foreignAgent; }
    [[nodiscard]] QString backendMode() const { return m_backendMode; }
    [[nodiscard]] QString providerId() const { return m_providerId; }
    [[nodiscard]] QString model() const { return m_model; }
    [[nodiscard]] QString credentialRef() const { return m_credentialRef; }
    [[nodiscard]] QString baseUrl() const { return m_baseUrl; }
    [[nodiscard]] QString agentNativeModel() const { return m_agentNativeModel; }
    [[nodiscard]] bool gatewayEnabled() const { return m_gatewayEnabled; }
    [[nodiscard]] bool editing() const { return m_editing; }
    [[nodiscard]] bool engineLocked() const { return m_engineLocked; }

    [[nodiscard]] QVariantList engineChoices() const { return m_engineChoices; }
    [[nodiscard]] bool needsBackendChoice() const;
    [[nodiscard]] bool needsInference() const;
    [[nodiscard]] bool needsKey() const;
    [[nodiscard]] bool gatewayRequired() const;
    [[nodiscard]] bool engineReady() const;
    [[nodiscard]] bool backendReady() const;
    [[nodiscard]] bool inferenceReady() const;
    [[nodiscard]] bool commitReady() const;
    [[nodiscard]] QString summary() const;
    [[nodiscard]] bool keyGatePassed() const;
    [[nodiscard]] QString keyGateMessage() const { return m_keyGateMessage; }

    // --- intents ----------------------------------------------------------------------------
    // Choose the engine: kind "Core" (agent ignored) or "Foreign" with the catalog agent name.
    Q_INVOKABLE void chooseEngine(const QString& kind, const QString& agent = QString());
    // Foreign only: choose the backend mode ("AgentNative" | "NodeProvider").
    Q_INVOKABLE void chooseBackend(const QString& mode);
    // The shared inference sub-form selection (Core OR NodeProvider): provider selector id + model
    // + the entered key (transient; stored profile-scoped at commit, never persisted here) + an
    // optional custom base URL.
    Q_INVOKABLE void setInference(const QString& provider, const QString& model,
                                  const QString& key = QString(),
                                  const QString& baseUrl = QString());
    // The optional AgentNative model steer hint.
    Q_INVOKABLE void setAgentNativeModel(const QString& model);
    // Report the LIVE provider/key selection for the key gate (FIX 4): each report unconditionally
    // re-arms the gate; the next authenticated model listing for `providerId` with a non-empty key
    // on a key-requiring provider resolves it. The key is held transiently only.
    Q_INVOKABLE void setInferenceSelection(const QString& providerId, const QString& key);
    // Shared instrumentation seam for the wizard's blocking key check (both front ends funnel the
    // authenticated-listing outcome here so pass/block decisions share one trace point).
    Q_INVOKABLE void logKeyValidation(const QString& provider, bool requiresKey, int modelCount,
                                      bool pass) const;
    // Commit the current selection through the profile store: create the profile (+ profile-scoped
    // credential + make-default + placeholder cleanup), proving the provider serves models for a
    // Core/NodeProvider inference selection before signalling. Async -> committed(profileId) on
    // success, failed(message) on a probe/validation failure. `name` is the chosen display name
    // (empty or equal to the seeded placeholder configures the placeholder in place).
    Q_INVOKABLE void commit(const QString& name = QString());
    // Editor mode: hydrate the selection from an existing profile row. The engine is then LOCKED
    // (an existing profile's engine cannot change); backend/inference stay editable.
    Q_INVOKABLE void loadProfile(const QString& id);
    // Clear all selection state back to a fresh Core setup (also abandons any in-flight commit).
    Q_INVOKABLE void reset();

    // Abandon an in-flight commit continuation/probe WITHOUT clearing the selection. Called by
    // FirstRunModel when the wizard leaves a committable step (skip / restart / re-enter / finish),
    // so a stale continuation can never fire post-done. Public (not an intent) for that
    // coordinator.
    void cancelCommit();

    void setGatewayEnabled(bool enabled);
    // Push the foreign-agent catalog rows (the service graph wires this to AgentRepository's
    // catalogRefreshed). Each row: {name, protocol, installed, verification}. `engineChoices` is
    // recomposed (a Native row prepended) and the chosen agent's protocol is re-resolved.
    void setForeignAgents(const QVariantList& agents);
    // Inject the EngineIdentity facade (as a QObject*) used to compose `summary` through its shared
    // label rules; kept as a QObject* so this lib does not link the daemon layer.
    void setEngineIdentity(QObject* engineIdentity);

signals:
    void stateChanged();
    void engineChoicesChanged();
    void keyGateChanged();
    // A commit resolved: the new/updated profile id, or "" for the permissive (no-store) path.
    void committed(const QString& profileId);
    // A commit failed its provider probe / validation; the caller keeps the form open.
    void failed(const QString& message);

private:
    void emitState() { emit stateChanged(); }
    void rebuildEngineChoices();
    [[nodiscard]] bool providerRequiresKey(const QString& providerId) const;
    [[nodiscard]] QString protocolFor(const QString& agent) const;

    // The commit body, run on a fresh ProfileList reflection (Core inference path).
    void applyReflectedCore(const QString& name);
    // The commit body, run on a fresh ProfileList reflection (Foreign path).
    void applyReflectedForeign(const QString& name);
    // Prove the committed provider serves models before signalling committed(); permissive when no
    // catalog-backed provider is resolvable (custom endpoint / no catalog).
    void probeThenCommit(const QString& providerId, const QString& credentialRef,
                         const QString& committedId);
    void finishCommit(const QString& profileId);
    void failCommit(const QString& message);
    // ModelActivate a LOCALLY installed model for the explicit profile (no-op for cloud / no
    // catalog).
    void activateLocalModel(const QString& model, const QString& profileId);
    // Run `then` on the NEXT profile-store changed() (single-shot); abandons if the commit was
    // cancelled first.
    void runOnNextProfilesChanged(const std::function<void()>& then);
    // Run `then` as soon as `reflected` holds over the profile store (immediately for synchronous
    // stores, else on each changed() until it does); abandons if the commit was cancelled.
    void whenProfilesReflect(const std::function<bool()>& reflected,
                             const std::function<void()>& then);
    // Resolve the key gate off a provider's (authenticated) model listing.
    void resolveKeyGate(const QString& providerId);

    profiles::IProfileStore* m_profiles = nullptr;
    accounts::IAccountsService* m_accounts = nullptr;
    models::IProviderCatalog* m_providerCatalog = nullptr;
    models::IModelCatalog* m_modelCatalog = nullptr;
    QPointer<QObject> m_engineIdentity;

    QString m_engineKind = QStringLiteral("Core");
    QString m_foreignAgent;
    QString m_backendMode = QStringLiteral("AgentNative");
    QString m_providerId;
    QString m_model;
    QString m_credentialRef;
    QString m_baseUrl;
    QString m_agentNativeModel;
    bool m_gatewayEnabled = false;
    bool m_editing = false;
    bool m_engineLocked = false;

    QVariantList m_engineChoices;    // rebuilt from the injected foreign catalog + Native row
    QVariantList m_foreignAgentRows; // the last pushed foreign-agent catalog rows

    // True while a commit continuation is in flight, so a double-committed Finish cannot start a
    // second chain; also the abandon guard for the refetch continuations.
    bool m_applying = false;

    // The inference sub-form's entered key (transient): consumed at commit to store the credential
    // profile-scoped, never held as a persistent property.
    QString m_inferenceKey;
    // The key gate's reported selection (FIX 4). The key lives here transiently: consumed for the
    // authenticated listing only, never persisted or logged.
    QString m_gateProviderId;
    QString m_gateKey;
    bool m_keyProven = false;
    QString m_keyGateMessage;

    // The finish probe in flight (provider id being proven; empty = none) + its timeout/connection.
    QString m_probeProviderId;
    QString m_probeCommittedId; // the profile id to report on a successful probe
    QTimer* m_probeTimeout = nullptr;
    QMetaObject::Connection m_probeConnection;
};

} // namespace setup
