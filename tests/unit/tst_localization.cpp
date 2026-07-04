// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "i18n/localization.h"

#include <QCoreApplication>
#include <QString>
#include <QStringList>
#include <QtTest/QtTest>

// Smoke test for the shared runtime translation layer (src/core/i18n). The test
// binary embeds the committed pseudolocale catalog via qt_add_translations (see
// CMakeLists), so applyLocale("pseudo") can install it and prove that marked
// strings are actually translated at runtime - i.e. the qsTr()/tr() -> .ts ->
// .qm -> QTranslator pipeline works end to end.
class TestLocalization : public QObject {
    Q_OBJECT

private slots:
    void availableLocalesAreOffered();
    void resolveLocaleName();
    void pseudolocaleTranslatesMarkedStrings();
    void sourceLanguageRemovesTranslations();
    void rtlLocaleIsRightToLeft();
};

void TestLocalization::availableLocalesAreOffered() {
    QStringList codes;
    for (const i18n::LocaleOption& opt : i18n::availableLocales()) {
        codes << opt.code;
        QVERIFY2(!opt.label.isEmpty(), "every offered locale needs a menu label");
    }
    QVERIFY(codes.contains(QStringLiteral("system")));
    QVERIFY(codes.contains(QStringLiteral("en")));
    QVERIFY(codes.contains(QStringLiteral("pseudo")));
    // No real translations ship yet, so no concrete language locale is offered.
    QVERIFY(!codes.contains(QStringLiteral("ar")));
}

void TestLocalization::resolveLocaleName() {
    // An explicit locale code passes through unchanged (used to load its catalog).
    QCOMPARE(i18n::resolveLocaleName(QStringLiteral("pseudo")), QStringLiteral("pseudo"));
    QCOMPARE(i18n::resolveLocaleName(QStringLiteral("fr")), QStringLiteral("fr"));
    // "system" resolves to the OS locale name, which is always non-empty.
    QVERIFY(!i18n::resolveLocaleName(QStringLiteral("system")).isEmpty());
}

void TestLocalization::pseudolocaleTranslatesMarkedStrings() {
    i18n::applyLocale(QStringLiteral("pseudo"));

    // A C++ (QObject::tr) string and a QML (qsTr) string, in their respective
    // contexts, must both come back pseudolocalized (wrapped in guillemets and
    // != the source) once the pseudo catalog is installed.
    const QString you = QCoreApplication::translate("QObject", "YOU");
    QVERIFY2(you != QStringLiteral("YOU"), "marked C++ string should be translated");
    QVERIFY(you.startsWith(QChar(0x00ab))); // «

    const QString theme = QCoreApplication::translate("AppearanceSection", "Theme");
    QVERIFY2(theme != QStringLiteral("Theme"), "marked QML string should be translated");

    // Shared C++ view-model strings (a default tab title and a production error
    // message) must translate in their own contexts too, proving the GUI + TUI
    // shared layer flows through the same catalog as the QML surfaces.
    const QString tabTitle = QCoreApplication::translate("TabModel", "Session");
    QVERIFY2(tabTitle != QStringLiteral("Session"), "shared model string should be translated");

    const QString reqError = QCoreApplication::translate("NodeApiClient", "request timed out");
    QVERIFY2(reqError != QStringLiteral("request timed out"),
             "user-facing error string should be translated");

    // Restore the source language so later tests/processes are unaffected.
    i18n::applyLocale(QStringLiteral("en"));
}

void TestLocalization::sourceLanguageRemovesTranslations() {
    i18n::applyLocale(QStringLiteral("pseudo"));
    QVERIFY(QCoreApplication::translate("QObject", "YOU") != QStringLiteral("YOU"));

    // Switching back to the source language must uninstall the catalog so the
    // in-source English text shows again (this is what live-switching relies on).
    i18n::applyLocale(QStringLiteral("en"));
    QCOMPARE(QCoreApplication::translate("QObject", "YOU"), QStringLiteral("YOU"));
}

void TestLocalization::rtlLocaleIsRightToLeft() {
    // Layout direction is derived from the locale itself (not hardcoded), so a
    // future RTL language mirrors the UI automatically. No Arabic catalog ships,
    // but applying an RTL locale code must still report right-to-left.
    QCOMPARE(i18n::applyLocale(QStringLiteral("ar")), Qt::RightToLeft);
    QCOMPARE(i18n::applyLocale(QStringLiteral("en")), Qt::LeftToRight);
    i18n::applyLocale(QStringLiteral("en"));
}

QTEST_GUILESS_MAIN(TestLocalization)
#include "tst_localization.moc"
