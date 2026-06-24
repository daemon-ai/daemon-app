# DaemonApp

QML feature modules. Each subfolder is one module whose path mirrors its URI
(`DaemonApp.<Feature>`), defined with `qt_add_qml_module` and bundling its `*.qml`
plus any backing C++ view model. The `App` module is the executable that composes
the rest; nothing depends on `App`.

Modules:
- `App` - shell window + `main()`.
- `Theme` - palette / theming.
- `Controls` - reusable UI kit.
- `BlockEditor` - markdown, code, table, image, math, and diagram transcript rendering.
- `Editor` - file editor tabs and syntax highlighting controller.
- `Turn` - scripted pre-backend turn event source.
- `ComposerSession` - composer state, model picker, commands, attachments, and completions.
- `Composer` - bottom composer chrome and overlays.
- `Sidebar` - navigation (All Sessions / Archived / Folders / Tags).
- `SessionsList` - the list of chat threads.
- `Session` - tabbed center pane hosting transcripts, manager pages, memory/profile tabs, and files.
- `Tabs` - shared tab model and tab bar.
- `Files` - file explorer, finder, picker, and models.
- `Transcript` - scrollable transcript and find bar.
- `Settings` - per-transcript settings popup and persisted UI settings.
- `Pages` - app-level manager/settings pages.
- `Memory` - memory overview, list, graph, timeline, and profile surfaces.
- `StatusModel` - shared status/footer model.
- `StatusBar` - footer chrome and gateway menu.
- `Command` - command palette registry.
- `Export` - transcript export support.

Most backend-facing modules are wired to mock or local-dev services today. Keep
new data access behind `src/core` interfaces so daemon adapters can replace those
services in one place.
