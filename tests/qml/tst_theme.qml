import QtQuick
import QtTest
import DaemonApp.Theme

// Proves the theme switch is live and the per-theme palette is correct for all
// three themes (Light / Dark / Sepia). Guards the Fault 0 regression where an
// id/property collision made `setTheme` a no-op and left only Light rendering.
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
                background: "#ffffff", sidebar: "#ededed", text: "#37352e",
                sidebarSelection: "#448ac9", sidebarHover: "#b4d0e9",
                searchBackground: "#ffffff", searchFocusBorder: "#a6c6e4",
                listSelection: "#dae9ef",
                codeBackground: "#f1f1ef", activeBlockBackground: "#f8fbff",
                activeBlockBorder: "#d7e9fb"
            },
            {
                tag: "Dark", theme: "Dark", isDark: true, isSepia: false,
                background: "#191919", sidebar: "#333333", text: "#d6d6d6",
                sidebarSelection: "#448ac9", sidebarHover: "#233445",
                searchBackground: "#2a2a2a", searchFocusBorder: "#2c536f",
                listSelection: "#233445",
                codeBackground: "#2a2a2a", activeBlockBackground: "#1f2733",
                activeBlockBorder: "#2c4a63"
            },
            {
                tag: "Sepia", theme: "Sepia", isDark: false, isSepia: true,
                background: "#fbf0d9", sidebar: "#ededed", text: "#321e03",
                sidebarSelection: "#448ac9", sidebarHover: "#b4d0e9",
                searchBackground: "#fbf0d9", searchFocusBorder: "#a6c6e4",
                listSelection: "#dae9ef",
                codeBackground: "#efe6d2", activeBlockBackground: "#f3ead2",
                activeBlockBorder: "#d8c79a"
            }
        ];
    }

    function test_palette(data) {
        Theme.setTheme(data.theme);

        compare(Theme.theme, data.theme, "active theme");
        compare(Theme.isDark, data.isDark, "isDark flag");
        compare(Theme.isSepia, data.isSepia, "isSepia flag");

        compare(hex(Theme.background), data.background, "background");
        compare(hex(Theme.sidebar), data.sidebar, "sidebar");
        compare(hex(Theme.text), data.text, "text");
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

        verify(light !== dark, "Light vs Dark background must differ");
        verify(dark !== sepia, "Dark vs Sepia background must differ");
        verify(light !== sepia, "Light vs Sepia background must differ");
    }

    // Invalid names are ignored (setTheme guards the value).
    function test_invalid_theme_ignored() {
        Theme.setTheme("Dark");
        Theme.setTheme("Nonsense");
        compare(Theme.theme, "Dark", "invalid theme name is ignored");
    }
}
