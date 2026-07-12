// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

namespace auth {

// [waveB:app-v31] The client's response to a challenge, mapped onto the wire AuthStepInput. `kind`
// picks the arm: Fields carries the filled key->value `fields` (answers a Form), Callback carries
// the captured redirect URL/query `callback` (answers a Redirect), Poll carries nothing (answers a
// Qr / Message — "has the pairing/approval landed yet?"). Kept a plain POD so the seam stays free
// of the daemon codec.
enum class StepInputKind {
    Fields,
    Callback,
    Poll,
};
struct StepInput {
    StepInputKind kind = StepInputKind::Poll;
    QVariantMap fields; // valid for Fields
    QString callback;   // valid for Callback
};

// Interactive-auth seam (begin -> challenge/response state machine -> complete) over the node's
// generic AuthProviders/AuthBegin/AuthStep contract (wire v31). One begin/step flow serves BOTH
// provider OAuth (AuthFlowKind::OAuth2Pkce) and transport SSO (AuthFlowKind::MatrixSso) and every
// non-redirect kind (form/QR/message) — the family is a parameter, never a fork. The daemon adapter
// (src/core/daemon/daemon_auth_flow_service) maps these onto AuthRepository; the mock simulates the
// round-trip for offscreen tests and the standalone path. Signals carry plain values (a challenge
// is a QVariantMap, never a wire type) so this seam stays free of the daemon codec — the Auth*/Acp*
// op names touch only the codec + repositories.
//
// A challenge QVariantMap (carried by begun()/challenged()) has the shape:
//   { kind: "redirect"|"form"|"qr"|"message",
//     authorizationUrl:  string  (redirect),
//     title:  string, fields: [ field ]  (form),
//     payload: string, image: QByteArray (empty=none), pollIntervalMs: number  (qr),
//     text:  string  (message) }
// where a `field` map is (the enriched wire v38 auth-param-field):
//   { key, label, required (bool),
//     kind: "Text"|"Password"|"Number"|"Choice"  (render/validation hint; "Text" default),
//     default: string (prefill; "" = none), placeholder: string ("" = none),
//     choices: [string] (allowed values when kind == "Choice"; else []) }
// — so a schema-driven form masks secrets (kind == "Password"), prefills, hints, and offers
// choice dropdowns without touching the codec. The same field shape backs providers()' params.
//
// Distinct from IAccountsService (account rows + API-key storage) and from the SASL connection
// handshake (node_api_auth): this seam drives a challenge/response login that ends in a node-stored
// credential; it never sees a secret itself.
class IAuthFlowService : public QObject {
    Q_OBJECT

public:
    using QObject::QObject;
    ~IAuthFlowService() override = default;

    // The families supporting interactive auth, each a row:
    //   { family, flowKind ("MatrixSso"|"OAuth2Pkce"|"BotToken"|"UserToken"|"PhoneOtp"|
    //     "QrPairing"|"UserPassword"), name, params: [ field ] }
    // (`field` is the enriched shape documented above.) Empty until refreshProviders() resolves
    // (or the mock seeds).
    [[nodiscard]] Q_INVOKABLE virtual QVariantList providers() const = 0;

    // Kick a provider-list fetch; providersChanged() fires when the list is ready.
    Q_INVOKABLE virtual void refreshProviders() = 0;

    // Begin a flow for `family` with the family-specific `params` (string map, e.g. Matrix SSO's
    // {homeserver}) against the CLIENT-owned `redirectUri`. A non-empty `bindProfile` asks the
    // node to bind the resulting account to that profile on success. Resolves asynchronously to
    // begun() (carrying the initial challenge) or flowFailed("begin", ...).
    virtual void begin(const QString& family, const QVariantMap& params, const QString& redirectUri,
                       const QString& bindProfile = QString()) = 0;

    // Advance `flowId` one step with the `input` collected for the current challenge. Resolves to
    // challenged() (the next challenge) or completed() (the flow finished), or flowFailed("step",
    // ...). The flow id stays valid across steps until it completes or is cancelled.
    virtual void step(const QString& flowId, const StepInput& input) = 0;

    // Drop a pending flow early (user cancelled / TTL expired client-side). Best-effort.
    virtual void cancel(const QString& flowId) = 0;

signals:
    void providersChanged();
    // A flow is parked node-side with its INITIAL `challenge` (see the shape doc above).
    // `expiresAt` is unix seconds (0 = no TTL advertised).
    void begun(const QString& flowId, const QVariantMap& challenge, quint64 expiresAt);
    // A step advanced the flow to a NEW `challenge`: re-render it and collect the next input.
    void challenged(const QVariantMap& challenge);
    // The flow finished: the credential blob landed node-side under `credentialRef` and the
    // account resolved to `accountLabel` / `transportInstance` (`boundProfile` non-empty when the
    // begin's bind was honored). No secret ever crosses this seam.
    void completed(const QString& credentialRef, const QString& accountLabel,
                   const QString& transportInstance, const QString& boundProfile);
    // `phase` is "providers" | "begin" | "step"; `message` is actionable.
    void flowFailed(const QString& phase, const QString& message);
};

} // namespace auth
