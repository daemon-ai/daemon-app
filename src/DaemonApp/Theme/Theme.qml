pragma Singleton
import QtQuick

// Single source of truth for the palette, ported from Daino Notes' dual theming
// (the QSS widget stylesheets + the QML `themeData` tokens). One `theme` switch
// drives every color token so the whole app recolors live. Values mirror
// architecture-map.md section 4 and styles/styles/*.css.
QtObject {
    id: theme

    // "Light" | "Dark" | "Sepia". Mutate via setTheme() to recolor the app.
    property string theme: "Light"
    readonly property bool isDark: theme === "Dark"
    readonly property bool isSepia: theme === "Sepia"

    function setTheme(name) {
        if (name === "Light" || name === "Dark" || name === "Sepia")
            theme = name;
    }

    // --- Surfaces -----------------------------------------------------------
    // Main / right-pane background (white / rgb(25,25,25) / rgb(251,240,217)).
    readonly property color background: isDark ? "#191919" : isSepia ? "#fbf0d9" : "#ffffff"
    // Middle column (conversations list) shares the main background in Daino.
    readonly property color surface: background
    // Left sidebar (rgb(237,237,237) / rgb(51,51,51) / rgb(251,240,217)).
    readonly property color sidebar: isDark ? "#333333" : isSepia ? "#fbf0d9" : "#ededed"

    // --- Lines / borders ----------------------------------------------------
    // Subtle separators between list rows (editor tokens #ededec / #3a3a3a).
    readonly property color border: isDark ? "#3a3a3a" : isSepia ? "#e6dcc4" : "#ededec"
    // The vertical splitter between columns (rgb(217,217,217) / rgb(54,55,57)).
    readonly property color splitter: isDark ? "#363739" : isSepia ? "#bfbfbf" : "#d9d9d9"

    // --- Text ---------------------------------------------------------------
    readonly property color text: isDark ? "#d6d6d6" : isSepia ? "#321e03" : "#37352e"
    readonly property color textMuted: isDark ? "#868686" : "#7d7c78"
    // Notes-count label is the same gray in every theme (rgb(132,132,132)).
    readonly property color countText: "#848484"

    // --- Interaction states (hover / pressed / selection) -------------------
    readonly property color hover: isDark ? "#313131" : isSepia ? "#f1e7d2" : "#efefef"
    readonly property color pressed: isDark ? "#2c2c2c" : isSepia ? "#dfd5c0" : "#dfdfde"
    readonly property color selection: isDark ? "#2c2c2c" : isSepia ? "#efe2c6" : "#e8f0fb"

    // --- Accent (selected text, cursor, focus) ------------------------------
    readonly property color accent: "#2383e2"

    // --- Search field (per searchEdit in main-window.css) -------------------
    readonly property color searchBackground: isDark ? "#2a2a2a" : background
    readonly property color searchBorder: isDark ? "#2a2a2a" : "#cdcdcd"
    readonly property color searchFocusBorder: isDark ? "#2c536f" : "#a6c6e4"
    readonly property color searchSelection: "#d2e4fa"

    // --- Icons --------------------------------------------------------------
    // IconButton default glyph color (Dark uses a blue accent like Daino).
    readonly property color iconColor: isDark ? "#5b94f5" : "#000000"
    // Muted toolbar glyphs (rgb(162,163,164) dark / rgb(100,100,100) light/sepia).
    readonly property color iconMuted: isDark ? "#a2a3a4" : "#646464"

    // --- Theme swatch chips (ThemeChooserButton) ----------------------------
    readonly property color chipLight: "#f7f7f7"
    readonly property color chipDark: "#191919"
    readonly property color chipSepia: "#f7cc6f"

    // --- Typography ---------------------------------------------------------
    // Daino sizes by point with a per-platform offset; we are desktop-first.
    readonly property string platform: "Other"
    readonly property int pointSizeOffset: platform === "Apple" ? 0 : -3

    // --- Metrics ------------------------------------------------------------
    readonly property int spacingSmall: 6
    readonly property int spacing: 12
    readonly property int spacingLarge: 20
    readonly property int radius: 5

    readonly property int sidebarWidth: 240
    readonly property int listWidth: 300
}
