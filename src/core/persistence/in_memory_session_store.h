#pragma once

#include "persistence/isession_store.h"

namespace persistence {

// In-memory ISessionStore seeded with sample data, so the UI is fully alive
// without a backend. Not persisted across runs. The sample data is a deliberate
// fleet-of-fleets that exercises arbitrary depth (orchestrators nested inside
// orchestrators) and a lone-unit root, so the recursive tree code is tested.
class InMemorySessionStore : public ISessionStore {
    Q_OBJECT

public:
    explicit InMemorySessionStore(QObject* parent = nullptr);

    [[nodiscard]] QList<domain::UnitNode>
    unitChildren(const domain::UnitId& parentId) const override;
    [[nodiscard]] domain::UnitNode unit(const domain::UnitId& id) const override;
    [[nodiscard]] QList<domain::Tag> tags() const override;
    [[nodiscard]] QList<domain::Session>
    sessions(const domain::ListScope& scope) const override;
    [[nodiscard]] int sessionCount(const domain::ListScope& scope) const override;
    [[nodiscard]] QString content(int sessionId) const override;
    [[nodiscard]] QString title(int sessionId) const override;

    int createSession(const domain::UnitId& unitId) override;
    domain::UnitId createUnit(const domain::UnitId& parentId, domain::UnitKind kind) override;
    int createTag(const QString& name, const QString& color) override;
    void setContent(int sessionId, const QString& markdown) override;
    void setArchived(int sessionId, bool archived) override;
    void renameSession(int sessionId, const QString& title) override;
    void deleteSession(int sessionId) override;
    void setPinned(int sessionId, bool pinned) override;
    [[nodiscard]] bool isPinned(int sessionId) const override;
    void moveSession(int sessionId, int delta) override;

protected:
    // Subclass entry point: build the base without the sample seed (a durable
    // store loads its rows from disk instead and seeds only when empty).
    InMemorySessionStore(QObject* parent, bool seed);

    // Populate the in-memory tree/tags/sessions with the canonical demo data.
    // Protected so a durable subclass can seed a fresh (empty) database.
    void seedSampleData();

    // Derive a unit's daemon-parity metadata (profile / session / role) from its
    // id + kind. These fields are NOT persisted (the SQLite schema predates them),
    // so this is applied uniformly after both seeding and loading so every unit
    // carries an agent identity regardless of where it came from. The mapping is
    // deterministic: `session == id`, roots are Primary (children ManagedChild,
    // with one EphemeralSubagent for variety), and a small id->profile table binds
    // engine/orchestrator units to a seeded profile (Hosts get no profile).
    static void applyUnitMeta(domain::UnitNode& n);

    // In-memory state. Protected so a durable subclass (e.g. the SQLite store) can
    // load these from disk and persist them write-through. The query and mutation
    // logic above operates on exactly these members.
    QList<domain::UnitNode> m_units;
    QList<domain::Tag> m_tags;
    QList<domain::Session> m_sessions;
    int m_nextId = 1;        // session ids
    int m_nextUnitSeq = 1;   // suffix for generated unit ids
    int m_nextTagId = 1;     // tag ids

private:
    [[nodiscard]] bool matchesScope(const domain::Session& c,
                                    const domain::ListScope& scope) const;
    // True when `unitId` is `rootId` or any descendant of it (subtree fold).
    // Walks the parent chain - a single recursive rule for every depth.
    [[nodiscard]] bool isInSubtree(const domain::UnitId& unitId,
                                   const domain::UnitId& rootId) const;
    // Canonical demo transcript markdown exercising every Phase 1 agent block,
    // seeded as a session for visual inspection of the renderers.
    [[nodiscard]] static QString agentBlocksSampleMarkdown();
    [[nodiscard]] static QString roleLayerSampleMarkdown();
};

} // namespace persistence
