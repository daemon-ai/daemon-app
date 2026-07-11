// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once
// mirror::Seeder — the mock-mode single Writer (spec 09 §9; §5.1 "Ingestor xor Seeder"). It builds
// canonical entities and writes them through the SAME mirror::Store apply/commit pipeline the
// ingestor uses, journal origin = Seeder, so mock/daemon parity holds BY CONSTRUCTION — "same
// store, same journal, same lenses, different feeder" (§9). There is no bespoke mock apply and no
// mock-only behaviour below the verb boundary (§14 rule 5); scripted outcomes (VerbScript) live
// only at the verb/wire boundary.
//
// A Scenario is a declarative seed set + a scripted timeline (events over time) + the mock Hello's
// api/<N> + a per-verb outcome script. ScenarioPlayer plays it deterministically against an
// injected clock (the caller/test advances it; the mock composition drives it from a QTimer).

#include "mirror/generated/entities_gen.h"
#include "mirror/store.h"

#include <cstddef>
#include <functional>
#include <QHash>
#include <QJsonObject>
#include <QString>
#include <vector>

namespace mirror {

// Declarative initial data (§9): one vector per class-M/L kind the mock mirror serves, plus the
// class-W chat windows keyed by "transport␟conv" (oldest-first). Applied in ONE batch per seed()
// (§4.2 one commit per applied batch). Extend with kinds as their mock verticals land.
struct SeedSet {
    std::vector<Conversation> conversations;
    std::vector<Contact> contacts;
    std::vector<Person> persons;
    std::vector<Session> sessions;
    std::vector<FleetUnit> fleetUnits;
    std::vector<RoutePin> routePins;
    std::vector<Room> rooms;
    std::vector<TransportAccount> transportAccounts;
    QHash<QString, std::vector<ChatMessage>> chat;
    // Class-W transcript blocks (scope = session, window seq): the canonical transcript CONTENT
    // form. Seeded through the same cursor-ordered upsert the engine/ingestor use so the mirror's
    // MirrorSessionStore::content() projection is fed identically in mock and daemon (§4.6/§9).
    std::vector<TranscriptBlock> transcriptBlocks;
};

class Seeder {
public:
    explicit Seeder(Store& store) : store_(store) {}

    // Apply the whole seed set in one batch, journal origin = Seeder. Returns the new head rev.
    quint64 seed(const SeedSet& set);

    // Convenience timeline steps (deterministic; no hidden clock). Each is ONE batch stamped
    // origin = Seeder. `originOp` carries a scripted provenance echo (§9 / §6.6): a mutation
    // outcome's op-id, so the mock exercises the outbox provenance-landing path identically to the
    // daemon read path.
    quint64 upsertConversation(const Conversation& c, const QString& originOp = QString());
    quint64 appendMessage(const ChatMessage& m, const QString& originOp = QString());
    quint64 upsertContact(const Contact& c);
    quint64 upsertSession(const Session& s, const QString& originOp = QString());
    quint64 upsertPerson(const Person& p);
    quint64 upsertRoom(const Room& r);
    quint64 upsertRoutePin(const RoutePin& p);
    quint64 upsertTransportAccount(const TransportAccount& a);

    [[nodiscard]] Store& store() noexcept { return store_; }

private:
    Store& store_;
};

// ---- scripted verb outcomes (§9; MANDATORY rejection fixtures) --------------------------------

// The outcome the mock verb/wire boundary returns instead of ad-hoc per-mock logic.
enum class VerbOutcomeKind {
    Ok,      // acks Ok; a MutationEffect may schedule a provenance echo after echoDelayMs
    Reject,  // a typed api-error (closed v38 set) — drives the outbox lane pause (§6.5)
    Timeout, // no reply within the drain timeout (transient; backoff)
    Delay,   // ack after delayMs (latency simulation)
};

struct VerbOutcome {
    VerbOutcomeKind kind = VerbOutcomeKind::Ok;
    QString errorKind;    // Reject: the api-error kind (e.g. "Forbidden", "Conflict")
    QString errorMessage; // Reject: the human-readable message
    int delayMs = 0;      // Delay / Timeout window (ms)
    int echoDelayMs = 0;  // Ok: provenance-echo delay before the seeder applies the change (ms)
};

// One per-(verb, matcher) rule. `matcher` is a predicate over the verb's JSON args (the outbox
// payload shape); a null matcher matches every call of the verb.
struct VerbRule {
    QString verb;
    std::function<bool(const QJsonObject& args)> matcher;
    VerbOutcome outcome;
};

// The scenario's verb script. First matching rule wins; an unmatched verb defaults to Ok (no
// provenance echo) — the benign default keeps unspecified verbs working (§9 "mock keeps working").
class VerbScript {
public:
    void add(VerbRule rule) { rules_.push_back(std::move(rule)); }
    [[nodiscard]] VerbOutcome outcomeFor(const QString& verb, const QJsonObject& args) const;
    [[nodiscard]] bool empty() const noexcept { return rules_.empty(); }

    // Fixture shorthands.
    static VerbRule ok(const QString& verb, int echoDelayMs = 0);
    static VerbRule reject(const QString& verb, const QString& errorKind, const QString& message);
    static VerbRule timeout(const QString& verb, int ms);

private:
    std::vector<VerbRule> rules_;
};

// One scripted timeline event: run `action` once the player passes `atMs`.
struct ScenarioStep {
    qint64 atMs = 0;
    std::function<void(Seeder&)> action;
};

// A whole mock scenario: initial data + the api/<N> the mock Hello advertises (§5.6/§6.8) + the
// verb script + the scripted timeline (ordered by atMs).
struct Scenario {
    SeedSet seed;
    int apiVersion = 39;
    VerbScript verbScript;
    std::vector<ScenarioStep> timeline;
};

// Plays a Scenario deterministically: seed once, then fire timeline steps as the clock advances.
// Non-QObject and clock-free — the caller (a test or the mock composition's QTimer) advances it, so
// there is no hidden timer in the data layer (§11 single-threaded, deterministic replay §12).
class ScenarioPlayer {
public:
    ScenarioPlayer(Seeder& seeder, Scenario scenario);

    quint64 playSeed();          // apply the initial seed set (one batch); returns the head rev
    int advanceTo(qint64 nowMs); // run every not-yet-run step with atMs <= nowMs, in order

    [[nodiscard]] const Scenario& scenario() const noexcept { return scenario_; }
    [[nodiscard]] int apiVersion() const noexcept { return scenario_.apiVersion; }
    [[nodiscard]] bool timelineExhausted() const noexcept {
        return next_ >= scenario_.timeline.size();
    }

private:
    Seeder& seeder_;
    Scenario scenario_;
    std::size_t next_ = 0;
};

} // namespace mirror
