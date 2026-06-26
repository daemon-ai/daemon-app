#include <QtCore/qstring.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlextensionplugin.h>
#include <QtQuickTest/quicktest.h>

// The tab strip and its dependencies are STATIC QML modules, so each plugin must
// be referenced explicitly or the linker drops it and the imports fail. TabBar
// pulls in Theme (tokens + FontIcons) and Controls (IconButton).
Q_IMPORT_QML_PLUGIN(DaemonApp_ThemePlugin)
Q_IMPORT_QML_PLUGIN(DaemonApp_ControlsPlugin)
Q_IMPORT_QML_PLUGIN(DaemonApp_TabsPlugin)

// TabBar / StackLayout import QtQuick.Controls + QtQuick.Layouts, whose QML
// modules live in the Qt installation's qml dir rather than on the test's default
// import path. TEST_QML_IMPORT_PATH is baked in at configure time (from the dev
// environment's QML2_IMPORT_PATH) and added to the engine so the imports resolve.
class TabsTestSetup : public QObject {
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

QUICK_TEST_MAIN_WITH_SETUP(tst_tabs, TabsTestSetup)

#include "tst_tabs_main.moc"
