// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "mirror/mirror_service.h"

#include <QLoggingCategory>

namespace mirror {

namespace {
Q_LOGGING_CATEGORY(lcMirror, "daemon.mirror.service")
} // namespace

MirrorService::MirrorService(QObject* parent)
    : QObject(parent), store_(observe_), write_behind_(store_, persistence_),
      scheduler_(forwarding_), ingestor_(store_, scheduler_) {
    // §5.1/§6.6 provenance-landing seam: the single-writer commit path doubles as the outbox's
    // confirmation feed. On every commit, re-emit each newly-stamped journal record's non-empty
    // origin_op so the graph can land the matching outbox op (Outbox::onProvenanceStamped). This
    // is a read-side coupling — the outbox never writes mirrored state.
    QObject::connect(&store_, &Store::committed, this, &MirrorService::relayProvenance);
}

void MirrorService::relayProvenance(quint64 revFrom, quint64 /*revTo*/) {
    for (const JournalRecord& rec : store_.journal().since(revFrom)) {
        if (!rec.origin_op.isEmpty()) {
            Q_EMIT provenanceStamped(rec.origin_op);
        }
    }
}

MirrorService::~MirrorService() = default;

bool MirrorService::open(const DbPathProvider& paths) {
    persistence_.close(); // reopen-safe: a per-identity switch closes the prior file first
    const bool ok = persistence_.open(paths);
    if (!ok) {
        qCWarning(lcMirror).noquote()
            << "mirror.db open failed; running in-memory:" << persistence_.lastError();
    }
    // Boot/reopen load: rebuild the M tables + the chat windows' persisted contiguous tails via
    // transients, then adopt the snapshot as the live root (§4.5; E1 offline render includes the
    // last-known timeline). adoptLoaded REBASES the journal: the in-memory tail is dropped and
    // every consumer watermark jumps to the new persisted head, so a prior identity's deltas
    // never leak into this file or its consumers.
    MirrorModel loaded;
    quint64 head = 0;
    if (ok) {
        (void)persistence_.loadInto(loaded, store_.windowMaxItems(EntityKind::ChatMessage));
        head = persistence_.persistedJournalHead();
    }
    store_.adoptLoaded(std::move(loaded), head);
    // Fresh per-identity sync bookkeeping, then seed the per-conversation ConvHistory cursors
    // from the reloaded windows so the reconnect top-up resumes AFTER the persisted tail — never
    // re-appending (and thus never duplicating/disordering) what the boot-load restored (§13 M1
    // cursor fix, boot edge).
    ingestor_.syncState() = SyncState{};
    for (const auto& entry : store_.snapshot().chat) {
        ingestor_.syncState().setConvCursor(entry.first.serialize(),
                                            entry.second.meta.newest_cursor);
    }
    // The durable consumer resumes at the rebased watermark and flushes every commit thereafter
    // (write-behind, B7) so the disposable cache always reflects committed node truth. Started
    // once; a reopen only rebased its watermark above.
    if (!wb_started_) {
        write_behind_.start();
        wb_started_ = true;
    }
    write_behind_.setFlushOnCommit(ok);
    persistent_ = ok;
    return ok;
}

void MirrorService::openInMemory() {
    if (!wb_started_) {
        write_behind_.start();
        wb_started_ = true;
    }
    // No flush-on-commit: nothing to persist. The store is authoritative in-memory for this run.
    write_behind_.setFlushOnCommit(false);
    persistent_ = false;
}

void MirrorService::setFetchExecutor(FetchExecutor* executor) {
    forwarding_.setDelegate(executor);
}

void MirrorService::ingest(const NodeEvent& event) {
    ingestor_.ingest(event);
}

} // namespace mirror
