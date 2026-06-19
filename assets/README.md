# assets

Static resources (`fonts/`, `images/`, `icons/`). These are registered into a
small QML resource module (`DaemonApp.Assets`) so QML/C++ reference them via the
`/qt/qml` resource path. Wired when the first module needs an asset.
