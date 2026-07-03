// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// Off-screen tests for the TUI add-account wizard (dialogs/add_account_flow):
// they drive a real AddAccountFlow bound to the MockAccountsService inside an
// off-screen Tui terminal by sending keys, then assert the painted frames and
// the resulting account rows - the TUI analog of the GUI AddAccountWizard
// (provider pick -> auth method -> masked key entry / OAuth kick).

#include "accounts/mock_accounts_service.h"
#include "cache_test_support.h"
#include "dialogs/add_account_flow.h"
#include "uimodels/variant_list_model.h"

#include <QEventLoop>
#include <QSignalSpy>
#include <QTimer>
#include <QtTest>
#include <Tui/ZImage.h>
#include <Tui/ZTerminal.h>
#include <Tui/ZTest.h>
#include <Tui/ZWidget.h>

using accounts::MockAccountsService;
using uimodels::VariantListModel;

namespace {

// Flatten the off-screen terminal frame to one string per row (cluster-aware,
// like the daemon-tui offscreen dump) so we can assert on the visible glyphs.
QString frameText(Tui::ZTerminal& terminal) {
    const Tui::ZImage img = terminal.grabCurrentImage();
    QString out;
    for (int y = 0; y < img.height(); ++y) {
        for (int x = 0; x < img.width(); ++x) {
            int left = 0;
            int right = 0;
            const QString cell = img.peekText(x, y, &left, &right);
            out += (left == x && !cell.isEmpty()) ? cell : QStringLiteral(" ");
        }
        out += QLatin1Char('\n');
    }
    return out;
}

void settle() {
    QEventLoop loop;
    QTimer::singleShot(20, &loop, &QEventLoop::quit);
    loop.exec();
}

VariantListModel* rowsOf(MockAccountsService& service) {
    return qobject_cast<VariantListModel*>(service.accounts());
}

} // namespace

class TestAccountsWizard : public QObject {
    Q_OBJECT

private slots:
    void init() { resetMockCache(); }

    // The straight API-key path: pick a single-kind provider, type the key into
    // the masked prompt (never echoed), and the connected account row appears
    // with the provider-name label (empty optional label, like the GUI).
    void apiKeyFlowAddsAccount() {
        Tui::ZTerminal terminal{Tui::ZTerminal::OffScreen{100, 30}};
        Tui::ZWidget root;
        terminal.setMainWidget(&root);

        MockAccountsService service;
        const int before = rowsOf(service)->count();
        auto* flow = new AddAccountFlow(&service, &root);
        QSignalSpy changed(flow, &AddAccountFlow::changed);
        QSignalSpy finished(flow, &AddAccountFlow::finished);

        flow->open();
        settle();
        QString frame = frameText(terminal);
        QVERIFY2(frame.contains(QStringLiteral("pick a provider")), qPrintable(frame));
        QVERIFY2(frame.contains(QStringLiteral("OpenRouter")), qPrintable(frame));

        // Filter down to OpenRouter (API-key only) and pick it.
        Tui::ZTest::sendText(&terminal, QStringLiteral("openrouter"), Qt::NoModifier);
        Tui::ZTest::sendKey(&terminal, Qt::Key_Enter, Qt::NoModifier);
        settle();
        frame = frameText(terminal);
        QVERIFY2(frame.contains(QStringLiteral("OpenRouter — API key")), qPrintable(frame));

        // The key is typed into a masked field: it must never be painted.
        Tui::ZTest::sendText(&terminal, QStringLiteral("secret-key-abcd"), Qt::NoModifier);
        settle();
        frame = frameText(terminal);
        QVERIFY2(!frame.contains(QStringLiteral("secret-key-abcd")), qPrintable(frame));

        Tui::ZTest::sendKey(&terminal, Qt::Key_Enter, Qt::NoModifier);
        QCOMPARE(rowsOf(service)->count(), before + 1);
        const QVariantMap row = rowsOf(service)->rows().last();
        QCOMPARE(row.value(QStringLiteral("provider")).toString(), QStringLiteral("openrouter"));
        QCOMPARE(row.value(QStringLiteral("kind")).toString(), QStringLiteral("apikey"));
        QCOMPARE(row.value(QStringLiteral("status")).toString(), QStringLiteral("connected"));
        QCOMPARE(row.value(QStringLiteral("label")).toString(), QStringLiteral("OpenRouter"));
        QVERIFY(row.value(QStringLiteral("detail")).toString().contains(QStringLiteral("abcd")));
        QCOMPARE(changed.count(), 1);
        QCOMPARE(finished.count(), 1);
    }

    // A multi-kind provider inserts the auth-method step (the GUI dropdown);
    // choosing OAuth kicks the simulated handshake: busy + a pending row, then
    // connected when oauthCompleted lands.
    void oauthKickFromMethodPick() {
        Tui::ZTerminal terminal{Tui::ZTerminal::OffScreen{100, 30}};
        Tui::ZWidget root;
        terminal.setMainWidget(&root);

        MockAccountsService service;
        const int before = rowsOf(service)->count();
        auto* flow = new AddAccountFlow(&service, &root);
        QSignalSpy changed(flow, &AddAccountFlow::changed);
        QSignalSpy done(&service, &accounts::IAccountsService::oauthCompleted);

        flow->open();
        Tui::ZTest::sendText(&terminal, QStringLiteral("anthropic"), Qt::NoModifier);
        Tui::ZTest::sendKey(&terminal, Qt::Key_Enter, Qt::NoModifier);
        settle();
        const QString frame = frameText(terminal);
        QVERIFY2(frame.contains(QStringLiteral("Authentication")), qPrintable(frame));
        QVERIFY2(frame.contains(QStringLiteral("Sign in (OAuth)")), qPrintable(frame));
        QVERIFY2(frame.contains(QStringLiteral("API key")), qPrintable(frame));

        Tui::ZTest::sendText(&terminal, QStringLiteral("sign"), Qt::NoModifier);
        Tui::ZTest::sendKey(&terminal, Qt::Key_Enter, Qt::NoModifier);

        // The handshake is in flight: busy, and a pending row is visible (the
        // page's live refresh renders it, like the GUI "Connecting…" badge).
        QVERIFY(service.busy());
        QCOMPARE(rowsOf(service)->count(), before + 1);
        QCOMPARE(rowsOf(service)->rows().last().value(QStringLiteral("status")).toString(),
                 QStringLiteral("pending"));
        QCOMPARE(changed.count(), 1);

        QVERIFY(QTest::qWaitFor([&] { return done.count() == 1; }, 4000));
        QVERIFY(!service.busy());
        QCOMPARE(rowsOf(service)->rows().last().value(QStringLiteral("status")).toString(),
                 QStringLiteral("connected"));
    }

    // needsBaseUrl providers get the extra base-URL prompt after the key (the
    // GUI's conditional Base URL field).
    void baseUrlPromptedWhenProviderNeedsIt() {
        Tui::ZTerminal terminal{Tui::ZTerminal::OffScreen{100, 30}};
        Tui::ZWidget root;
        terminal.setMainWidget(&root);

        MockAccountsService service;
        const int before = rowsOf(service)->count();
        auto* flow = new AddAccountFlow(&service, &root);

        flow->open();
        Tui::ZTest::sendText(&terminal, QStringLiteral("azure"), Qt::NoModifier);
        Tui::ZTest::sendKey(&terminal, Qt::Key_Enter, Qt::NoModifier);
        settle();
        QString frame = frameText(terminal);
        QVERIFY2(frame.contains(QStringLiteral("Azure OpenAI — API key")), qPrintable(frame));

        Tui::ZTest::sendText(&terminal, QStringLiteral("azure-key-9999"), Qt::NoModifier);
        Tui::ZTest::sendKey(&terminal, Qt::Key_Enter, Qt::NoModifier);
        settle();
        frame = frameText(terminal);
        QVERIFY2(frame.contains(QStringLiteral("Base URL")), qPrintable(frame));

        Tui::ZTest::sendText(&terminal, QStringLiteral("https://example.openai.azure.com"),
                             Qt::NoModifier);
        Tui::ZTest::sendKey(&terminal, Qt::Key_Enter, Qt::NoModifier);
        QCOMPARE(rowsOf(service)->count(), before + 1);
        QCOMPARE(rowsOf(service)->rows().last().value(QStringLiteral("provider")).toString(),
                 QStringLiteral("azure"));
    }

    // Esc aborts at either step without touching the seam (the GUI wizard's
    // Cancel / CloseOnEscape).
    void escAbortsWithoutMutation() {
        Tui::ZTerminal terminal{Tui::ZTerminal::OffScreen{100, 30}};
        Tui::ZWidget root;
        terminal.setMainWidget(&root);

        MockAccountsService service;
        const int before = rowsOf(service)->count();
        auto* flow = new AddAccountFlow(&service, &root);
        QSignalSpy changed(flow, &AddAccountFlow::changed);
        QSignalSpy finished(flow, &AddAccountFlow::finished);

        // Esc on the provider palette.
        flow->open();
        Tui::ZTest::sendKey(&terminal, Qt::Key_Escape, Qt::NoModifier);
        QCOMPARE(finished.count(), 1);

        // Esc on the masked key prompt.
        flow->open();
        Tui::ZTest::sendText(&terminal, QStringLiteral("openrouter"), Qt::NoModifier);
        Tui::ZTest::sendKey(&terminal, Qt::Key_Enter, Qt::NoModifier);
        settle();
        Tui::ZTest::sendKey(&terminal, Qt::Key_Escape, Qt::NoModifier);
        QCOMPARE(finished.count(), 2);

        QCOMPARE(changed.count(), 0);
        QCOMPARE(rowsOf(service)->count(), before);
    }
};

QTEST_MAIN(TestAccountsWizard)
#include "tst_accounts_wizard.moc"
