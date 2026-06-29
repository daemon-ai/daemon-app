// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include <QtQml/qqmlextensionplugin.h>
#include <QtQuickTest/quicktest.h>

// DaemonApp.Theme is a STATIC QML module, so its plugin must be referenced
// explicitly or the linker drops it and the `import DaemonApp.Theme` fails.
// Theme only imports QtQuick (no Controls), so this runs offscreen without a
// Controls style.
Q_IMPORT_QML_PLUGIN(DaemonApp_ThemePlugin)

QUICK_TEST_MAIN(tst_theme)
