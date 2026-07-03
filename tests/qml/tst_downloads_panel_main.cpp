// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "models/mock_model_catalog.h"

#include <QtCore/qstandardpaths.h>
#include <QtCore/qstring.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlextensionplugin.h>
#include <QtQuickTest/quicktest.h>

// DownloadsPanel lives in the STATIC DaemonApp.Pages module; its plugin and the
// Theme/Controls plugins the panel imports must be referenced explicitly or the
// linker drops them and the imports fail (same pattern as tst_composer).
Q_IMPORT_QML_PLUGIN(DaemonApp_ThemePlugin)
Q_IMPORT_QML_PLUGIN(DaemonApp_ControlsPlugin)
Q_IMPORT_QML_PLUGIN(DaemonApp_PagesPlugin)

// The panel binds the app-global ModelCatalog context property (injected by
// Application::start in production); the test injects a MockModelCatalog whose
// test-only seedDownloadRows() pins deterministic row states. TEST_QML_IMPORT_PATH
// mirrors tst_composer: QtQuick.Controls (incl. the Basic style) lives in the Qt
// installation's qml dir rather than on the test's default import path.
class DownloadsPanelTestSetup : public QObject {
    Q_OBJECT

public:
    DownloadsPanelTestSetup() {
        // Keep the mock's models.json cache out of the real user dirs (the mock
        // persists its installed set through appcache on state mutations).
        QStandardPaths::setTestModeEnabled(true);
    }

public slots:
    void qmlEngineAvailable(QQmlEngine* engine) {
#ifdef TEST_QML_IMPORT_PATH
        const QString paths = QStringLiteral(TEST_QML_IMPORT_PATH);
        const auto parts = paths.split(QLatin1Char(':'), Qt::SkipEmptyParts);
        for (const QString& p : parts)
            engine->addImportPath(p);
#endif
        if (m_catalog == nullptr) {
            m_catalog = new models::MockModelCatalog(this);
        }
        engine->rootContext()->setContextProperty(QStringLiteral("ModelCatalog"), m_catalog);
    }

private:
    models::MockModelCatalog* m_catalog = nullptr;
};

QUICK_TEST_MAIN_WITH_SETUP(tst_downloads_panel, DownloadsPanelTestSetup)

#include "tst_downloads_panel_main.moc"
