// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "mirror/seeder.h"

#include <algorithm>
#include <utility>

namespace mirror {

quint64 Seeder::seed(const SeedSet& set) {
    // One batch → one commit → one notification wave (§4.2), journal origin = Seeder throughout so
    // mock parity holds by construction (§9). Diff-before-write in the substrate suppresses no-ops.
    Batch b = store_.beginBatch();
    b.upsertPage(set.conversations, JournalOrigin::Seeder);
    b.upsertPage(set.contacts, JournalOrigin::Seeder);
    b.upsertPage(set.persons, JournalOrigin::Seeder);
    b.upsertPage(set.sessions, JournalOrigin::Seeder);
    b.upsertPage(set.fleetUnits, JournalOrigin::Seeder);
    b.upsertPage(set.routePins, JournalOrigin::Seeder);
    b.upsertPage(set.rooms, JournalOrigin::Seeder);
    b.upsertPage(set.transportAccounts, JournalOrigin::Seeder);
    for (auto it = set.chat.constBegin(); it != set.chat.constEnd(); ++it) {
        for (const ChatMessage& m : it.value()) {
            b.appendWindow(m, JournalOrigin::Seeder, m.origin_op);
        }
    }
    return b.commit();
}

quint64 Seeder::upsertConversation(const Conversation& c, const QString& originOp) {
    Batch b = store_.beginBatch();
    b.upsert(c, JournalOrigin::Seeder, originOp);
    return b.commit();
}

quint64 Seeder::appendMessage(const ChatMessage& m, const QString& originOp) {
    Batch b = store_.beginBatch();
    b.appendWindow(m, JournalOrigin::Seeder, originOp.isEmpty() ? m.origin_op : originOp);
    return b.commit();
}

quint64 Seeder::upsertContact(const Contact& c) {
    Batch b = store_.beginBatch();
    b.upsert(c, JournalOrigin::Seeder);
    return b.commit();
}

quint64 Seeder::upsertPerson(const Person& p) {
    Batch b = store_.beginBatch();
    b.upsert(p, JournalOrigin::Seeder);
    return b.commit();
}

quint64 Seeder::upsertRoom(const Room& r) {
    Batch b = store_.beginBatch();
    b.upsert(r, JournalOrigin::Seeder);
    return b.commit();
}

quint64 Seeder::upsertRoutePin(const RoutePin& p) {
    Batch b = store_.beginBatch();
    b.upsert(p, JournalOrigin::Seeder);
    return b.commit();
}

quint64 Seeder::upsertTransportAccount(const TransportAccount& a) {
    Batch b = store_.beginBatch();
    b.upsert(a, JournalOrigin::Seeder);
    return b.commit();
}

VerbOutcome VerbScript::outcomeFor(const QString& verb, const QJsonObject& args) const {
    for (const VerbRule& r : rules_) {
        if (r.verb != verb) {
            continue;
        }
        if (!r.matcher || r.matcher(args)) {
            return r.outcome;
        }
    }
    return VerbOutcome{}; // benign default: Ok, no echo
}

VerbRule VerbScript::ok(const QString& verb, int echoDelayMs) {
    VerbRule r;
    r.verb = verb;
    r.outcome.kind = VerbOutcomeKind::Ok;
    r.outcome.echoDelayMs = echoDelayMs;
    return r;
}

VerbRule VerbScript::reject(const QString& verb, const QString& errorKind, const QString& message) {
    VerbRule r;
    r.verb = verb;
    r.outcome.kind = VerbOutcomeKind::Reject;
    r.outcome.errorKind = errorKind;
    r.outcome.errorMessage = message;
    return r;
}

VerbRule VerbScript::timeout(const QString& verb, int ms) {
    VerbRule r;
    r.verb = verb;
    r.outcome.kind = VerbOutcomeKind::Timeout;
    r.outcome.delayMs = ms;
    return r;
}

ScenarioPlayer::ScenarioPlayer(Seeder& seeder, Scenario scenario)
    : seeder_(seeder), scenario_(std::move(scenario)) {
    // Steps fire in time order; sort defensively so an out-of-order fixture still plays correctly.
    std::ranges::stable_sort(scenario_.timeline, [](const ScenarioStep& a, const ScenarioStep& b) {
        return a.atMs < b.atMs;
    });
}

quint64 ScenarioPlayer::playSeed() {
    return seeder_.seed(scenario_.seed);
}

int ScenarioPlayer::advanceTo(qint64 nowMs) {
    int ran = 0;
    while (next_ < scenario_.timeline.size() && scenario_.timeline[next_].atMs <= nowMs) {
        const ScenarioStep& step = scenario_.timeline[next_];
        if (step.action) {
            step.action(seeder_);
        }
        ++next_;
        ++ran;
    }
    return ran;
}

} // namespace mirror
