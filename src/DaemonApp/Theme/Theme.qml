pragma Singleton
import QtQuick

// Single source of truth for the palette across all four themes (Light / Dark /
// Sepia / Midnight). One `theme` switch drives every color token so the whole
// app recolors live.
//
// Midnight: deep navy surfaces,
// Psyche-cream text, periwinkle muted text, and blue accents.
//
// NOTE: the id MUST NOT be `theme` - that collides with the `theme` property and
// makes `theme = name` target the component (not an lvalue) while every
// `theme === "Dark"` compares an object to a string (always false). Use `root`.
QtObject {
    id: root

    // "Light" | "Dark" | "Sepia" | "Midnight". Mutate via setTheme() to recolor.
    property string theme: "Light"
    readonly property bool isDark: theme === "Dark"
    readonly property bool isSepia: theme === "Sepia"
    readonly property bool isMidnight: theme === "Midnight"
    // True for any dark-background theme; controls that pick a light/dark variant
    // (Switch track, CheckBox glyph, Tooltip) should branch on this, not isDark.
    readonly property bool isDarkMode: isDark || isMidnight

    function setTheme(name) {
        if (name === "Light" || name === "Dark" || name === "Sepia" || name === "Midnight")
            theme = name;
    }

    // --- Surfaces -----------------------------------------------------------
    // Main / right-pane background (white / rgb(25,25,25) / rgb(251,240,217)).
    readonly property color background: isMidnight ? "#0d162d" : isDark ? "#191919" : isSepia ? "#fbf0d9" : "#ffffff"
    // Middle column (conversations list) shares the main background.
    readonly property color surface: background
    // Left sidebar (frameLeft): Light AND Sepia are rgb(237,237,237); Dark 51.
    readonly property color sidebar: isMidnight ? "#09286f" : isDark ? "#333333" : "#ededed"

    // --- Lines / borders ----------------------------------------------------
    // Subtle separators between list rows (editor tokens #ededec / #3a3a3a).
    readonly property color border: isMidnight ? "#233a6b" : isDark ? "#3a3a3a" : isSepia ? "#e6dcc4" : "#ededec"
    // The vertical splitter between columns (rgb(217,217,217) / rgb(54,55,57)).
    readonly property color splitter: isMidnight ? "#1b2d57" : isDark ? "#363739" : isSepia ? "#bfbfbf" : "#d9d9d9"

    // --- Text ---------------------------------------------------------------
    // Editor / general body text (themeData text token). Midnight = Psyche cream.
    readonly property color text: isMidnight ? "#ffe6cb" : isDark ? "#d6d6d6" : isSepia ? "#321e03" : "#37352e"
    readonly property color textMuted: isMidnight ? "#b5c7f3" : isDark ? "#868686" : "#7d7c78"
    // Notes-count label (same gray in Light/Dark/Sepia; periwinkle in Midnight).
    readonly property color countText: isMidnight ? "#8fa6d8" : "#848484"

    // --- Interaction states (hover / pressed / selection) -------------------
    // Editor top-bar tokens: highlight / pressed.
    readonly property color hover: isMidnight ? "#16224a" : isDark ? "#313131" : isSepia ? "#f1e7d2" : "#efefef"
    readonly property color pressed: isMidnight ? "#1b2a55" : isDark ? "#2c2c2c" : isSepia ? "#f1e7d2" : "#dfdfde"

    // --- Accent (selected text, cursor, focus) ------------------------------
    // Midnight uses the legible blue; the other themes share blue.
    readonly property color accent: isMidnight ? "#4f8cff" : "#2383e2"

    // --- Sidebar (NodeTreeView delegates) -----------------------------------
    // Folder/All/Tag title color (foldertreedelegateeditor m_titleColor).
    readonly property color sidebarText: isMidnight ? "#dbe4ff" : isDark ? "#d4d4d4" : "#1a1a1a"
    // Selected row fill (m_activeColor rgb 68,138,201) - same in all themes.
    readonly property color sidebarSelection: "#448ac9"
    // Selected row text/icon/count.
    readonly property color sidebarSelectedText: "#ffffff"
    // Row hover (NodeTreeDelegate m_hoverColor: rgb 180,208,233 / rgb 35,52,69).
    readonly property color sidebarHover: isMidnight ? "#16285c" : isDark ? "#233445" : "#b4d0e9"
    // Folder/tag icon color when unselected (m_folderIconColor rgb 68,138,201).
    readonly property color sidebarIcon: "#448ac9"
    // Folders/Tags separator label (m_separatorTextColor rgb 143,143,143).
    readonly property color separatorText: "#8f8f8f"
    // Separator "+" add button (blue / hover / pressed).
    readonly property color addButton: "#448ac9"
    readonly property color addButtonHover: "#336ea2"
    readonly property color addButtonPressed: "#27557d"

    // --- Conversations list (NoteListView delegate) -------------------------
    // Folder/title label in the notes bar (listviewLabel1).
    readonly property color listTitle: isMidnight ? "#cdd9f5" : isDark ? "#dfe0e0" : "#444444"
    // Note row title / date (m_titleColor / m_dateColor).
    readonly property color listText: isMidnight ? "#dbe4ff" : isDark ? "#d4d4d4" : "#1a1a1a"
    // Note snippet (m_contentColor rgb 142,146,150).
    readonly property color listSnippet: "#8e9296"
    // Selected note row (m_activeColor focused / m_notActiveColor unfocused).
    readonly property color listSelection: isMidnight ? "#16285c" : isDark ? "#233445" : "#dae9ef"
    readonly property color listSelectionInactive: isMidnight ? "#142850" : isDark ? "#233445" : "#afd4e4"
    // Row separator (m_separatorColor rgb 191,191,191 / white@50%).
    readonly property color listSeparator: isMidnight ? "#2a3f70" : isDark ? "#7fffffff" : "#bfbfbf"

    // --- Search field (per searchEdit in main-window.css) -------------------
    readonly property color searchBackground: isMidnight ? "#0b2150" : isDark ? "#2a2a2a" : isSepia ? "#fbf0d9" : "#ffffff"
    readonly property color searchBorder: isMidnight ? "#233a6b" : isDark ? "#2a2a2a" : "#cdcdcd"
    readonly property color searchFocusBorder: isMidnight ? "#3a63bd" : isDark ? "#2c536f" : "#a6c6e4"
    readonly property color searchText: isMidnight ? "#e6ecff" : isDark ? "#cfcfcf" : isSepia ? "#321e03" : "#1a1a1a"
    readonly property color searchSelection: isMidnight ? "#2f4f99" : "#d2e4fa"

    // --- Markdown / block editor (Transcript renderer) ----------------------
    // These tokens carry the ported BlockEditor's vocabulary so the upstream QML
    // components work by only adding `import DaemonApp.Theme`; the mapping to the
    // theme-aware palette lives here (one place) instead of scattered edits.
    // Inline-code and code-fence background (fed to the C++ projector palette).
    readonly property color codeBackground: isMidnight ? "#1a2c5c" : isDark ? "#2a2a2a" : isSepia ? "#efe6d2" : "#f1f1ef"
    readonly property color codeText: text
    // Markdown links reuse the accent.
    readonly property color link: accent
    // Subtle raised surface (code/table-header/diagram backgrounds).
    readonly property color surfaceRaised: isMidnight ? "#122247" : isDark ? "#202020" : isSepia ? "#f3ebd6" : "#f7f7f5"
    // Muted body text (alias of textMuted in the block-editor vocabulary).
    readonly property color mutedText: textMuted
    // Text selection in the renderer.
    readonly property color selection: searchSelection
    readonly property color selectionText: text
    readonly property color transparent: "transparent"
    // Active (focused) block highlight.
    readonly property color activeBlockBackground: isMidnight ? "#14224a" : isDark ? "#1f2733" : isSepia ? "#f3ead2" : "#f8fbff"
    readonly property color activeBlockBorder: isMidnight ? "#2c4a8f" : isDark ? "#2c4a63" : isSepia ? "#d8c79a" : "#d7e9fb"

    // --- Icons --------------------------------------------------------------
    // IconButton default glyph color (dark themes use a blue accent).
    readonly property color iconColor: isMidnight ? "#6fa0ff" : isDark ? "#5b94f5" : "#000000"
    // Muted toolbar glyphs = toolButtonColor (rgb 162,163,164 dark / 100,100,100).
    readonly property color iconMuted: isMidnight ? "#9fb3e6" : isDark ? "#a2a3a4" : "#646464"

    // --- Theme swatch chips (ThemeChooserButton) ----------------------------
    readonly property color chipLight: "#f7f7f7"
    readonly property color chipDark: "#191919"
    readonly property color chipSepia: "#f7cc6f"
    // Recognizable royal blue for the Midnight swatch.
    readonly property color chipMidnight: "#0d2f86"

    // --- Status bar (Hermes footer port) ------------------------------------
    // A thin chrome strip below the columns. Hermes uses the sidebar surface for
    // its footer background; we follow that so it reads as one chrome band and
    // picks up the Midnight navy. The strip is slightly distinct from `sidebar`
    // to separate it from the left column where they meet.
    readonly property color statusBarBackground: isMidnight ? "#0a1f57" : isDark ? "#272727" : isSepia ? "#efe4c8" : "#f2f2f2"
    // Default (tertiary) label color in the bar.
    readonly property color statusBarText: isMidnight ? "#8fa6d8" : isDark ? "#8a8a8a" : "#6b6a66"
    // Item hover fill.
    readonly property color statusBarHover: isMidnight ? "#142b66" : isDark ? "#343434" : isSepia ? "#e4d8b8" : "#e4e4e4"
    // Tone colors for degraded / offline / failure states (amber / red).
    readonly property color warning: isDarkMode ? "#e0a93b" : "#b97e16"
    readonly property color danger: isDarkMode ? "#f06a6a" : "#c0392b"

    // --- Typography ---------------------------------------------------------
    // Sizes by point with a per-platform offset; we are desktop-first.
    readonly property string platform: "Other"
    readonly property int pointSizeOffset: platform === "Apple" ? 0 : -3

    // --- Metrics ------------------------------------------------------------
    readonly property int spacingSmall: 6
    readonly property int spacing: 12
    readonly property int spacingLarge: 20
    readonly property int radius: 5

    // Block-editor renderer metrics (theme-independent constants, upstream names).
    readonly property int pageMargin: 24
    readonly property int blockPadding: 10
    readonly property int contentSpacing: 18
    readonly property int smallSpacing: 6
    readonly property int hairline: 1
    readonly property int radiusSmall: 8
    readonly property int bodyFontSize: 15
    readonly property int captionFontSize: 13

    readonly property int sidebarWidth: 240
    readonly property int listWidth: 300

    // Hermes footer is 20px; 22 reads better at desktop DPI with our 11px labels.
    readonly property int statusBarHeight: 22
}
