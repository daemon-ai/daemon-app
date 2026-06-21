#include <QtTest>

#include "tui_palette.h"

#include <Tui/ZColor.h>

using theme::ThemeName;

namespace {

// Compare a ZColor to an expected "#rrggbb" hex (the TUI renders truecolor RGB).
void compareHex(const Tui::ZColor& c, const char* hex)
{
    const QColor expected = QColor::fromString(QLatin1String(hex));
    QCOMPARE(c.red(), expected.red());
    QCOMPARE(c.green(), expected.green());
    QCOMPARE(c.blue(), expected.blue());
}

} // namespace

// Proves the TUI palette is theme-aware: each tpal accessor resolves the active
// theme's shared token to a ZColor, and changing the active theme changes the
// resolved colors. Color correctness elsewhere is locked by tst_theme_palette;
// here we verify the wiring (active-theme switch -> ZColor) the TUI relies on.
class TuiPaletteTests : public QObject {
    Q_OBJECT

private slots:
    void cleanup();
    void resolvesActiveThemeTokens();
    void switchingThemeChangesColors();
    void daemonPaletteBuildsForEveryTheme();
};

void TuiPaletteTests::cleanup()
{
    // Leave the module in its default state for the next test / the app.
    tpal::setActiveTheme(ThemeName::Dark);
}

void TuiPaletteTests::resolvesActiveThemeTokens()
{
    tpal::setActiveTheme(ThemeName::Light);
    QCOMPARE(tpal::activeTheme(), ThemeName::Light);
    compareHex(tpal::fg(), "#37352e");
    compareHex(tpal::bg(), "#ffffff");
    compareHex(tpal::accent(), "#2383e2");
    compareHex(tpal::codeBg(), "#f1f1ef");
    compareHex(tpal::statusError(), "#c0392b");

    tpal::setActiveTheme(ThemeName::Midnight);
    QCOMPARE(tpal::activeTheme(), ThemeName::Midnight);
    compareHex(tpal::fg(), "#ffe6cb");
    compareHex(tpal::bg(), "#0d162d");
    compareHex(tpal::accent(), "#9fb3e6");
}

void TuiPaletteTests::switchingThemeChangesColors()
{
    tpal::setActiveTheme(ThemeName::Light);
    const Tui::ZColor lightBg = tpal::bg();
    tpal::setActiveTheme(ThemeName::Dark);
    const Tui::ZColor darkBg = tpal::bg();
    tpal::setActiveTheme(ThemeName::Sepia);
    const Tui::ZColor sepiaBg = tpal::bg();

    QVERIFY(lightBg != darkBg);
    QVERIFY(darkBg != sepiaBg);
    QVERIFY(lightBg != sepiaBg);
}

void TuiPaletteTests::daemonPaletteBuildsForEveryTheme()
{
    // Building the stock-widget palette for every theme must not crash (the quit
    // dialog and lists inherit it). We can't assert role colors without a widget
    // context, so this is a construction smoke test.
    for (ThemeName t : { ThemeName::Light, ThemeName::Dark, ThemeName::Sepia,
                         ThemeName::Midnight }) {
        Tui::ZPalette p = daemonPalette(t);
        Q_UNUSED(p);
    }
    QVERIFY(true);
}

QTEST_MAIN(TuiPaletteTests)
#include "tst_tui_palette.moc"
