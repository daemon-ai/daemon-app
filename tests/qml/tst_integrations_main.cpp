// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "auth/auth_flow_controller.h"
#include "transports/mock_persons_service.h"
#include "transports/mock_transport_registry.h"

#include <QtCore/qstring.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlextensionplugin.h>
#include <QtQuickTest/quicktest.h>

// IntegrationsTree lives in the STATIC DaemonApp.Sidebar module; its plugin and the Theme/Controls
// plugins it imports (plus DaemonApp.Pages for the mounted AuthFlowSheet) must be referenced
// explicitly or the linker drops them and the imports fail (same pattern as tst_downloads_panel).
Q_IMPORT_QML_PLUGIN(DaemonApp_ThemePlugin)
Q_IMPORT_QML_PLUGIN(DaemonApp_ControlsPlugin)
Q_IMPORT_QML_PLUGIN(DaemonApp_PagesPlugin)
Q_IMPORT_QML_PLUGIN(DaemonApp_SidebarPlugin)

// The tree binds the app-global Transports / Persons context properties (injected by
// Application::registerContext in production); the AuthFlowSheet binds AuthFlow. The test injects
// the in-repo mocks. The AuthFlowController is constructed WITHOUT a service so start() fails
// closed instead of binding a loopback redirect sink (offscreen runs must never bind ports).
class IntegrationsTestSetup : public QObject {
    Q_OBJECT

public slots:
    void qmlEngineAvailable(QQmlEngine* engine) {
#ifdef TEST_QML_IMPORT_PATH
        const QString paths = QStringLiteral(TEST_QML_IMPORT_PATH);
        const auto parts = paths.split(QLatin1Char(':'), Qt::SkipEmptyParts);
        for (const QString& p : parts)
            engine->addImportPath(p);
#endif
        if (m_registry == nullptr) {
            m_registry = new transports::MockTransportRegistry(this);
            m_persons = new transports::MockPersonsService(this);
            m_authFlow = new auth::AuthFlowController(nullptr, this);
        }
        engine->rootContext()->setContextProperty(QStringLiteral("Transports"), m_registry);
        engine->rootContext()->setContextProperty(QStringLiteral("Persons"), m_persons);
        engine->rootContext()->setContextProperty(QStringLiteral("AuthFlow"), m_authFlow);
    }

private:
    transports::MockTransportRegistry* m_registry = nullptr;
    transports::MockPersonsService* m_persons = nullptr;
    auth::AuthFlowController* m_authFlow = nullptr;
};

QUICK_TEST_MAIN_WITH_SETUP(tst_integrations, IntegrationsTestSetup)

#include "tst_integrations_main.moc"
