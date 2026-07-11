// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once
// A7T: the substrate-gated adapter behind ITranscriptMirrorSink. Maps a coalesced
// CachedTranscriptBlockRow onto the generated mirror::TranscriptBlock and feeds the ingestor
// (the single mirror writer, §5.1). The one lossy step is structural, not logical: the generated
// entity carries no argsSummary / detailKind / detailBody (see LEDGER-a7t "ENTITY-FIELD GAP").

#include "daemon/transcript_mirror_sink.h"

#include <QObject>

namespace mirror {
class Ingestor;
} // namespace mirror

namespace daemonapp::daemon {

// QObject so the app service graph can own it by parent; the engine holds only the
// ITranscriptMirrorSink base (no mirror/QObject coupling required at the callsite).
class MirrorTranscriptSink final : public QObject, public ITranscriptMirrorSink {
    Q_OBJECT

public:
    explicit MirrorTranscriptSink(mirror::Ingestor* ingestor, QObject* parent = nullptr)
        : QObject(parent), m_ingestor(ingestor) {}

    void deliverBlock(const CachedTranscriptBlockRow& row) override;
    void clear(const QString& sessionId) override;

private:
    mirror::Ingestor* m_ingestor = nullptr;
};

} // namespace daemonapp::daemon
