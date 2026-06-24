#pragma once

#include "memory/memory_dtos.h"

#include <QList>
#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

namespace memory {

// Transport-agnostic memory-inspection seam (sibling of IFsService): the GUI
// and TUI inspect an agent's / profile's Mnemosyne memory through the connected
// node, never reading the SQLite bank directly. Every op is async + signal-based
// to suit a latency-bound remote transport. Reads are filtered by the current
// (profile, session, include-global) scope. A seeded MockMemoryService backs the
// UI now; a daemon NodeApi adapter replaces it later by decoding the wire once.
// The wire shape (memory-visualization-spec section 4) is free to change without
// touching this seam.
class IMemoryService : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString profile READ profile NOTIFY scopeChanged)
    Q_PROPERTY(QString session READ session NOTIFY scopeChanged)
    Q_PROPERTY(bool includeGlobal READ includeGlobal NOTIFY scopeChanged)

public:
    using QObject::QObject;
    ~IMemoryService() override = default;

    [[nodiscard]] QString profile() const { return m_profile; }
    [[nodiscard]] QString session() const { return m_session; }
    [[nodiscard]] bool includeGlobal() const { return m_includeGlobal; }

    // Set the (profile, session, include-global) scope all subsequent reads are
    // filtered by. Emits scopeChanged when anything actually changes.
    Q_INVOKABLE void setScope(const QString& profile, const QString& session, bool includeGlobal);

    // Aggregate counts + breakdowns. Result via statsReady.
    Q_INVOKABLE virtual void requestStats() = 0;
    // A page of memory rows. `query` keys: tier, source, veracity, status, text,
    // sort ("recent"|"importance"|"recall"), limit, after. Result via pageReady
    // with op == "list".
    Q_INVOKABLE virtual void requestList(const QVariantMap& query) = 0;
    // One memory by id. Result via entryReady.
    Q_INVOKABLE virtual void requestGet(const QString& id) = 0;
    // A graph snapshot. `kind` is association|knowledge|constellation; `seed`
    // (optional) + `depth` drive BFS expansion. Result via graphReady.
    Q_INVOKABLE virtual void requestGraph(const QString& kind, const QString& seed, int depth,
                                          int limit) = 0;
    // Full-text-ish search across memories. Result via pageReady with op == "search".
    Q_INVOKABLE virtual void requestSearch(const QString& text, int limit) = 0;
    // Grouped event timeline. `group` is "day" | "session". Result via timelineReady.
    Q_INVOKABLE virtual void requestTimeline(const QString& group, int limit) = 0;
    // The conversations (sessions) within `profile`'s bank, for the session-filter
    // facet (memory is owned by the agent; sessions are an optional lens). Result
    // via sessionsReady.
    Q_INVOKABLE virtual void requestSessions(const QString& profile) = 0;
    // Subscribe / unsubscribe to the live memory event stream (memoryEvent).
    Q_INVOKABLE virtual void startWatch() = 0;
    Q_INVOKABLE virtual void stopWatch() = 0;

signals:
    void scopeChanged();
    void statsReady(const memory::MemoryStats& stats);
    void pageReady(const QString& op, const QList<memory::MemoryEntry>& items,
                   const QString& nextCursor);
    void entryReady(const QString& id, const memory::MemoryEntry& entry, bool found);
    void graphReady(const memory::MemoryGraph& graph);
    void timelineReady(const QList<memory::MemoryTimelineGroup>& groups);
    void memoryEvent(const memory::MemoryEvent& event);
    // The agent's conversations: [{ id, label }] for the session-filter facet.
    void sessionsReady(const QString& profile, const QVariantList& sessions);
    void failed(const QString& op, const QString& message);

protected:
    QString m_profile;
    QString m_session;
    bool m_includeGlobal = true;
};

// Register the DTO metatypes used by the signals above (safe to call more than
// once). Lets queued connections and QML marshal the memory:: structs.
void registerMemoryMetatypes();

} // namespace memory
