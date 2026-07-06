// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

namespace auth {

// Interactive-auth seam (begin -> browser hand-off -> complete) over the node's generic
// AuthProviders/AuthBegin/AuthComplete contract. One begin/complete flow serves BOTH provider
// OAuth (AuthFlowKind::OAuth2Pkce) and transport SSO (AuthFlowKind::MatrixSso) — the family is a
// parameter, never a fork. The daemon adapter (src/core/daemon/daemon_auth_flow_service) maps
// these onto AuthRepository; the mock simulates the round-trip for offscreen tests and the
// standalone path. Signals carry plain values (never wire types) so this seam stays free of the
// daemon codec — the announced Auth*/Acp* op renames touch only the codec + repositories.
//
// Distinct from IAccountsService (account rows + API-key storage) and from the SASL connection
// handshake (node_api_auth): this seam drives a browser round-trip that ends in a node-stored
// credential; it never sees a secret itself.
class IAuthFlowService : public QObject {
    Q_OBJECT

public:
    using QObject::QObject;
    ~IAuthFlowService() override = default;

    // The families supporting interactive auth, each a row:
    //   { family, flowKind ("MatrixSso"|"OAuth2Pkce"), name,
    //     params: [ { key, label, required (bool) } ] }
    // Empty until refreshProviders() resolves (or the mock seeds).
    [[nodiscard]] Q_INVOKABLE virtual QVariantList providers() const = 0;

    // Kick a provider-list fetch; providersChanged() fires when the list is ready.
    Q_INVOKABLE virtual void refreshProviders() = 0;

    // Begin a flow for `family` with the family-specific `params` (string map, e.g. Matrix SSO's
    // {homeserver}) against the CLIENT-owned `redirectUri`. A non-empty `bindProfile` asks the
    // node to bind the resulting account to that profile on success. Resolves asynchronously to
    // begun() or flowFailed("begin", ...).
    virtual void begin(const QString& family, const QVariantMap& params, const QString& redirectUri,
                       const QString& bindProfile = QString()) = 0;

    // Finish `flowId` from the captured redirect (`callback`: the full redirect URL or its query
    // string). Resolves to completed() or flowFailed("complete", ...). The flow is single-use.
    virtual void complete(const QString& flowId, const QString& callback) = 0;

    // Drop a pending flow early (user cancelled / TTL expired client-side). Best-effort.
    virtual void cancel(const QString& flowId) = 0;

signals:
    void providersChanged();
    // A flow is parked node-side; open `authorizationUrl` in the user's browser. `expiresAt` is
    // unix seconds (0 = no TTL advertised); `flowKind` is the wire kind label.
    void begun(const QString& flowId, const QString& authorizationUrl, const QString& redirectUri,
               quint64 expiresAt, const QString& flowKind);
    // The exchange finished: the credential blob landed node-side under `credentialRef` and the
    // account resolved to `accountLabel` / `transportInstance` (`boundProfile` non-empty when the
    // begin's bind was honored). No secret ever crosses this seam.
    void completed(const QString& credentialRef, const QString& accountLabel,
                   const QString& transportInstance, const QString& boundProfile);
    // `phase` is "providers" | "begin" | "complete"; `message` is actionable.
    void flowFailed(const QString& phase, const QString& message);
};

} // namespace auth
