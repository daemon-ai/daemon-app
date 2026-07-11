// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "auth/auth_flow_controller.h"
#include "daemonnet/mock_fleet_source.h"
#include "fleet/mock_approvals_inbox.h"
#include "persistence/in_memory_session_store.h"
#include "profiles/mock_profile_store.h"
#include "transports/mock_persons_service.h"
#include "transports/mock_transport_registry.h"

#include <QtCore/qstandardpaths.h>
#include <QtCore/qstring.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlextensionplugin.h>
#include <QtQuickTest/quicktest.h>

// Sidebar lives in the STATIC DaemonApp.Sidebar module; its plugin plus the Theme/Controls/
// Presentation/Pages plugins it imports must be referenced explicitly or the linker drops them.
Q_IMPORT_QML_PLUGIN(DaemonApp_ThemePlugin)
Q_IMPORT_QML_PLUGIN(DaemonApp_ControlsPlugin)
Q_IMPORT_QML_PLUGIN(DaemonApp_PresentationPlugin)
Q_IMPORT_QML_PLUGIN(DaemonApp_PagesPlugin)
Q_IMPORT_QML_PLUGIN(DaemonApp_SidebarPlugin)

// The Sidebar binds the app-global context-property seams (injected by Application::registerContext
// in production). The test injects the in-repo mocks: the mock fleet seed backs the session store,
// and the dedicated IntegrationsTree renders from the transport registry + persons seams (the
// legacy transports-tree sidebar section was deleted in M3). The AuthFlowController is serviceless
// so the mounted AuthFlowSheet never binds a redirect sink offscreen.
class SidebarTestSetup : public QObject {
    Q_OBJECT

public:
    SidebarTestSetup() { QStandardPaths::setTestModeEnabled(true); }

public slots:
    void qmlEngineAvailable(QQmlEngine* engine) {
#ifdef TEST_QML_IMPORT_PATH
        const QString paths = QStringLiteral(TEST_QML_IMPORT_PATH);
        const auto parts = paths.split(QLatin1Char(':'), Qt::SkipEmptyParts);
        for (const QString& p : parts)
            engine->addImportPath(p);
#endif
        if (m_store == nullptr) {
            m_fleetSeed = new daemonnet::MockFleetSource(this);
            m_store = new persistence::InMemorySessionStore(m_fleetSeed, this);
            m_profiles = new profiles::MockProfileStore(this);
            m_approvals = new fleet::MockApprovalsInbox(this);
            m_registry = new transports::MockTransportRegistry(this);
            m_persons = new transports::MockPersonsService(this);
            m_authFlow = new auth::AuthFlowController(nullptr, this);
        }
        auto* ctx = engine->rootContext();
        ctx->setContextProperty(QStringLiteral("SessionStore"), m_store);
        ctx->setContextProperty(QStringLiteral("Profiles"), m_profiles);
        ctx->setContextProperty(QStringLiteral("Approvals"), m_approvals);
        ctx->setContextProperty(QStringLiteral("Transports"), m_registry);
        ctx->setContextProperty(QStringLiteral("Persons"), m_persons);
        ctx->setContextProperty(QStringLiteral("AuthFlow"), m_authFlow);
    }

private:
    daemonnet::MockFleetSource* m_fleetSeed = nullptr;
    persistence::InMemorySessionStore* m_store = nullptr;
    profiles::MockProfileStore* m_profiles = nullptr;
    fleet::MockApprovalsInbox* m_approvals = nullptr;
    transports::MockTransportRegistry* m_registry = nullptr;
    transports::MockPersonsService* m_persons = nullptr;
    auth::AuthFlowController* m_authFlow = nullptr;
};

QUICK_TEST_MAIN_WITH_SETUP(tst_sidebar, SidebarTestSetup)

#include "tst_sidebar_main.moc"
