// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "auth/auth_flow_controller.h"
#include "daemon/mirror_session_store.h"
#include "daemon/mock_scenario.h"
#include "fleet/mock_approvals_inbox.h"
#include "mirror/mirror_service.h"
#include "mirror/seeder.h"
#include "profiles/mock_profile_store.h"

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
// in production). The test seeds the default-scenario MIRROR: the session store projects it and
// the dedicated IntegrationsTree renders the same mirror through the `Mirror` context property
// (AD 1a.3 — the registry/persons read seams are gone; the verb-sink `Transports` stays unbound
// offscreen). The AuthFlowController is serviceless so the mounted AuthFlowSheet never binds a
// redirect sink offscreen.
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
            // AD: the scenario-seeded MIRROR store — the same projection production binds.
            m_mirror = new mirror::MirrorService(this);
            m_mirror->openInMemory();
            mirror::Seeder seeder(m_mirror->store());
            seeder.seed(
                daemonapp::daemon::mockScenarioByName(QStringLiteral("default")).mirror.seed);
            m_store = new daemonapp::daemon::MirrorSessionStore(
                &m_mirror->store(), &m_mirror->ingestor(), nullptr, this);
            m_profiles = new profiles::MockProfileStore(this);
            m_approvals = new fleet::MockApprovalsInbox(this);
            m_authFlow = new auth::AuthFlowController(nullptr, this);
        }
        auto* ctx = engine->rootContext();
        ctx->setContextProperty(QStringLiteral("SessionStoreMirror"), m_store);
        ctx->setContextProperty(QStringLiteral("Mirror"), m_mirror);
        ctx->setContextProperty(QStringLiteral("Profiles"), m_profiles);
        ctx->setContextProperty(QStringLiteral("Approvals"), m_approvals);
        ctx->setContextProperty(QStringLiteral("AuthFlow"), m_authFlow);
    }

private:
    mirror::MirrorService* m_mirror = nullptr;
    daemonapp::daemon::MirrorSessionStore* m_store = nullptr;
    profiles::MockProfileStore* m_profiles = nullptr;
    fleet::MockApprovalsInbox* m_approvals = nullptr;
    auth::AuthFlowController* m_authFlow = nullptr;
};

QUICK_TEST_MAIN_WITH_SETUP(tst_sidebar, SidebarTestSetup)

#include "tst_sidebar_main.moc"
