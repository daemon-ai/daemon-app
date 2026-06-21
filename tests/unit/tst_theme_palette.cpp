#include <QtTest>

#include "theme/theme_palette.h"

using theme::ThemeName;
using theme::ThemePalette;
using theme::Token;

// Locks the shared theme token table - the single source of truth backing both
// Theme.qml (via the ThemeTokens singleton) and the TUI's tui_palette. The
// canonical per-theme hexes mirror tests/qml/theme/tst_theme.qml, so this proves
// the C++ table matches the original QML values the GUI shipped with.
class ThemePaletteTests : public QObject {
    Q_OBJECT

private slots:
    void namesRoundTrip();
    void knownThemes();
    void canonicalHexes_data();
    void canonicalHexes();
    void tokensDifferBetweenThemes();
    void unknownTokenIsInvalid();
};

void ThemePaletteTests::namesRoundTrip()
{
    QCOMPARE(ThemePalette::toString(ThemeName::Light), QStringLiteral("Light"));
    QCOMPARE(ThemePalette::toString(ThemeName::Dark), QStringLiteral("Dark"));
    QCOMPARE(ThemePalette::toString(ThemeName::Sepia), QStringLiteral("Sepia"));
    QCOMPARE(ThemePalette::toString(ThemeName::Midnight), QStringLiteral("Midnight"));

    QCOMPARE(ThemePalette::fromString(QStringLiteral("Dark")), ThemeName::Dark);
    QCOMPARE(ThemePalette::fromString(QStringLiteral("Sepia")), ThemeName::Sepia);
    QCOMPARE(ThemePalette::fromString(QStringLiteral("Midnight")), ThemeName::Midnight);
    // Unknown names fall back to Light (matches the GUI/UiSettings default).
    QCOMPARE(ThemePalette::fromString(QStringLiteral("Nonsense")), ThemeName::Light);

    const QStringList names = ThemePalette::allNames();
    QCOMPARE(names.size(), 4);
    QVERIFY(names.contains(QStringLiteral("Light")));
    QVERIFY(names.contains(QStringLiteral("Midnight")));
}

void ThemePaletteTests::knownThemes()
{
    QVERIFY(ThemePalette::isKnown(QStringLiteral("Light")));
    QVERIFY(ThemePalette::isKnown(QStringLiteral("Dark")));
    QVERIFY(ThemePalette::isKnown(QStringLiteral("Sepia")));
    QVERIFY(ThemePalette::isKnown(QStringLiteral("Midnight")));
    QVERIFY(!ThemePalette::isKnown(QStringLiteral("light")));
    QVERIFY(!ThemePalette::isKnown(QString()));
}

void ThemePaletteTests::canonicalHexes_data()
{
    QTest::addColumn<int>("theme");
    QTest::addColumn<QString>("background");
    QTest::addColumn<QString>("sidebar");
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("accent");
    QTest::addColumn<QString>("rowActive");
    QTest::addColumn<QString>("searchBackground");
    QTest::addColumn<QString>("codeBackground");
    QTest::addColumn<QString>("statusOk");
    QTest::addColumn<QString>("danger");

    QTest::newRow("Light") << int(ThemeName::Light) << "#ffffff" << "#ededed" << "#37352e"
                           << "#2383e2" << "#dcdcdc" << "#ffffff" << "#f1f1ef" << "#2f9e44"
                           << "#c0392b";
    QTest::newRow("Dark") << int(ThemeName::Dark) << "#191919" << "#333333" << "#d6d6d6"
                          << "#757575" << "#474747" << "#2a2a2a" << "#2a2a2a" << "#5fbf73"
                          << "#f06a6a";
    QTest::newRow("Sepia") << int(ThemeName::Sepia) << "#fbf0d9" << "#ece0c2" << "#321e03"
                           << "#b06a2c" << "#d8c79a" << "#fbf0d9" << "#efe6d2" << "#2f9e44"
                           << "#c0392b";
    QTest::newRow("Midnight") << int(ThemeName::Midnight) << "#0d162d" << "#09286f" << "#ffe6cb"
                              << "#9fb3e6" << "#1a4499" << "#0b2150" << "#1a2c5c" << "#5fbf73"
                              << "#f06a6a";
}

void ThemePaletteTests::canonicalHexes()
{
    QFETCH(int, theme);
    const auto t = static_cast<ThemeName>(theme);

    const auto hex = [&](Token tok) { return ThemePalette::color(t, tok).name(); };

    QFETCH(QString, background);
    QCOMPARE(hex(Token::Background), background);
    QFETCH(QString, sidebar);
    QCOMPARE(hex(Token::Sidebar), sidebar);
    QFETCH(QString, text);
    QCOMPARE(hex(Token::Text), text);
    QFETCH(QString, accent);
    QCOMPARE(hex(Token::Accent), accent);
    QFETCH(QString, rowActive);
    QCOMPARE(hex(Token::RowActive), rowActive);
    QFETCH(QString, searchBackground);
    QCOMPARE(hex(Token::SearchBackground), searchBackground);
    QFETCH(QString, codeBackground);
    QCOMPARE(hex(Token::CodeBackground), codeBackground);
    QFETCH(QString, statusOk);
    QCOMPARE(hex(Token::StatusOk), statusOk);
    QFETCH(QString, danger);
    QCOMPARE(hex(Token::Danger), danger);

    // The string-keyed (QML) overload resolves to the same color.
    QCOMPARE(ThemePalette::color(ThemePalette::toString(t), QStringLiteral("background")).name(),
             background);
}

void ThemePaletteTests::tokensDifferBetweenThemes()
{
    // Switching themes must actually change tokens (guards a constant table).
    const QString light = ThemePalette::color(ThemeName::Light, Token::Background).name();
    const QString dark = ThemePalette::color(ThemeName::Dark, Token::Background).name();
    const QString sepia = ThemePalette::color(ThemeName::Sepia, Token::Background).name();
    const QString midnight = ThemePalette::color(ThemeName::Midnight, Token::Background).name();
    QVERIFY(light != dark);
    QVERIFY(dark != sepia);
    QVERIFY(sepia != midnight);
    QVERIFY(light != midnight);
}

void ThemePaletteTests::unknownTokenIsInvalid()
{
    QVERIFY(!ThemePalette::hasToken(QStringLiteral("nope")));
    QVERIFY(!ThemePalette::color(QStringLiteral("Dark"), QStringLiteral("nope")).isValid());
    QVERIFY(ThemePalette::hasToken(QStringLiteral("background")));
}

QTEST_MAIN(ThemePaletteTests)
#include "tst_theme_palette.moc"
