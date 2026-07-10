// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "outbox_types.h"

#include <QList>
#include <QObject>
#include <QString>

QT_FORWARD_DECLARE_CLASS(QSqlQuery)

namespace mirror {

// One client-local sidecar row per conversation (spec 09 §3.4). Keyed by the conversation's
// canonical entity key (transport\x1fid). These are client-instance facts (unread bookkeeping,
// drafts, mute/collapse), NOT domain state — they live here, never in a mirror table (§3.3 gate).
struct SidecarConversation {
    QString key; // canonical Conversation key
    int unreadCount = 0;
    qint64 lastReadCursor = 0;
    bool muted = false;
    bool collapsed = false;
    QString draft;
    QString draftRefs; // storage bookkeeping (see §3.4 DDL sketch)
    qint64 updatedMs = 0;
};

// One client-local sidecar row per session (spec 09 §3.4): the composer draft that survives every
// mirror drop/rebuild.
struct SidecarSession {
    QString key; // canonical Session key
    QString draft;
    QString draftRefs;
    qint64 updatedMs = 0;
};

// The PRECIOUS per-identity `local-<id>.db` (spec 09 §4.5): the durable home of the outbox (§6.1)
// and the sidecar tables (§3.4). Unlike the disposable `mirror-<id>.db` / `daemon_cache.db`
// (drop-and-rebuild on a schema bump), this database is **versioned and MIGRATED, never dropped** —
// a mirror nuke or app update cannot destroy a pending op or a user-authored draft (C3, made
// structural by keeping the two files separate).
//
// This is A2's module. The store/journal (`mirror-<id>.db`) is A1's; this class never touches it.
class LocalDatabase : public QObject {
public:
    // Current local-db schema version. Bumped by hand when the DDL changes; every prior version
    // has a forward migration (never a drop). See `ensureSchema`.
    static constexpr int kSchemaVersion = 2;

    explicit LocalDatabase(const QString& dbPath = QString(), QObject* parent = nullptr);
    ~LocalDatabase() override;

    // Per-identity namespacing (spec 09 §4.5): reopen under a per-user filename hash so a user
    // switch never surfaces the prior principal's ops/drafts. Empty resets to the shared/default
    // file (pre-auth). Unlike the cache, the file is migrated (not dropped) if it already exists.
    void setIdentityNamespace(const QString& userKey);

    [[nodiscard]] bool isOpen() const;
    [[nodiscard]] int schemaVersion() const; // persisted local_meta.schema_version (0 if absent)
    [[nodiscard]] QString databasePath() const { return m_dbPath; }
    [[nodiscard]] QString lastError() const { return m_lastError; }

    // --- outbox rows (spec 09 §6.1) ---
    bool insertOp(const PendingOp& op);
    bool updateOp(const PendingOp& op); // state/attempts/last_error/payload/display
    bool deleteOp(const QString& opId);
    // FIFO by (enqueued_ms, op_id) so a lane's order is stable and export-friendly (UUIDv7 tie-
    // break preserves mint order — §6.2).
    [[nodiscard]] QList<PendingOp> loadOps() const;
    [[nodiscard]] int laneDepth(const QString& lane) const;
    [[nodiscard]] int totalDepth() const;
    // Boot transition (spec 09 §6.6): every `inflight` row reverts to `pending` (at-least-once).
    bool resetInflightToPending();

    // --- sidecar tables (spec 09 §3.4) ---
    bool upsertSidecarConversation(const SidecarConversation& row);
    [[nodiscard]] SidecarConversation sidecarConversation(const QString& key) const;
    [[nodiscard]] QList<SidecarConversation> sidecarConversations() const;
    bool upsertSidecarSession(const SidecarSession& row);
    [[nodiscard]] SidecarSession sidecarSession(const QString& key) const;

    bool setMeta(const QString& key, const QString& value);
    [[nodiscard]] QString meta(const QString& key) const;

    // True if column `column` exists on `table` (used by the migration test to prove a forward
    // ALTER landed without dropping rows).
    [[nodiscard]] bool hasColumn(const QString& table, const QString& column) const;

private:
    [[nodiscard]] static QString defaultDatabasePath();
    [[nodiscard]] QString namespacedPath(const QString& userKey) const;
    void openAt(const QString& path);
    bool openConnection();
    void dropConnection();
    // Create the meta table (never dropped), then either create the current tables (fresh db) or
    // run the forward migration ladder from the persisted version to `kSchemaVersion`. NEVER drops
    // user data.
    bool ensureSchema();
    bool createCurrentTables();
    bool migrateFrom(int storedVersion);
    bool execSql(const char* sql);
    bool execWrite(QSqlQuery& query);
    void setLastError(const QString& message) const;

    QString m_dbPath;
    QString m_basePath;
    QString m_connectionName;
    mutable QString m_lastError;
};

} // namespace mirror
