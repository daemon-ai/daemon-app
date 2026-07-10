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
    // Boot load: rebuild the M tables via transients, seed the rev counter from the persisted
    // head, then adopt the snapshot as the live root (§4.5 boot path).
    MirrorModel loaded;
    persistence_.loadInto(loaded);
    store_.adoptLoaded(std::move(loaded), persistence_.persistedJournalHead());
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
