#include "daemon/daemon_profile_store.h"
#include "daemon/daemon_transport.h"
#include "daemon/node_api_client.h"
#include "daemon/repositories.h"
#include "uimodels/variant_list_model.h"

#include <QtTest/QtTest>

using daemonapp::daemon::NodeApiClient;
using daemonapp::daemon::ProfileRepository;
using profiles::DaemonProfileStore;

// Unit coverage for the client-side logic in DaemonProfileStore that is not exercised by the wire
// gates: the id-derivation (slug) for create/clone. The encode byte-correctness is covered by the
// daemon-node verify-codec fixtures, and create/update/clone/get against a real daemon by the
// system-tests; here we only need an (unconnected) repository so the synchronous return value is
// observable.
class TestDaemonProfileStore : public QObject {
    Q_OBJECT

private slots:
    void createSlugsTheNameIntoAnId() {
        daemonapp::daemon::DaemonTransport transport;
        NodeApiClient client(&transport);
        ProfileRepository repo(&client, nullptr);
        DaemonProfileStore store(&repo);

        QCOMPARE(store.createProfile(QStringLiteral("My Work Agent")),
                 QStringLiteral("my-work-agent"));
        QCOMPARE(store.createProfile(QStringLiteral("Coder/Reviewer 2")),
                 QStringLiteral("coder-reviewer-2"));
        // Empty / punctuation-only names fall back to a stable default rather than an empty id.
        QCOMPARE(store.createProfile(QString()), QStringLiteral("profile"));
        QCOMPARE(store.createProfile(QStringLiteral("  --  ")), QStringLiteral("profile"));
    }

    void cloneSlugsTheNewId() {
        daemonapp::daemon::DaemonTransport transport;
        NodeApiClient client(&transport);
        ProfileRepository repo(&client, nullptr);
        DaemonProfileStore store(&repo);

        QCOMPARE(store.cloneProfile(QStringLiteral("default"), QStringLiteral("Work Copy")),
                 QStringLiteral("work-copy"));
        // An empty target id is rejected (no clone issued).
        QVERIFY(store.cloneProfile(QStringLiteral("default"), QString()).isEmpty());
    }

    void exposesAProfilesModel() {
        daemonapp::daemon::DaemonTransport transport;
        NodeApiClient client(&transport);
        ProfileRepository repo(&client, nullptr);
        DaemonProfileStore store(&repo);

        auto* model = qobject_cast<uimodels::VariantListModel*>(store.profiles());
        QVERIFY(model != nullptr);
        QCOMPARE(model->count(), 0); // empty until a ProfileList round-trips
    }
};

QTEST_MAIN(TestDaemonProfileStore)
#include "tst_daemon_profile_store.moc"
