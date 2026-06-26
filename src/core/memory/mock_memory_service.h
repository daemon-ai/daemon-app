#pragma once

#include "memory/imemory_service.h"

#include <QList>
#include <QPair>
#include <QString>
#include <QTimer>

namespace memory {

// Seeded stand-in for the daemon memory-inspection surface. The ctor builds a
// believable Mnemosyne bank (working + episodic memories across sessions,
// veracities and sources; entity mentions; memory-memory references; SPO facts
// with a contradiction; a grouped event timeline) and answers every request
// asynchronously (queued emit) so the UI behaves as it will against a real
// latency-bound transport. A QTimer drives the live memory event stream. A real
// NodeApi adapter later replaces this class by emitting the same signals.
class MockMemoryService : public IMemoryService {
    Q_OBJECT

public:
    explicit MockMemoryService(QObject* parent = nullptr);

    void requestStats() override;
    void requestList(const QVariantMap& query) override;
    void requestGet(const QString& id) override;
    void requestGraph(const QString& kind, const QString& seed, int depth, int limit) override;
    void requestSearch(const QString& text, int limit) override;
    void requestTimeline(const QString& group, int limit) override;
    void requestSessions(const QString& profile) override;
    void startWatch() override;
    void stopWatch() override;

private:
    struct Mention {
        QString memId;
        QString entity;
    };
    struct Fact {
        QString profile; // the owning agent (bank) this fact belongs to
        QString subject;
        QString predicate;
        QString object;
        double confidence = 1.0;
        bool contradicts = false; // contradicts the prior fact with same subject
    };

    void seed();
    [[nodiscard]] QList<MemoryEntry> scoped() const;
    [[nodiscard]] bool inScope(const MemoryEntry& e) const;
    [[nodiscard]] MemoryStats computeStats() const;
    [[nodiscard]] MemoryGraph buildAssociation(const QString& seed, int depth, int limit) const;
    [[nodiscard]] MemoryGraph buildKnowledge(int limit) const;
    [[nodiscard]] MemoryGraph buildConstellation(int limit) const;
    [[nodiscard]] QList<MemoryTimelineGroup> buildTimeline(const QString& group, int limit) const;
    void emitTick();

    QList<MemoryEntry> m_memories;
    QList<Mention> m_mentions;             // memory -> entity
    QList<QPair<QString, QString>> m_refs; // memory -> memory ("references")
    QList<Fact> m_facts;                   // SPO knowledge
    QStringList m_sessions;
    QTimer m_watch;
    quint64 m_seq = 0;
};

} // namespace memory
