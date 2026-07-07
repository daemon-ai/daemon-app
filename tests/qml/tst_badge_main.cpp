// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include <QtCore/qstring.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlextensionplugin.h>
#include <QtQuickTest/quicktest.h>

// DaemonApp.Theme and DaemonApp.Controls are STATIC QML modules, so their
// plugins must be referenced explicitly or the linker drops them and the
// imports fail at runtime.
Q_IMPORT_QML_PLUGIN(DaemonApp_ThemePlugin)
Q_IMPORT_QML_PLUGIN(DaemonApp_ControlsPlugin)

// Badge pulls in QtQuick.Controls (Label), whose QML modules live in the Qt
// installation's qml dir; TEST_QML_IMPORT_PATH is baked in at configure time so
// the imports resolve offscreen (Basic style).
class BadgeTestSetup : public QObject {
    Q_OBJECT

public slots:
    void qmlEngineAvailable(QQmlEngine* engine) {
#ifdef TEST_QML_IMPORT_PATH
        const QString paths = QStringLiteral(TEST_QML_IMPORT_PATH);
        const auto parts = paths.split(QLatin1Char(':'), Qt::SkipEmptyParts);
        for (const QString& p : parts)
            engine->addImportPath(p);
#endif
        Q_UNUSED(engine)
    }
};

QUICK_TEST_MAIN_WITH_SETUP(tst_badge, BadgeTestSetup)

#include "tst_badge_main.moc"
