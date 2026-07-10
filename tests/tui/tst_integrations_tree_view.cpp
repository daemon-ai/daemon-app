// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "display_role_adapter.h"
#include "integrations_tree_model.h"
#include "transports/ipersons_service.h"
#include "transports/itransport_registry.h"

#include <QtTest>
#include <Tui/ZCommon.h>

using transports::IPersonsService;
using transports::ITransportRegistry;

namespace {

QVariantMap conv(const QString& transport, const QString& id, const QString& title,
                 const QString& kind, const QString& parent) {
    return {{QStringLiteral("transport"), transport},
            {QStringLiteral("id"), id},
            {QStringLiteral("title"), title},
            {QStringLiteral("kind"), kind},
            {QStringLiteral("parent"), parent}};
}

class FakeRegistry : public ITransportRegistry {
public:
    using ITransportRegistry::ITransportRegistry;
    [[nodiscard]] QVariantList availableAdapters() const override {
        QVariantMap caps{{QStringLiteral("rooms"), true},
                         {QStringLiteral("directMessages"), true},
                         {QStringLiteral("presence"), true}};
        QVariantMap m{{QStringLiteral("family"), QStringLiteral("matrix")},
                      {QStringLiteral("displayName"), QStringLiteral("Matrix")},
                      {QStringLiteral("capabilities"), caps},
                      {QStringLiteral("directory"), false},
                      {QStringLiteral("schema"), QVariantList{}}};
        return {m};
    }
    [[nodiscard]] QVariantList instances() const override {
        QVariantMap m{{QStringLiteral("transport"), QStringLiteral("matrix/@you")},
                      {QStringLiteral("family"), QStringLiteral("matrix")},
                      {QStringLiteral("displayName"), QStringLiteral("Matrix (@you)")},
                      {QStringLiteral("enabled"), true},
                      {QStringLiteral("connectionReason"), QStringLiteral("ready")}};
        return {m};
    }
    [[nodiscard]] QVariantList conversations(const QString& transport) const override {
        if (transport != QStringLiteral("matrix/@you")) {
            return {};
        }
        return {conv(transport, QStringLiteral("!space1"), QStringLiteral("Demo Server"),
                     QStringLiteral("space"), QString()),
                conv(transport, QStringLiteral("!general"), QStringLiteral("general"),
                     QStringLiteral("channel"), QStringLiteral("!space1")),
                conv(transport, QStringLiteral("!dm"), QStringLiteral("Alice"),
                     QStringLiteral("dm"), QString())};
    }
};

class FakePersons : public IPersonsService {
public:
    using IPersonsService::IPersonsService;
    [[nodiscard]] QVariantList persons() const override { return {}; }
};

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
        FakeRegistry reg;
        FakePersons persons;
        IntegrationsTreeModel model;
        model.setRegistry(&reg);
        model.setPersons(&persons);

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
        FakeRegistry reg;
        FakePersons persons;
        IntegrationsTreeModel model;
        model.setRegistry(&reg);
        model.setPersons(&persons);
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
