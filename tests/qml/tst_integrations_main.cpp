// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "auth/auth_flow_controller.h"
#include "mirror/mirror_service.h"
#include "mirror/seeder.h"

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

// AD (1a.3): the tree binds the app-global `Mirror` composition root (its READ source; the
// registry seam is a daemon-only VERB sink the offscreen test leaves unbound). The fixture seeds
// a real in-memory mirror through the Seeder — the SAME apply pipeline production feeds — so the
// REAL IntegrationsTree.qml renders the projection exactly as both shipped modes do. The
// AuthFlowController is constructed WITHOUT a service so start() fails closed instead of binding
// a loopback redirect sink (offscreen runs must never bind ports).
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
        if (m_mirror == nullptr) {
            m_mirror = new mirror::MirrorService(this);
            m_mirror->openInMemory();
            mirror::SeedSet seed;
            mirror::Adapter matrix;
            matrix.family = QStringLiteral("matrix");
            matrix.display_name = QStringLiteral("Matrix");
            matrix.cap_rooms = true;
            matrix.cap_direct_messages = true;
            seed.adapters = {matrix};
            mirror::TransportAccount acct;
            acct.transport = QStringLiteral("matrix/@you:matrix.org");
            acct.family = QStringLiteral("matrix");
            acct.display_name = QStringLiteral("@you:matrix.org");
            acct.enabled = true;
            acct.connection = QStringLiteral("connected");
            seed.transportAccounts = {acct};
            mirror::Conversation dev;
            dev.transport = acct.transport;
            dev.id = QStringLiteral("!daemon-dev:matrix.org");
            dev.title = QStringLiteral("daemon-dev");
            dev.kind = QStringLiteral("channel");
            mirror::Conversation dm;
            dm.transport = acct.transport;
            dm.id = QStringLiteral("!dmAlice");
            dm.title = QStringLiteral("Alice");
            dm.kind = QStringLiteral("dm");
            seed.conversations = {dev, dm};
            mirror::Seeder seeder(m_mirror->store());
            seeder.seed(seed);
            m_authFlow = new auth::AuthFlowController(nullptr, this);
        }
        engine->rootContext()->setContextProperty(QStringLiteral("Mirror"), m_mirror);
        engine->rootContext()->setContextProperty(QStringLiteral("AuthFlow"), m_authFlow);
    }

private:
    mirror::MirrorService* m_mirror = nullptr;
    auth::AuthFlowController* m_authFlow = nullptr;
};

QUICK_TEST_MAIN_WITH_SETUP(tst_integrations, IntegrationsTestSetup)

#include "tst_integrations_main.moc"
