#include "memory/mock_memory_service.h"

#include <algorithm>
#include <QHash>
#include <QSet>

namespace memory {
namespace {

double trustWeightFor(const QString& veracity) {
    if (veracity == QStringLiteral("stated"))
        return 1.0;
    if (veracity == QStringLiteral("inferred"))
        return 0.7;
    if (veracity == QStringLiteral("tool"))
        return 0.5;
    if (veracity == QStringLiteral("imported"))
        return 0.6;
    return 0.8; // unknown
}

double degradationWeightFor(const QString& tier, int level) {
    if (tier != QStringLiteral("episodic"))
        return 1.0;
    switch (level) {
    case 1:
        return 1.0;
    case 2:
        return 0.5;
    default:
        return 0.25;
    }
}

QString degradationLabelFor(const QString& tier, int level) {
    if (tier != QStringLiteral("episodic"))
        return {};
    switch (level) {
    case 1:
        return QStringLiteral("hot");
    case 2:
        return QStringLiteral("warm");
    default:
        return QStringLiteral("cold");
    }
}

// Increment a labelled count bucket list in place.
void bump(QList<Bucket>& buckets, const QString& key) {
    if (key.isEmpty())
        return;
    for (Bucket& b : buckets) {
        if (b.key == key) {
            b.count += 1;
            return;
        }
    }
    buckets.append(Bucket{key, 1});
}

} // namespace

MockMemoryService::MockMemoryService(QObject* parent) : IMemoryService(parent) {
    registerMemoryMetatypes();
    seed();
    m_watch.setInterval(4000);
    connect(&m_watch, &QTimer::timeout, this, &MockMemoryService::emitTick);
}

void MockMemoryService::seed() {
    m_sessions = {QStringLiteral("sess-alpha"), QStringLiteral("sess-beta")};

    struct Spec {
        const char* id;
        const char* content;
        const char* source;
        const char* session;
        const char* scope;
        const char* tier;
        int level;
        double importance;
        const char* veracity;
        int recall;
        const char* timestamp;
        const char* supersededBy;
        const char* profile; // the owning agent (bank)
    };

    // Memory is owned by the AGENT (one bank per profile). Each profile below is a
    // distinct bank holding several sessions (sessions), global + session
    // scope, working + episodic tiers (1/2/3 degradation), varied veracity/sources.
    static const Spec specs[] = {
        // prof-1 / General Assistant: planning + app notes.
        {"m01", "User prefers Rust for systems work and dislikes heavy frameworks.", "session",
         "ga-plan", "global", "working", 1, 0.9, "stated", 5, "2026-06-22T09:10:00Z", "", "prof-1"},
        {"m02", "Ada is the lead engineer on the daemon orchestrator.", "session", "ga-plan",
         "global", "working", 1, 0.85, "stated", 3, "2026-06-22T09:14:00Z", "", "prof-1"},
        {"m03", "The daemon uses Rust for the core engine.", "session", "ga-plan", "session",
         "working", 1, 0.7, "stated", 2, "2026-06-22T09:20:00Z", "", "prof-1"},
        {"m11", "The graph view should be GUI-only with a TUI fallback.", "session", "ga-plan",
         "session", "working", 1, 0.7, "stated", 1, "2026-06-24T08:40:00Z", "", "prof-1"},
        {"m15", "Daemon-app is a thin client with GUI and TUI front ends.", "session", "ga-notes",
         "global", "working", 1, 0.8, "stated", 2, "2026-06-22T09:30:00Z", "", "prof-1"},
        {"m16", "Consolidated summary of the June planning session.", "consolidation", "ga-plan",
         "session", "episodic", 1, 0.9, "stated", 3, "2026-06-22T20:00:00Z", "", "prof-1"},
        {"m12", "Scratch: try force-directed layout for the memory graph.", "scratchpad",
         "ga-notes", "session", "scratchpad", 1, 0.2, "unknown", 0, "2026-06-24T08:42:00Z", "",
         "prof-1"},
        // prof-2 / Coder: API work + review.
        {"m05", "Qt Quick renders the GUI; the TUI uses Tui Widgets.", "session", "cd-api",
         "session", "episodic", 1, 0.6, "stated", 1, "2026-06-21T16:30:00Z", "", "prof-2"},
        {"m06", "SQLite backs the Mnemosyne bank with WAL mode.", "tool", "cd-api", "global",
         "episodic", 2, 0.55, "inferred", 0, "2026-05-30T11:00:00Z", "", "prof-2"},
        {"m13", "Mnemosyne supports hybrid recall: FTS5 + vector + importance.", "tool",
         "cd-review", "global", "episodic", 1, 0.6, "tool", 1, "2026-06-19T12:00:00Z", "",
         "prof-2"},
        {"m10", "Old preference: user liked TypeScript (superseded).", "session", "cd-review",
         "global", "episodic", 3, 0.3, "stated", 0, "2026-02-01T09:00:00Z", "m05", "prof-2"},
        // prof-3 / Researcher: a survey session.
        {"m04", "Mnemosyne is the agent memory system being ported to Rust.", "tool", "rs-survey",
         "global", "episodic", 1, 0.8, "tool", 4, "2026-06-21T15:00:00Z", "", "prof-3"},
        {"m07", "The user mentioned a deadline near the end of June.", "session", "rs-survey",
         "session", "working", 1, 0.65, "inferred", 1, "2026-06-23T08:05:00Z", "", "prof-3"},
        {"m08", "Imported note: prior project used Python for the daemon.", "import", "rs-survey",
         "session", "episodic", 2, 0.4, "imported", 0, "2026-04-12T10:00:00Z", "", "prof-3"},
        {"m09", "Ada works at the platform team.", "session", "rs-survey", "global", "episodic", 1,
         0.75, "stated", 2, "2026-06-20T13:45:00Z", "", "prof-3"},
        {"m14", "User dislikes notifications during focus time.", "session", "rs-survey", "global",
         "episodic", 2, 0.5, "inferred", 0, "2026-05-15T18:20:00Z", "", "prof-3"},
    };

    for (const Spec& s : specs) {
        MemoryEntry e;
        e.id = QString::fromLatin1(s.id);
        e.content = QString::fromUtf8(s.content);
        e.source = QString::fromLatin1(s.source);
        e.profile = QString::fromLatin1(s.profile);
        e.sessionId = QString::fromLatin1(s.session);
        e.scope = QString::fromLatin1(s.scope);
        e.tier = QString::fromLatin1(s.tier);
        e.tierLevel = s.level;
        e.importance = s.importance;
        e.veracity = QString::fromLatin1(s.veracity);
        e.trustTier = e.veracity.toUpper();
        e.recallCount = s.recall;
        e.timestamp = QString::fromLatin1(s.timestamp);
        e.lastRecalled = s.recall > 0 ? e.timestamp : QString();
        e.supersededBy = QString::fromLatin1(s.supersededBy);
        e.status =
            e.supersededBy.isEmpty() ? QStringLiteral("active") : QStringLiteral("superseded");
        e.degradationLabel = degradationLabelFor(e.tier, e.tierLevel);
        e.trustWeight = trustWeightFor(e.veracity);
        e.degradationWeight = degradationWeightFor(e.tier, e.tierLevel);
        e.effectiveWeight = e.importance * e.trustWeight * e.degradationWeight;
        e.contaminated = e.veracity != QStringLiteral("stated");
        m_memories.append(e);
    }

    // Memory -> entity mentions (drives the constellation bipartite graph). These
    // auto-scope to the current bank because the graph builders only keep edges
    // whose memory endpoints are in the scoped (per-profile) set.
    m_mentions = {
        {QStringLiteral("m01"), QStringLiteral("Rust")},
        {QStringLiteral("m02"), QStringLiteral("Ada")},
        {QStringLiteral("m02"), QStringLiteral("daemon")},
        {QStringLiteral("m03"), QStringLiteral("daemon")},
        {QStringLiteral("m03"), QStringLiteral("Rust")},
        {QStringLiteral("m15"), QStringLiteral("daemon")},
        {QStringLiteral("m15"), QStringLiteral("Qt")},
        {QStringLiteral("m05"), QStringLiteral("Qt")},
        {QStringLiteral("m06"), QStringLiteral("SQLite")},
        {QStringLiteral("m13"), QStringLiteral("Mnemosyne")},
        {QStringLiteral("m04"), QStringLiteral("Mnemosyne")},
        {QStringLiteral("m04"), QStringLiteral("SQLite")},
        {QStringLiteral("m09"), QStringLiteral("Ada")},
    };

    // Memory -> memory references (the association graph), kept within a profile.
    m_refs = {
        {QStringLiteral("m02"), QStringLiteral("m03")},
        {QStringLiteral("m03"), QStringLiteral("m01")},
        {QStringLiteral("m16"), QStringLiteral("m02")},
        {QStringLiteral("m16"), QStringLiteral("m03")},
        {QStringLiteral("m16"), QStringLiteral("m15")},
        {QStringLiteral("m15"), QStringLiteral("m11")},
        {QStringLiteral("m05"), QStringLiteral("m06")},
        {QStringLiteral("m06"), QStringLiteral("m13")},
        {QStringLiteral("m04"), QStringLiteral("m09")},
        {QStringLiteral("m04"), QStringLiteral("m14")},
        {QStringLiteral("m07"), QStringLiteral("m08")},
    };

    // SPO facts (the knowledge graph), grouped per agent bank, with one
    // contradiction in the General Assistant bank.
    m_facts = {
        {QStringLiteral("prof-1"), QStringLiteral("Ada"), QStringLiteral("works_on"),
         QStringLiteral("daemon"), 0.95, false},
        {QStringLiteral("prof-1"), QStringLiteral("daemon"), QStringLiteral("uses"),
         QStringLiteral("Rust"), 0.9, false},
        {QStringLiteral("prof-1"), QStringLiteral("daemon"), QStringLiteral("uses"),
         QStringLiteral("Python"), 0.4, true},
        {QStringLiteral("prof-2"), QStringLiteral("Qt"), QStringLiteral("renders"),
         QStringLiteral("GUI"), 0.8, false},
        {QStringLiteral("prof-2"), QStringLiteral("Mnemosyne"), QStringLiteral("stored_in"),
         QStringLiteral("SQLite"), 0.85, false},
        {QStringLiteral("prof-3"), QStringLiteral("Mnemosyne"), QStringLiteral("is_a"),
         QStringLiteral("memory system"), 0.95, false},
        {QStringLiteral("prof-3"), QStringLiteral("Ada"), QStringLiteral("works_at"),
         QStringLiteral("platform team"), 0.9, false},
    };
}

bool MockMemoryService::inScope(const MemoryEntry& e) const {
    // Memory is owned by the agent: the profile (bank) is the primary filter.
    if (!m_profile.isEmpty() && e.profile != m_profile)
        return false;
    // Within the bank, an empty session means the whole bank; otherwise narrow to
    // that session's rows (optionally folding in global-scope rows).
    if (m_session.isEmpty())
        return true;
    if (e.sessionId == m_session)
        return true;
    return m_includeGlobal && e.scope == QStringLiteral("global");
}

QList<MemoryEntry> MockMemoryService::scoped() const {
    QList<MemoryEntry> out;
    for (const MemoryEntry& e : m_memories) {
        if (inScope(e))
            out.append(e);
    }
    return out;
}

MemoryStats MockMemoryService::computeStats() const {
    MemoryStats s;
    for (const MemoryEntry& e : scoped()) {
        if (e.tier == QStringLiteral("working"))
            s.working += 1;
        else if (e.tier == QStringLiteral("episodic"))
            s.episodic += 1;
        else if (e.tier == QStringLiteral("scratchpad"))
            s.scratchpad += 1;
        bump(s.bySource, e.source);
        bump(s.byScope, e.scope);
        bump(s.byVeracity, e.veracity);
        if (!e.degradationLabel.isEmpty())
            bump(s.byDegradation, e.degradationLabel);
        bump(s.bySession, e.sessionId);
    }
    // Facts/triples/conflicts are bank-scoped (the current agent's profile).
    for (const Fact& f : m_facts) {
        if (!m_profile.isEmpty() && f.profile != m_profile)
            continue;
        s.facts += 1;
        s.triples += 1;
        if (f.contradicts)
            s.conflicts += 1;
    }
    return s;
}

MemoryGraph MockMemoryService::buildAssociation(const QString& seedId, int depth, int limit) const {
    MemoryGraph g;
    g.kind = QStringLiteral("association");

    QSet<QString> allowed;
    for (const MemoryEntry& e : scoped())
        allowed.insert(e.id);

    // Optional BFS expansion from a seed memory over reference edges.
    QSet<QString> include;
    if (seedId.isEmpty()) {
        include = allowed;
    } else if (allowed.contains(seedId)) {
        include.insert(seedId);
        QSet<QString> frontier{seedId};
        for (int d = 0; d < std::max(depth, 1); ++d) {
            QSet<QString> next;
            for (const auto& ref : m_refs) {
                if (frontier.contains(ref.first) && allowed.contains(ref.second))
                    next.insert(ref.second);
                if (frontier.contains(ref.second) && allowed.contains(ref.first))
                    next.insert(ref.first);
            }
            next.subtract(include);
            include.unite(next);
            frontier = next;
            if (frontier.isEmpty())
                break;
        }
    }

    QHash<QString, MemoryEntry> byId;
    for (const MemoryEntry& e : m_memories)
        byId.insert(e.id, e);

    int count = 0;
    for (const MemoryEntry& e : scoped()) {
        if (!include.contains(e.id))
            continue;
        if (limit > 0 && count >= limit)
            break;
        MemoryNode n;
        n.id = e.id;
        n.kind = QStringLiteral("memory");
        n.label = e.content.left(48);
        n.weight = e.importance;
        n.meta.insert(QStringLiteral("tier"), e.tier);
        n.meta.insert(QStringLiteral("veracity"), e.veracity);
        g.nodes.append(n);
        count += 1;
    }
    QSet<QString> present;
    for (const MemoryNode& n : g.nodes)
        present.insert(n.id);
    for (const auto& ref : m_refs) {
        if (present.contains(ref.first) && present.contains(ref.second))
            g.edges.append(
                MemoryEdge{ref.first, ref.second, QStringLiteral("references"), 0.8, QString()});
    }
    return g;
}

MemoryGraph MockMemoryService::buildKnowledge(int limit) const {
    MemoryGraph g;
    g.kind = QStringLiteral("knowledge");
    // Knowledge is bank-scoped: only this agent's (profile's) facts.
    const auto bankFact = [this](const Fact& f) {
        return m_profile.isEmpty() || f.profile == m_profile;
    };
    QHash<QString, int> mentions; // entity -> mention count
    for (const Fact& f : m_facts) {
        if (!bankFact(f))
            continue;
        mentions[f.subject] += 1;
        mentions[f.object] += 1;
    }
    QSet<QString> added;
    const auto addEntity = [&](const QString& name) {
        if (added.contains(name))
            return;
        added.insert(name);
        MemoryNode n;
        n.id = QStringLiteral("ent:") + name;
        n.kind = QStringLiteral("entity");
        n.label = name;
        n.weight = static_cast<double>(mentions.value(name, 1));
        g.nodes.append(n);
    };
    int count = 0;
    QString prevSubjectFact;
    for (const Fact& f : m_facts) {
        if (!bankFact(f))
            continue;
        if (limit > 0 && count >= limit)
            break;
        addEntity(f.subject);
        addEntity(f.object);
        MemoryEdge e;
        e.source = QStringLiteral("ent:") + f.subject;
        e.target = QStringLiteral("ent:") + f.object;
        e.edgeType = f.predicate;
        e.label = f.predicate;
        e.weight = f.confidence;
        g.edges.append(e);
        // Mark a contradiction edge back to the prior fact's object node.
        if (f.contradicts && !prevSubjectFact.isEmpty()) {
            g.edges.append(MemoryEdge{QStringLiteral("ent:") + f.object, prevSubjectFact,
                                      QStringLiteral("contradicts"), 1.0,
                                      QStringLiteral("contradicts")});
        }
        prevSubjectFact = QStringLiteral("ent:") + f.object;
        count += 1;
    }
    return g;
}

MemoryGraph MockMemoryService::buildConstellation(int limit) const {
    MemoryGraph g;
    g.kind = QStringLiteral("constellation");
    QSet<QString> allowed;
    for (const MemoryEntry& e : scoped())
        allowed.insert(e.id);

    QStringList memoryIds;
    int count = 0;
    for (const MemoryEntry& e : scoped()) {
        if (limit > 0 && count >= limit)
            break;
        MemoryNode n;
        n.id = e.id;
        n.kind = QStringLiteral("memory");
        n.label = e.content.left(40);
        n.weight = e.importance;
        n.meta.insert(QStringLiteral("source"), e.source);
        g.nodes.append(n);
        memoryIds.append(e.id);
        count += 1;
    }

    QHash<QString, int> entityCounts;
    for (const Mention& m : m_mentions) {
        if (allowed.contains(m.memId))
            entityCounts[m.entity] += 1;
    }
    QStringList entityIds;
    for (auto it = entityCounts.constBegin(); it != entityCounts.constEnd(); ++it) {
        MemoryNode n;
        n.id = QStringLiteral("ent:") + it.key();
        n.kind = QStringLiteral("entity");
        n.label = it.key();
        n.weight = static_cast<double>(it.value());
        g.nodes.append(n);
        entityIds.append(n.id);
    }
    for (const Mention& m : m_mentions) {
        if (!allowed.contains(m.memId))
            continue;
        g.edges.append(MemoryEdge{m.memId, QStringLiteral("ent:") + m.entity,
                                  QStringLiteral("mentions"), 1.0, QString()});
    }
    QSet<QString> present(memoryIds.constBegin(), memoryIds.constEnd());
    for (const auto& ref : m_refs) {
        if (present.contains(ref.first) && present.contains(ref.second))
            g.edges.append(
                MemoryEdge{ref.first, ref.second, QStringLiteral("references"), 0.6, QString()});
    }
    g.clusters.append(
        MemoryCluster{QStringLiteral("memories"), QStringLiteral("Memories"), memoryIds});
    g.clusters.append(
        MemoryCluster{QStringLiteral("entities"), QStringLiteral("Entities"), entityIds});
    return g;
}

QList<MemoryTimelineGroup> MockMemoryService::buildTimeline(const QString& group, int limit) const {
    QList<MemoryEntry> items = scoped();
    std::sort(items.begin(), items.end(),
              [](const MemoryEntry& a, const MemoryEntry& b) { return a.timestamp > b.timestamp; });

    QList<MemoryTimelineGroup> groups;
    const auto groupKeyFor = [&group](const MemoryEntry& e) -> QString {
        if (group == QStringLiteral("session"))
            return e.sessionId;
        return e.timestamp.left(10); // day (YYYY-MM-DD)
    };
    const auto findGroup = [&groups](const QString& key) -> MemoryTimelineGroup* {
        for (MemoryTimelineGroup& g : groups) {
            if (g.key == key)
                return &g;
        }
        groups.append(MemoryTimelineGroup{key, {}});
        return &groups.last();
    };

    int count = 0;
    for (const MemoryEntry& e : items) {
        if (limit > 0 && count >= limit)
            break;
        MemoryTimelineGroup* g = findGroup(groupKeyFor(e));
        MemoryEvent ev;
        ev.seq = static_cast<quint64>(count + 1);
        ev.kind = e.source == QStringLiteral("consolidation") ? QStringLiteral("consolidated")
                                                              : QStringLiteral("added");
        ev.memoryId = e.id;
        ev.at = e.timestamp;
        ev.summary = e.content.left(60);
        g->events.append(ev);
        count += 1;
        if (e.recallCount > 0) {
            MemoryEvent rec;
            rec.seq = static_cast<quint64>(count + 1000);
            rec.kind = QStringLiteral("recalled");
            rec.memoryId = e.id;
            rec.at = e.lastRecalled;
            rec.summary = QStringLiteral("Recalled %1x").arg(e.recallCount);
            g->events.append(rec);
        }
    }
    return groups;
}

void MockMemoryService::requestStats() {
    const MemoryStats s = computeStats();
    QMetaObject::invokeMethod(this, [this, s] { emit statsReady(s); }, Qt::QueuedConnection);
}

void MockMemoryService::requestList(const QVariantMap& query) {
    const QString tier = query.value(QStringLiteral("tier")).toString();
    const QString source = query.value(QStringLiteral("source")).toString();
    const QString veracity = query.value(QStringLiteral("veracity")).toString();
    const QString status = query.value(QStringLiteral("status")).toString();
    const QString text = query.value(QStringLiteral("text")).toString().trimmed();
    const QString sort = query.value(QStringLiteral("sort"), QStringLiteral("recent")).toString();
    const int limit = query.value(QStringLiteral("limit"), 50).toInt();

    QList<MemoryEntry> items;
    for (const MemoryEntry& e : scoped()) {
        if (!tier.isEmpty() && e.tier != tier)
            continue;
        if (!source.isEmpty() && e.source != source)
            continue;
        if (!veracity.isEmpty() && e.veracity != veracity)
            continue;
        if (!status.isEmpty() && e.status != status)
            continue;
        if (!text.isEmpty() && !e.content.contains(text, Qt::CaseInsensitive))
            continue;
        items.append(e);
    }
    if (sort == QStringLiteral("importance")) {
        std::sort(items.begin(), items.end(), [](const MemoryEntry& a, const MemoryEntry& b) {
            return a.importance > b.importance;
        });
    } else if (sort == QStringLiteral("recall")) {
        std::sort(items.begin(), items.end(), [](const MemoryEntry& a, const MemoryEntry& b) {
            return a.recallCount > b.recallCount;
        });
    } else {
        std::sort(items.begin(), items.end(), [](const MemoryEntry& a, const MemoryEntry& b) {
            return a.timestamp > b.timestamp;
        });
    }
    if (limit > 0 && items.size() > limit)
        items = items.mid(0, limit);

    QMetaObject::invokeMethod(
        this, [this, items] { emit pageReady(QStringLiteral("list"), items, QString()); },
        Qt::QueuedConnection);
}

void MockMemoryService::requestGet(const QString& id) {
    MemoryEntry found;
    bool ok = false;
    for (const MemoryEntry& e : m_memories) {
        if (e.id == id) {
            found = e;
            ok = true;
            break;
        }
    }
    QMetaObject::invokeMethod(
        this, [this, id, found, ok] { emit entryReady(id, found, ok); }, Qt::QueuedConnection);
}

void MockMemoryService::requestGraph(const QString& kind, const QString& seed, int depth,
                                     int limit) {
    MemoryGraph g;
    if (kind == QStringLiteral("knowledge"))
        g = buildKnowledge(limit);
    else if (kind == QStringLiteral("constellation"))
        g = buildConstellation(limit);
    else
        g = buildAssociation(seed, depth, limit);
    QMetaObject::invokeMethod(this, [this, g] { emit graphReady(g); }, Qt::QueuedConnection);
}

void MockMemoryService::requestSearch(const QString& text, int limit) {
    const QString needle = text.trimmed();
    QList<MemoryEntry> items;
    for (const MemoryEntry& e : scoped()) {
        if (needle.isEmpty() || e.content.contains(needle, Qt::CaseInsensitive)) {
            MemoryEntry hit = e;
            // Cheap relevance: text presence + importance + recency tilt.
            hit.score = (needle.isEmpty() ? 0.0 : 0.5) + e.importance * 0.5 +
                        (e.recallCount > 0 ? 0.1 : 0.0);
            items.append(hit);
        }
    }
    std::sort(items.begin(), items.end(),
              [](const MemoryEntry& a, const MemoryEntry& b) { return a.score > b.score; });
    if (limit > 0 && items.size() > limit)
        items = items.mid(0, limit);
    QMetaObject::invokeMethod(
        this, [this, items] { emit pageReady(QStringLiteral("search"), items, QString()); },
        Qt::QueuedConnection);
}

void MockMemoryService::requestTimeline(const QString& group, int limit) {
    const QList<MemoryTimelineGroup> groups = buildTimeline(group, limit);
    QMetaObject::invokeMethod(
        this, [this, groups] { emit timelineReady(groups); }, Qt::QueuedConnection);
}

void MockMemoryService::requestSessions(const QString& profile) {
    // Distinct sessions (sessions) within this agent's bank, in first-seen
    // order, for the Memory page's session-filter facet.
    QVariantList sessions;
    QSet<QString> seen;
    for (const MemoryEntry& e : m_memories) {
        if (e.profile != profile || e.sessionId.isEmpty() || seen.contains(e.sessionId))
            continue;
        seen.insert(e.sessionId);
        sessions.append(QVariantMap{
            {QStringLiteral("id"), e.sessionId},
            {QStringLiteral("label"), e.sessionId},
        });
    }
    QMetaObject::invokeMethod(
        this, [this, profile, sessions] { emit sessionsReady(profile, sessions); },
        Qt::QueuedConnection);
}

void MockMemoryService::startWatch() {
    if (!m_watch.isActive())
        m_watch.start();
}

void MockMemoryService::stopWatch() {
    m_watch.stop();
}

void MockMemoryService::emitTick() {
    const QList<MemoryEntry> items = scoped();
    if (items.isEmpty())
        return;
    static const char* kinds[] = {"recalled", "added", "updated", "consolidated"};
    m_seq += 1;
    const MemoryEntry& e = items.at(static_cast<int>(m_seq % static_cast<quint64>(items.size())));
    MemoryEvent ev;
    ev.seq = m_seq;
    ev.kind = QString::fromLatin1(kinds[m_seq % 4]);
    ev.memoryId = e.id;
    ev.at = QStringLiteral("live");
    ev.summary = e.content.left(60);
    emit memoryEvent(ev);
}

} // namespace memory
