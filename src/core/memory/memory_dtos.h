// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QList>
#include <QMetaType>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>

namespace memory {

// One memory record as served by IMemoryService. Fields mirror the Mnemosyne
// BEAM row (working/episodic) plus derived trust/lifecycle attributes the
// dashboard computes. All identity/enumerated fields are strings so the wire
// shape (daemon NodeApi, not yet finalized) can change without touching this
// seam; the future daemon adapter is the only place that decodes the codec.
struct MemoryEntry {
    QString id;
    QString content;
    QString source;
    QString timestamp; // RFC3339-ish; sortable lexically
    QString profile;   // the owning agent (ProfileRef == the Mnemosyne bank)
    QString sessionId;
    QString scope;     // "session" | "global"
    QString tier;      // "working" | "episodic" | "scratchpad"
    int tierLevel = 1; // episodic degradation tier 1/2/3; else 1
    double importance = 0.0;
    QString veracity; // stated|inferred|tool|imported|unknown
    QString trustTier;
    int recallCount = 0;
    QString lastRecalled;
    QString validUntil;
    QString supersededBy;
    QString status;           // active|expired|superseded
    QString degradationLabel; // hot|warm|cold (episodic)
    double trustWeight = 1.0;
    double degradationWeight = 1.0;
    double effectiveWeight = 1.0;
    bool contaminated = false;
    double score = -1.0; // >= 0 only for search/recall results
};

// One labelled count for a breakdown bar (by source/scope/veracity/...).
struct Bucket {
    QString key;
    int count = 0;
};

// Aggregate counts + breakdowns backing the Overview cards and gauges.
struct MemoryStats {
    int working = 0;
    int episodic = 0;
    int scratchpad = 0;
    int triples = 0;
    int facts = 0;
    int conflicts = 0;
    QList<Bucket> bySource;
    QList<Bucket> byScope;
    QList<Bucket> byVeracity;
    QList<Bucket> byDegradation;
    QList<Bucket> bySession;
};

// A node in a memory graph. `kind` is memory|entity|fact|gist|conflict; `meta`
// carries kind-specific display fields (e.g. veracity, predicate).
struct MemoryNode {
    QString id;
    QString kind;
    QString label;
    double weight = 1.0; // importance / confidence / mention_count
    QVariantMap meta;
};

// A typed, weighted edge. `edgeType` is references|mentions|<predicate>|
// contradicts|summarizes; `label` is the predicate text for SPO edges.
struct MemoryEdge {
    QString source;
    QString target;
    QString edgeType;
    double weight = 1.0;
    QString label;
};

// A category grouping of nodes (constellation clusters).
struct MemoryCluster {
    QString id;
    QString label;
    QStringList nodeIds;
};

// A graph snapshot for one MemoryGraphKind (association|knowledge|constellation).
struct MemoryGraph {
    QString kind;
    QList<MemoryNode> nodes;
    QList<MemoryEdge> edges;
    QList<MemoryCluster> clusters;
};

// One live/historical memory event (the dashboard's realtime stream taxonomy).
// A Q_GADGET so the live Overview feed can read its fields directly from the
// memoryEvent signal in QML; the other DTOs are consumed by C++ models.
struct MemoryEvent {
    Q_GADGET
    Q_PROPERTY(quint64 seq MEMBER seq)
    Q_PROPERTY(QString kind MEMBER kind)
    Q_PROPERTY(QString memoryId MEMBER memoryId)
    Q_PROPERTY(QString at MEMBER at)
    Q_PROPERTY(QString summary MEMBER summary)
public:
    quint64 seq = 0;
    QString kind; // added|updated|recalled|invalidated|consolidated|snapshot
    QString memoryId;
    QString at;
    QString summary;
};

// A timeline bucket (by day or by session) of events.
struct MemoryTimelineGroup {
    QString key;
    QList<MemoryEvent> events;
};

} // namespace memory

Q_DECLARE_METATYPE(memory::MemoryEntry)
Q_DECLARE_METATYPE(memory::Bucket)
Q_DECLARE_METATYPE(memory::MemoryStats)
Q_DECLARE_METATYPE(memory::MemoryNode)
Q_DECLARE_METATYPE(memory::MemoryEdge)
Q_DECLARE_METATYPE(memory::MemoryCluster)
Q_DECLARE_METATYPE(memory::MemoryGraph)
Q_DECLARE_METATYPE(memory::MemoryEvent)
Q_DECLARE_METATYPE(memory::MemoryTimelineGroup)
