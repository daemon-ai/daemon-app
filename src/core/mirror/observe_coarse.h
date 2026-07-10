#pragma once
// CoarseObserve — the handwritten coarse-signal implementation of the observe seam (ADR-008
// candidate (b)). A single per-commit Qt signal carries the touched kinds + rev range; scalar
// and derived-value consumers connect to it and re-read the store snapshot. Radically simple
// (01§7-ADAPT): its price is whole-kind notification granularity (equality gating happens per
// reader by value compare — cheap under immer) and hand-plumbed derived values. The simplicity
// fence (ADR-008 consequences): the moment a derived-node graph is wanted on top, reopen the
// ADR rather than hand-building the graph (01§6's bug class).

#include "mirror/observe.h"

#include <QObject>

namespace mirror {

class CoarseObserve : public QObject, public Observe {
    Q_OBJECT

public:
    explicit CoarseObserve(QObject* parent = nullptr) : QObject(parent) {}

    void publish(const MirrorModel& /*snapshot*/, const CommitInfo& info) override {
        ++commit_count_;
        last_ = info;
        if (!info.isEmpty()) {
            Q_EMIT mirrorCommitted(info.revFrom, info.revTo);
        }
    }

    [[nodiscard]] const char* seamName() const override { return "coarse-signals"; }

    [[nodiscard]] int commitCount() const noexcept { return commit_count_; }
    [[nodiscard]] const CommitInfo& lastCommit() const noexcept { return last_; }

Q_SIGNALS:
    // The one seam signal (spec §4.2 name). A consumer re-reads the store snapshot for its kind
    // and value-compares to decide whether to emit its own Q_PROPERTY NOTIFY.
    void mirrorCommitted(quint64 revFrom, quint64 revTo);

private:
    CommitInfo last_;
    int commit_count_ = 0;
};

} // namespace mirror
