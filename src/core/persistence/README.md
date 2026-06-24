# core/persistence

Storage layer. Owns the database connection on a dedicated worker thread and maps
rows to/from `domain` types. Target: `da_persistence` (Qt6::Sql) -> `da_domain`.

Planned contents:
- `db_manager.*` - GUI-thread facade over a `QThread`-bound worker.
- repositories - session (and later message/tag) queries.
- `migrations/` - schema creation and version upgrades.
