// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "i18n/localization.h"

#include <QCoreApplication>
#include <QString>
#include <QStringList>
#include <QtTest/QtTest>

// Smoke test for the shared runtime translation layer (src/core/i18n). The test
// binary embeds one shipped catalog (German) via qt_add_translations (see
// CMakeLists), so applyLocale("de") can install it and prove that marked strings
// are actually translated at runtime - i.e. the qsTr()/tr() -> .ts -> .qm ->
// QTranslator pipeline works end to end.
class TestLocalization : public QObject {
    Q_OBJECT

private slots:
    void availableLocalesAreOffered();
    void resolveLocaleName();
    void germanCatalogTranslatesMarkedStrings();
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
    // Shipped real catalogs are offered by their locale code.
    QVERIFY(codes.contains(QStringLiteral("de")));
    QVERIFY(codes.contains(QStringLiteral("pt_BR")));
    // A locale we do not ship a catalog for is not offered.
    QVERIFY(!codes.contains(QStringLiteral("ar")));
}

void TestLocalization::resolveLocaleName() {
    // An explicit locale code passes through unchanged (used to load its catalog).
    QCOMPARE(i18n::resolveLocaleName(QStringLiteral("de")), QStringLiteral("de"));
    QCOMPARE(i18n::resolveLocaleName(QStringLiteral("fr")), QStringLiteral("fr"));
    // "system" resolves to the OS locale name, which is always non-empty.
    QVERIFY(!i18n::resolveLocaleName(QStringLiteral("system")).isEmpty());
}

void TestLocalization::germanCatalogTranslatesMarkedStrings() {
    i18n::applyLocale(QStringLiteral("de"));

    // A C++ (QObject::tr) string and a QML (qsTr) string, in their respective
    // contexts, must both come back translated (non-empty and != the source
    // English) once the German catalog is installed.
    const QString you = QCoreApplication::translate("QObject", "YOU");
    QVERIFY2(!you.isEmpty(), "translated C++ string should be non-empty");
    QVERIFY2(you != QStringLiteral("YOU"), "marked C++ string should be translated");

    const QString theme = QCoreApplication::translate("AppearanceSection", "Theme");
    QVERIFY2(!theme.isEmpty(), "translated QML string should be non-empty");
    QVERIFY2(theme != QStringLiteral("Theme"), "marked QML string should be translated");

    // Shared C++ view-model strings (a default tab title and a production error
    // message) must translate in their own contexts too, proving the GUI + TUI
    // shared layer flows through the same catalog as the QML surfaces.
    const QString tabTitle = QCoreApplication::translate("TabModel", "Session");
    QVERIFY2(!tabTitle.isEmpty(), "shared model string should be non-empty");
    QVERIFY2(tabTitle != QStringLiteral("Session"), "shared model string should be translated");

    const QString reqError = QCoreApplication::translate("NodeApiClient", "request timed out");
    QVERIFY2(reqError != QStringLiteral("request timed out"),
             "user-facing error string should be translated");

    // Restore the source language so later tests/processes are unaffected.
    i18n::applyLocale(QStringLiteral("en"));
}

void TestLocalization::sourceLanguageRemovesTranslations() {
    i18n::applyLocale(QStringLiteral("de"));
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
