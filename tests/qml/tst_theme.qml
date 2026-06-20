import QtQuick
import QtTest
import DaemonApp.Theme

// Proves the theme switch is live and the per-theme palette is correct for all
// four themes (Light / Dark / Sepia / Midnight). Guards the Fault 0 regression
// where an id/property collision made `setTheme` a no-op and left only Light.
TestCase {
    name: "Theme"

    function init() {
        // Start from a known state; the singleton persists across functions.
        Theme.setTheme("Light");
    }

    function hex(c) {
        return c.toString();
    }

    function test_palette_data() {
        return [
            {
                tag: "Light", theme: "Light", isDark: false, isSepia: false,
                isMidnight: false,
                background: "#ffffff", sidebar: "#ededed", text: "#37352e",
                accent: "#2383e2",
                rowHover: "#e6e6e6", rowActive: "#dcdcdc",
                sidebarSelection: "#dcdcdc", sidebarHover: "#e6e6e6",
                searchBackground: "#ffffff", searchFocusBorder: "#a6c6e4",
                listSelection: "#dcdcdc",
                codeBackground: "#f1f1ef", activeBlockBackground: "#f8fbff",
                activeBlockBorder: "#d7e9fb"
            },
            {
                tag: "Dark", theme: "Dark", isDark: true, isSepia: false,
                isMidnight: false,
                background: "#191919", sidebar: "#333333", text: "#d6d6d6",
                accent: "#757575",
                rowHover: "#3d3d3d", rowActive: "#474747",
                sidebarSelection: "#474747", sidebarHover: "#3d3d3d",
                searchBackground: "#2a2a2a", searchFocusBorder: "#2c536f",
                listSelection: "#474747",
                codeBackground: "#2a2a2a", activeBlockBackground: "#1f2733",
                activeBlockBorder: "#2c4a63"
            },
            {
                tag: "Sepia", theme: "Sepia", isDark: false, isSepia: true,
                isMidnight: false,
                background: "#fbf0d9", sidebar: "#ece0c2", text: "#321e03",
                accent: "#b06a2c",
                rowHover: "#e3d4ad", rowActive: "#d8c79a",
                sidebarSelection: "#d8c79a", sidebarHover: "#e3d4ad",
                searchBackground: "#fbf0d9", searchFocusBorder: "#a6c6e4",
                listSelection: "#d8c79a",
                codeBackground: "#efe6d2", activeBlockBackground: "#f3ead2",
                activeBlockBorder: "#d8c79a"
            },
            {
                tag: "Midnight", theme: "Midnight", isDark: false, isSepia: false,
                isMidnight: true,
                background: "#0d162d", sidebar: "#09286f", text: "#ffe6cb",
                accent: "#9fb3e6",
                rowHover: "#123882", rowActive: "#1a4499",
                sidebarSelection: "#1a4499", sidebarHover: "#123882",
                searchBackground: "#0b2150", searchFocusBorder: "#3a63bd",
                listSelection: "#1a4499",
                codeBackground: "#1a2c5c", activeBlockBackground: "#14224a",
                activeBlockBorder: "#2c4a8f"
            }
        ];
    }

    function test_palette(data) {
        Theme.setTheme(data.theme);

        compare(Theme.theme, data.theme, "active theme");
        compare(Theme.isDark, data.isDark, "isDark flag");
        compare(Theme.isSepia, data.isSepia, "isSepia flag");
        compare(Theme.isMidnight, data.isMidnight, "isMidnight flag");
        compare(Theme.isDarkMode, data.isDark || data.isMidnight, "isDarkMode flag");

        compare(hex(Theme.background), data.background, "background");
        compare(hex(Theme.sidebar), data.sidebar, "sidebar");
        compare(hex(Theme.text), data.text, "text");
        compare(hex(Theme.accent), data.accent, "accent");
        // selection is a NEUTRAL wash (rowActive), not the accent;
        // icons/+ are monochrome (textMuted); section headers carry the accent.
        compare(hex(Theme.sidebarSelection), data.rowActive, "sidebarSelection tracks rowActive");
        compare(hex(Theme.listSelection), data.rowActive, "listSelection tracks rowActive");
        compare(hex(Theme.sidebarHover), data.rowHover, "sidebarHover tracks rowHover");
        compare(hex(Theme.sidebarIcon), hex(Theme.textMuted), "sidebarIcon is monochrome");
        compare(hex(Theme.addButton), hex(Theme.textMuted), "addButton is monochrome");
        compare(hex(Theme.separatorText), data.accent, "separatorText tracks accent");
        compare(hex(Theme.rowHover), data.rowHover, "rowHover");
        compare(hex(Theme.rowActive), data.rowActive, "rowActive");
        compare(hex(Theme.sidebarSelection), data.sidebarSelection, "sidebarSelection");
        compare(hex(Theme.sidebarHover), data.sidebarHover, "sidebarHover");
        compare(hex(Theme.searchBackground), data.searchBackground, "searchBackground");
        compare(hex(Theme.searchFocusBorder), data.searchFocusBorder, "searchFocusBorder");
        compare(hex(Theme.listSelection), data.listSelection, "listSelection");

        // Block-editor (Transcript renderer) tokens recolor per theme too.
        compare(hex(Theme.codeBackground), data.codeBackground, "codeBackground");
        compare(hex(Theme.activeBlockBackground), data.activeBlockBackground, "activeBlockBackground");
        compare(hex(Theme.activeBlockBorder), data.activeBlockBorder, "activeBlockBorder");
    }

    // Switching must actually change tokens (not a constant palette): capture the
    // background for each theme and assert all three differ.
    function test_tokens_change_between_themes() {
        Theme.setTheme("Light");
        var light = hex(Theme.background);
        Theme.setTheme("Dark");
        var dark = hex(Theme.background);
        Theme.setTheme("Sepia");
        var sepia = hex(Theme.background);
        Theme.setTheme("Midnight");
        var midnight = hex(Theme.background);

        verify(light !== dark, "Light vs Dark background must differ");
        verify(dark !== sepia, "Dark vs Sepia background must differ");
        verify(light !== sepia, "Light vs Sepia background must differ");
        verify(midnight !== dark, "Midnight vs Dark background must differ");
        verify(midnight !== sepia, "Midnight vs Sepia background must differ");
        verify(midnight !== light, "Midnight vs Light background must differ");
    }

    // Invalid names are ignored (setTheme guards the value).
    function test_invalid_theme_ignored() {
        Theme.setTheme("Dark");
        Theme.setTheme("Nonsense");
        compare(Theme.theme, "Dark", "invalid theme name is ignored");
    }
}
