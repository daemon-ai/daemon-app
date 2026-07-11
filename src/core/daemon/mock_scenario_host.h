// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once
// mirror A8 (spec 09 §9, wave M5) — the mock-mode scenario driver. Owns the mock single Writer
// (mirror::Seeder, §5.1 "Ingestor xor Seeder") and plays one MockScenario against the live mock
// mirror:
//
//  - seeds the mirror at construction (one batch through the SAME apply pipeline, origin=seeder);
//  - advances the scripted timeline on a QTimer (deterministic steps; message arrivals, presence
//    flips — each one Seeder batch);
//  - answers `Outbox::sendRequested` from the scenario's VerbScript (§9 scripted outcomes): ok →
//    delayed ack + a provenance ECHO through the seeder (`origin_op = op_id`, §6.6 — the mock
//    twin of the node's read-path echo); reject → typed api-error → lane pause (§6.5); timeout →
//    delayed transient failure → backoff (§6.3). Dispatch-lane ops (turn prompts) are NOT
//    answered — their own consumer drains them.
//  - installs a scripted-pager FetchExecutor that records and async-completes every fetch job
//    (the mirror is seeder-authoritative in mock; the recording is the app-level §5.6 evidence);
//  - `onConnectionReady()` is the mock Hello: it drives the ingestor's per-connection mode select
//    from the scenario's api/<N> (38 degraded / 39 full+Bootstrap), flips the outbox's provenance
//    capability, and auto-replays iff §6.8 allows — the same wiring the daemon graph runs on
//    AuthOk.
//
// Nothing mock-only exists below the verb boundary (§14 rule 5): this class writes ONLY through
// the seeder and speaks ONLY the outbox/ingestor seams.

#include "daemon/mock_scenario.h"
#include "mirror/fetch_scheduler.h"
#include "mirror/seeder.h"
#include "outbox_types.h"

#include <QElapsedTimer>
#include <QList>
#include <QObject>
#include <QTimer>

namespace mirror {
class MirrorService;
class Outbox;
} // namespace mirror

namespace daemonapp::daemon {

class MockScenarioHost : public QObject {
    Q_OBJECT

public:
    MockScenarioHost(mirror::MirrorService* svc, mirror::Outbox* outbox, MockScenario scenario,
                     QObject* parent = nullptr);

    [[nodiscard]] int apiVersion() const noexcept { return m_player.apiVersion(); }
    [[nodiscard]] const MockScenario& scenario() const noexcept { return m_scenario; }

    // The scripted pager to install as the mock FetchExecutor (MirrorService::setFetchExecutor).
    [[nodiscard]] mirror::FetchExecutor* fetchExecutor() noexcept { return &m_pager; }
    // Every job the reconnect/staleness machinery issued (the §5.6 mode evidence).
    [[nodiscard]] QList<mirror::FetchJob> executedJobs() const { return m_pager.executed; }

    // The mock Hello (§5.6/§6.8): mode select + provenance capability + auto-replay.
    void onConnectionReady();
    void onConnectionLost();

    // Deterministic manual timeline advance (tests); the live QTimer drives the same path.
    void advanceTimelineTo(qint64 elapsedMs);

    // AD (§7 direct create, mock side): answer MirrorSessionStore::createRequested from the
    // scenario's VerbScript — on ok, mint a deterministic id, seed the blank Session row through
    // the seeder (the node-parity shape: untitled, Ready, bound to `profileId`), and emit
    // sessionCreated so the composition relays it into the store's adoption path.
    void onCreateSessionRequested(const QString& profileId);

signals:
    // A scripted SessionCreate resolved: `sessionId` is the host-minted id (the mock twin of the
    // node's SessionCreated reply), `profileId` echoes the request.
    void sessionCreated(const QString& sessionId, const QString& profileId);

private:
    // Records + async-completes every job: the mirror is seeder-authoritative, so a fetch has
    // nothing to fetch — but the scheduler's slots must be freed (async to avoid re-entrant
    // pump() during enqueue).
    class ScriptedPager final : public mirror::FetchExecutor {
    public:
        explicit ScriptedPager(MockScenarioHost& host) : host(host) {}
        void execute(const mirror::FetchJob& job) override;
        MockScenarioHost& host; // NOLINT(misc-non-private-member-variables-in-classes)
        QList<mirror::FetchJob> executed;
    };

    void onSendRequested(const mirror::PendingOp& op);
    void applyConvSendEcho(const mirror::PendingOp& op);
    // AD (§6.4 session-meta lane): the mock twin of the node's SessionMetaChanged → SessionGet
    // read-path echo — apply the acked patch to the mirror Session row through the seeder,
    // provenance-stamped (origin_op = op_id) so the outbox op lands (§6.6).
    void applySessionMetaEcho(const mirror::PendingOp& op);
    [[nodiscard]] quint64 nextChatCursor(const QString& transport, const QString& conv) const;

    mirror::MirrorService* m_svc = nullptr;
    mirror::Outbox* m_outbox = nullptr;
    MockScenario m_scenario;
    mirror::Seeder m_seeder;
    mirror::ScenarioPlayer m_player;
    ScriptedPager m_pager;
    QTimer m_timelineTimer;
    QTimer m_expirySweep;
    QElapsedTimer m_elapsed;
    int m_nextSessionSeq = 1; // deterministic ids for scripted SessionCreate answers
};

} // namespace daemonapp::daemon
