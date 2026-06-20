#include <QtQuickTest/quicktest.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlextensionplugin.h>
#include <QtCore/qstring.h>

// The composer and its dependencies are STATIC QML modules, so each plugin must
// be referenced explicitly or the linker drops it and the imports fail. Composer
// pulls in Theme (tokens + FontIcons) and Controls (IconButton/TextButton).
Q_IMPORT_QML_PLUGIN(DaemonApp_ThemePlugin)
Q_IMPORT_QML_PLUGIN(DaemonApp_ControlsPlugin)
Q_IMPORT_QML_PLUGIN(DaemonApp_ComposerPlugin)

// The composer imports QtQuick.Controls, whose QML modules (incl. the Basic
// style) live in the Qt installation's qml dir rather than on the test's default
// import path. TEST_QML_IMPORT_PATH is baked in at configure time (from the dev
// environment's QML2_IMPORT_PATH) and added to the engine so the imports resolve.
class ComposerTestSetup : public QObject {
    Q_OBJECT

public slots:
    void qmlEngineAvailable(QQmlEngine* engine)
    {
#ifdef TEST_QML_IMPORT_PATH
        const QString paths = QStringLiteral(TEST_QML_IMPORT_PATH);
        const auto parts = paths.split(QLatin1Char(':'), Qt::SkipEmptyParts);
        for (const QString& p : parts)
            engine->addImportPath(p);
#endif
        Q_UNUSED(engine)
    }
};

QUICK_TEST_MAIN_WITH_SETUP(tst_composer, ComposerTestSetup)

#include "tst_composer_main.moc"
