#pragma once

#include "persistence/in_memory_session_store.h"

#include <QString>

namespace persistence {

// A durable ISessionStore: the same in-memory tree/tags/sessions model as
// InMemorySessionStore (so all the recursive query + session-action logic is
// reused verbatim), but loaded from and written through to a local SQLite file.
//
// This is the client-local cache the audit (first-run-config-audit.md S4.3) calls
// for: sessions and archive survive restarts and are browsable offline. The
// daemon, when wired, hydrates this same store; nothing in the UI changes.
//
// Strategy: the base class holds the authoritative copy in RAM and emits
// changed() on every mutation. We load the RAM copy from disk at construction
// (seeding the demo data only when the database is empty), and persist a full
// snapshot whenever changed() fires. The dataset is small and local, so a
// whole-snapshot write is simpler and safe than per-row dirty tracking.
//
// On-disk table/column names keep their historical `conversations`/`agent_id`
// spelling to avoid a data migration; only the C++ vocabulary is daemon-aligned.
class SqliteSessionStore : public InMemorySessionStore {
    Q_OBJECT

public:
    // `dbPath` is the SQLite file to open. When empty, a per-user location under
    // QStandardPaths::AppDataLocation is used. Tests pass a temp path.
    explicit SqliteSessionStore(const QString& dbPath = QString(),
                                QObject* parent = nullptr);
    ~SqliteSessionStore() override;

    // The resolved on-disk path of the database (for diagnostics / tests).
    [[nodiscard]] QString databasePath() const { return m_dbPath; }

private:
    // Resolve the default database path (creating its directory) when none given.
    [[nodiscard]] static QString defaultDatabasePath();
    void openDatabase();
    void createSchema();
    // Load units/tags/sessions + id counters from disk into the base members.
    // Returns true when any sessions/units existed (i.e. not a fresh db).
    bool loadAll();
    // Write the full in-memory snapshot back to disk in one transaction.
    void saveAll();

    QString m_dbPath;
    QString m_connectionName;
};

} // namespace persistence
