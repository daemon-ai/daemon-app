# core/domain

Pure C++ entities and value types. No UI, no SQL, no platform code (Qt6::Core
only). Target: `da_domain`.

Modeled now:
- `Session` (`session.h`) - a chat thread, mirroring the wire `SessionInfo`
  (id, bound profile, lifecycle, role, state, parent, pinned/archived).
- `SessionLogEntry` (`session_log.h`) - a transcript envelope (seq, direction,
  disposition, origin) plus a decoded payload; the daemon turn stream
  (AgentEvents) projects into these.
- `Origin` (`origin.h`), `Tag` (`tag.h`), `Participant` (`participant.h`),
  `UnitNode` (`unit_node.h`), `DeliveryTarget` (`delivery.h`),
  `SidebarNode`/`ListScope` (`sidebar_node.h`), and the id newtypes (`ids.h`).

Deferred until we model them:
- `Folder`/`Project`. Per-message structured content beyond the transcript-log
  envelope is rendered through the BlockEditor, not a domain `Message` type.
