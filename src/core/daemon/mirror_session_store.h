// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once
// mirror A7 (spec 09 §13 wave M4) — the mirror-backed ISessionStore: the ONE projection the
// session/fleet consumers (roster list, sidebar, pickers, detail pane, transcript chrome) rebind
// onto, consumer-by-consumer, replacing the six divergent session row shapes with reads of the
// ONE generated mirror::Session entity (6→1).
//
// Read posture (dual-write window, §13 M4):
//  - Session ROWS, counts, title/pinned lookups: PURE mirror projections of Store::snapshot()
//    .sessions — never a hybrid row (rule zero / §14.1). Rows re-derive on Store::committed
//    (journal-filtered to Session/FleetUnit deltas above a registered consumer watermark, §4.4).
//  - content()/setContent(): DELEGATED to the legacy store. Transcript blocks join the mirror
//    LAST (sub-gate 6, single-writer re-homing of the engine stream); until then the legacy
//    cache projection remains the transcript source — never a blank pane.
//  - Mutations (pin/archive/rename/delete/create) + scoped refreshes: DELEGATED to the legacy
//    store (the node-authoritative SessionRepository wire ops — one mutation path), while the
//    scoped refreshes ALSO trigger the ingestor's mirror-side scoped fetches so the mirror rows
//    land too (dual-write, both live).
//  - Documented degradations (07§9.3 / LEDGER-a7): row `content` snippets and `tagIds` are empty
//    (transcript re-homing / client-local tag sidecar are later waves); `unitId` stays empty
//    (parity with the deleted legacy daemon store, which never set it either).
//
// Signals: changed() = mirror session/fleet/transcript journal deltas; sessionCreated is relayed
// from the direct create seam (daemon: SessionRepository::createSession; mock: the scenario
// host's scripted create via onNodeSessionCreated); metaUpdateFailed is the session-meta lane's
// §6.5 rejection relay.
//
// AD: the legacy ISessionStore delegate is GONE — every mutation rides its §7-declared path
// (session-meta lane / direct SessionCreate), every read is a pure mirror projection, and the
// node-owned no-verb operations (delete/move/setContent) are no-ops on BOTH modes (the mock
// aligns to daemon; the fork was the deletion — A8 precedent).

#include "persistence/isession_store.h"

#include <QHash>
#include <QJsonObject>
#include <QList>
#include <QSet>
#include <QString>

namespace mirror {
class Ingestor;
class Outbox;
class Store;
} // namespace mirror

namespace daemonapp::daemon {

class SessionRepository;

class MirrorSessionStore : public persistence::ISessionStore {
    Q_OBJECT

public:
    // `store`/`ingestor` are the mirror substrate (required). `sessions` is the daemon-mode
    // DIRECT create seam (§7: SessionCreate is a direct verb — the node mints the id and the
    // repo's sessionCreated relays it); null in mock mode, where requestNewSession emits
    // createRequested and the scenario host answers (scripted outcome + seeded row +
    // onNodeSessionCreated). Ownership stays with the caller (the app service graph).
    MirrorSessionStore(mirror::Store* store, mirror::Ingestor* ingestor,
                       SessionRepository* sessions = nullptr, QObject* parent = nullptr);

    // AD (§6.4/§7): bind the durable outbox — the session-meta mutation lane. Once bound,
    // pin/archive/rename enqueue `SessionUpdateMeta` ops (op_id minted per §6.2, offline-durable,
    // dedup-safe replay per §10.3); a lane rejection is relayed as metaUpdateFailed (§6.5 — the
    // verb seam's failure signal). Bound in BOTH modes by the composition (the mock host answers
    // the lane from the scenario script). Without an outbox (bare test stacks) the meta mutations
    // are no-ops — there is no legacy fallback path anymore.
    void setOutbox(mirror::Outbox* outbox);

    // The composition callback for a created session (mock: the scenario host's scripted
    // SessionCreate answer; the daemon repo relays through its own signal instead): re-emits
    // ISessionStore::sessionCreated so the initiating surface adopts the id (open + auto-select).
    void onNodeSessionCreated(const QString& sessionId, const QString& profileId);

    // --- pure mirror reads -------------------------------------------------------------------
    [[nodiscard]] QList<domain::Session> sessions(const domain::ListScope& scope) const override;
    [[nodiscard]] int sessionCount(const domain::ListScope& scope) const override;
    [[nodiscard]] QString title(const domain::SessionId& id) const override;
    [[nodiscard]] bool isPinned(const domain::SessionId& id) const override;

    // The fleet TREE renders through MirrorFleetTree (G2: the FleetUnit entity carries the
    // child_ids edge), never through these ISessionStore unit reads — daemon-mode consumers never
    // hit them (sessions are flat: unitId stays empty; the sidebar's fleet membership view reads
    // profiles + Agent-scoped sessions, not unitChildren). Empty, like the rest of the documented
    // degradations.
    [[nodiscard]] QList<domain::UnitNode>
    unitChildren(const domain::UnitId& parentId) const override;
    [[nodiscard]] domain::UnitNode unit(const domain::UnitId& id) const override;
    // Parity with the legacy daemon store: no tags / participants in daemon mode.
    [[nodiscard]] QList<domain::Tag> tags() const override;
    [[nodiscard]] QList<domain::Participant> participants() const override;

    // --- transcript reads: mirror-served since the G2 flip ------------------------------------
    // content() projects the mirror `w_transcript_blocks` window (mirrorContent). setContent
    // still delegates (a no-op on the legacy daemon store — the engine owns transcript writes).
    [[nodiscard]] QString content(const domain::SessionId& id) const override;
    void setContent(const domain::SessionId& id, const QString& markdown) override;

    // A7T/G2: the msg-fence transcript projection served FROM THE MIRROR `w_transcript_blocks`
    // window — the byte-stable twin of the legacy cache projection, ported grammar-for-grammar
    // and byte-identical across the full parity matrix (S1-S9, args + structured detail included)
    // since G2 enriched the entity (args_summary, detail_kind, detail_body). `content()` now
    // serves this; kept as a named method for the parity harness.
    [[nodiscard]] QString mirrorContent(const domain::SessionId& id) const;

    // --- node-authoritative mutations: §7-declared paths --------------------------------------
    // Pin/archive/rename → the session-meta outbox lane (§6.4). Create → the direct SessionCreate
    // seam (§7). newSession/delete/move/setContent are node-owned with NO v39 wire verb: no-ops
    // on both modes (creation is async via requestNewSession + sessionCreated).
    domain::SessionId newSession(const domain::UnitId& unitId) override;
    void setArchived(const domain::SessionId& id, bool archived) override;
    void renameSession(const domain::SessionId& id, const QString& title) override;
    void deleteSession(const domain::SessionId& id) override;
    void setPinned(const domain::SessionId& id, bool pinned) override;
    void moveSession(const domain::SessionId& id, int delta) override;
    void requestNewSession(const QString& profileId) override;
    domain::UnitId createUnit(const domain::UnitId& parentId, domain::UnitKind kind) override;
    int createTag(const QString& name, const QString& color) override;

    // --- scoped refreshes: the ingestor's mirror scoped fetches -------------------------------
    void refreshSessionsForProfile(const QString& profileId) override;
    void refreshArchivedSessions() override;
    void refreshSessionsForTransport(const QString& transportId) override;

signals:
    // Mock-mode direct-create seam (§7/§9): no SessionRepository is bound, so the store asks the
    // composition to answer the create — the scenario host scripts the outcome, seeds the row
    // (origin = seeder), and calls onNodeSessionCreated. Never emitted in daemon mode (the repo
    // seam answers directly).
    void createRequested(const QString& profileId);

private:
    void onCommitted();
    void rebuildFromSnapshot();
    [[nodiscard]] bool matchesScope(const domain::Session& s, const domain::ListScope& scope) const;
    // Enqueue one SessionUpdateMeta patch to the session-meta lane (§6.4) and nudge a drain (the
    // user's own action is the manual tap; gates hold it offline). `patch` carries only the
    // touched key(s) — the tri-state wire SessionMetaPatch shape as JSON.
    void enqueueSessionMeta(const domain::SessionId& id, const QJsonObject& patch,
                            const QString& display);

    mirror::Store* m_mirror = nullptr;
    mirror::Ingestor* m_ingestor = nullptr;
    mirror::Outbox* m_outbox = nullptr;
    SessionRepository* m_sessions = nullptr; // daemon direct-create seam (null in mock)

    // The sorted mirror projection (pinned first, then last-activity desc, then id — a
    // deterministic order where the legacy cache's insertion order was arbitrary). The list index
    // is the demoted local int handle, like the legacy stores.
    QList<domain::Session> m_rows;
    // Node-resolved ByTransport membership (B4): transport id -> session id set, recorded from
    // the ingestor's transportSessionsResolved. A projection input, never a mirror write.
    QHash<QString, QSet<QString>> m_transportSessions;
    // Journal watermark (§4.4): committed() re-derives only when Session/FleetUnit deltas landed
    // above it, so chat/routing traffic does not thrash the roster consumers.
    quint64 m_watermark = 0;
};

} // namespace daemonapp::daemon
