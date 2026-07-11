// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "display_role_adapter.h"
#include "integrations_tree_model.h"
#include "mirror/mirror_service.h"
#include "mirror/seeder.h"

#include <QtTest>
#include <Tui/ZCommon.h>

namespace {

// A minimal seeded MIRROR: one Matrix account, a space nesting a room, and a DM — the same
// apply pipeline production feeds (§9; the legacy registry/persons fakes died with the read
// port).
void seedTree(mirror::MirrorService& svc) {
    mirror::SeedSet seed;
    mirror::Adapter matrix;
    matrix.family = QStringLiteral("matrix");
    matrix.display_name = QStringLiteral("Matrix");
    matrix.cap_rooms = true;
    matrix.cap_direct_messages = true;
    seed.adapters = {matrix};
    mirror::TransportAccount acct;
    acct.transport = QStringLiteral("matrix/@you");
    acct.family = QStringLiteral("matrix");
    acct.display_name = QStringLiteral("Matrix (@you)");
    acct.enabled = true;
    acct.reason = QStringLiteral("ready");
    seed.transportAccounts = {acct};
    mirror::Conversation space;
    space.transport = acct.transport;
    space.id = QStringLiteral("!space1");
    space.title = QStringLiteral("Demo Server");
    space.kind = QStringLiteral("space");
    mirror::Conversation general;
    general.transport = acct.transport;
    general.id = QStringLiteral("!general");
    general.title = QStringLiteral("general");
    general.kind = QStringLiteral("channel");
    general.parent = QStringLiteral("!space1");
    mirror::Conversation dm;
    dm.transport = acct.transport;
    dm.id = QStringLiteral("!dm");
    dm.title = QStringLiteral("Alice");
    dm.kind = QStringLiteral("dm");
    seed.conversations = {space, general, dm};
    mirror::Seeder seeder(svc.store());
    seeder.seed(seed);
}

} // namespace

// The TUI half of the GUI/TUI parity contract: the SAME IntegrationsTreeModel (the shared C++
// view-model) renders through the DisplayRoleAdapter's Integrations kind, mapping the model's
// custom roles onto Qt::DisplayRole + Tui list decorations exactly as the GUI ListView delegate
// reads them.
class TestIntegrationsTreeView : public QObject {
    Q_OBJECT

    static QString displayFor(const DisplayRoleAdapter& a, const QString& contains) {
        for (int i = 0; i < a.rowCount(); ++i) {
            const QString t = a.data(a.index(i, 0), Qt::DisplayRole).toString();
            if (t.contains(contains)) {
                return t;
            }
        }
        return {};
    }

private slots:
    // The account root renders its label with a disclosure marker and a connection decoration dot;
    // a nested room is indented under its space; a section header renders as a foldable divider.
    void rendersSharedModelInTui() {
        mirror::MirrorService svc;
        svc.openInMemory();
        seedTree(svc);
        IntegrationsTreeModel model;
        model.setMirror(&svc);

        DisplayRoleAdapter adapter(DisplayRoleAdapter::Kind::Integrations);
        adapter.setSourceModel(&model);
        QCOMPARE(adapter.rowCount(), model.rowCount());

        // The account root: disclosure marker + label.
        const QString account = displayFor(adapter, QStringLiteral("Matrix (@you)"));
        QVERIFY(!account.isEmpty());
        QVERIFY(account.contains(QStringLiteral("\u25be")) ||
                account.contains(QStringLiteral("\u25b8")));

        // The space nests its child room (indent grows with depth).
        const QString space = displayFor(adapter, QStringLiteral("Demo Server"));
        const QString general = displayFor(adapter, QStringLiteral("general"));
        QVERIFY(!space.isEmpty() && !general.isEmpty());
        const int spaceIndent =
            static_cast<int>(space.size() - QStringView(space).trimmed().size());
        const int generalIndent =
            static_cast<int>(general.size() - QStringView(general).trimmed().size());
        QVERIFY2(generalIndent > spaceIndent, "a room is indented deeper than its space");

        // The Direct Messages section renders as a divider header.
        const QString dms = displayFor(adapter, QStringLiteral("Direct Messages"));
        QVERIFY(dms.contains(QStringLiteral("\u2500"))); // box-drawing divider
    }

    // The account row carries a connection decoration glyph (proving the model's connection role
    // maps onto a Tui LeftDecoration, the TUI analogue of the GUI presence dot).
    void accountCarriesConnectionDecoration() {
        mirror::MirrorService svc;
        svc.openInMemory();
        seedTree(svc);
        IntegrationsTreeModel model;
        model.setMirror(&svc);
        DisplayRoleAdapter adapter(DisplayRoleAdapter::Kind::Integrations);
        adapter.setSourceModel(&model);

        int accountRow = -1;
        for (int i = 0; i < adapter.rowCount(); ++i) {
            if (adapter.data(adapter.index(i, 0), Qt::DisplayRole)
                    .toString()
                    .contains(QStringLiteral("Matrix (@you)"))) {
                accountRow = i;
                break;
            }
        }
        QVERIFY(accountRow >= 0);
        const QVariant deco = adapter.data(adapter.index(accountRow, 0), Tui::LeftDecorationRole);
        QCOMPARE(deco.toString(), QStringLiteral("\u25cf"));
    }
};

QTEST_MAIN(TestIntegrationsTreeView)
#include "tst_integrations_tree_view.moc"
