# DaemonApp

QML feature modules. Each subfolder is one module whose path mirrors its URI
(`DaemonApp.<Feature>`), defined with `qt_add_qml_module` and bundling its `*.qml`
plus any backing C++ view model. The `App` module is the executable that composes
the rest; nothing depends on `App`.

Modules:
- `App` - shell window + `main()`.
- `Theme` - palette / theming.
- `Controls` - reusable UI kit.
- `Sidebar` - navigation (All Sessions / Archived / Folders / Tags).
- `SessionsList` - the list of chat threads.
- `Session` - center container; composes `Transcript` above `Composer`.
- `Transcript` - scrollable list of message bubbles.
- `Composer` - bottom-middle message input.
- `Settings` - settings menu.

A markdown/code/diagram renderer is deferred and will be added later (e.g.
`DaemonApp.RichText`) or inside `Transcript`.
