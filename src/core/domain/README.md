# core/domain

Pure C++ entities and value types. No UI, no SQL, no platform code (Qt6::Core
only). Target: `da_domain`.

Modeled now:
- `Session` - a chat thread.

Deferred until we model them:
- `Message` (an individual user/assistant/system turn within a session),
  `Folder`/`Project`, `Tag`.
