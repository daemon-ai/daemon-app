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
      scheduler_(forwarding_), ingestor_(store_, scheduler_) {}

MirrorService::~MirrorService() = default;

bool MirrorService::open(const DbPathProvider& paths) {
    if (!persistence_.open(paths)) {
        qCWarning(lcMirror).noquote()
            << "mirror.db open failed; running in-memory:" << persistence_.lastError();
        openInMemory();
        return false;
    }
    // Boot load: rebuild the M tables + the chat windows' persisted contiguous tails via
    // transients, seed the rev counter from the persisted head, then adopt the snapshot as the
    // live root (§4.5 boot path; E1 offline render includes the last-known timeline).
    MirrorModel loaded;
    (void)persistence_.loadInto(loaded, store_.windowMaxItems(EntityKind::ChatMessage));
    store_.adoptLoaded(std::move(loaded), persistence_.persistedJournalHead());
    // Seed the ingestor's per-conversation ConvHistory cursors from the reloaded windows so the
    // reconnect top-up resumes AFTER the persisted tail — never re-appending (and thus never
    // duplicating/disordering) what the boot-load restored (§13 M1 cursor fix, boot edge).
    for (const auto& entry : store_.snapshot().chat) {
        ingestor_.syncState().setConvCursor(entry.first.serialize(),
                                            entry.second.meta.newest_cursor);
    }
    // The durable consumer resumes at its persisted watermark and flushes every commit thereafter
    // (write-behind, B7) so the disposable cache always reflects committed node truth.
    write_behind_.start();
    write_behind_.setFlushOnCommit(true);
    persistent_ = true;
    return true;
}

void MirrorService::openInMemory() {
    write_behind_.start();
    // No flush-on-commit: nothing to persist. The store is authoritative in-memory for this run.
    persistent_ = false;
}

void MirrorService::setFetchExecutor(FetchExecutor* executor) {
    forwarding_.setDelegate(executor);
}

void MirrorService::ingest(const NodeEvent& event) {
    ingestor_.ingest(event);
}

} // namespace mirror
