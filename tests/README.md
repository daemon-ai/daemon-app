# tests

- `unit/` - C++ unit tests (QTest) for `core` (domain, persistence) and view models.
- `qml/` - QtQuickTest for QML modules.

Wired into the build (`enable_testing()` + `add_subdirectory(tests)` from the root
`CMakeLists.txt`) alongside the code they cover.
