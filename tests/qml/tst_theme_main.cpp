#include <QtQuickTest/quicktest.h>
#include <QtQml/qqmlextensionplugin.h>

// DaemonApp.Theme is a STATIC QML module, so its plugin must be referenced
// explicitly or the linker drops it and the `import DaemonApp.Theme` fails.
// Theme only imports QtQuick (no Controls), so this runs offscreen without a
// Controls style.
Q_IMPORT_QML_PLUGIN(DaemonApp_ThemePlugin)

QUICK_TEST_MAIN(tst_theme)
