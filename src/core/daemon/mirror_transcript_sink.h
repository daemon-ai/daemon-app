// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once
// A7T/G2: the substrate-gated adapter behind ITranscriptMirrorSink. Maps a coalesced
// CachedTranscriptBlockRow onto the generated mirror::TranscriptBlock and feeds the ingestor
// (the single mirror writer, §5.1). Lossless since G2 closed the A7T "ENTITY-FIELD GAP": the
// entity carries args_summary + detail_kind/detail_body, so the mirror projection reproduces
// tool fences byte-for-byte (S1-S9 parity).

#include "daemon/transcript_mirror_sink.h"

#include <QObject>

namespace mirror {
class Ingestor;
class Store;
} // namespace mirror

namespace daemonapp::daemon {

// QObject so the app service graph can own it by parent; the engine holds only the
// ITranscriptMirrorSink base (no mirror/QObject coupling required at the callsite).
class MirrorTranscriptSink final : public QObject, public ITranscriptMirrorSink {
    Q_OBJECT

public:
    // `store` (optional) answers the roster enrichment READS (child title / end-reason) from the
    // mirror snapshot; null (bare test stacks that only exercise the write path) answers empty.
    explicit MirrorTranscriptSink(mirror::Ingestor* ingestor, mirror::Store* store = nullptr,
                                  QObject* parent = nullptr)
        : QObject(parent), m_ingestor(ingestor), m_store(store) {}

    void deliverBlock(const CachedTranscriptBlockRow& row) override;
    void clear(const QString& sessionId) override;
    [[nodiscard]] QString sessionTitle(const QString& sessionId) const override;
    [[nodiscard]] QString unitEndReason(const QString& sessionId) const override;

private:
    mirror::Ingestor* m_ingestor = nullptr;
    mirror::Store* m_store = nullptr;
};

} // namespace daemonapp::daemon
