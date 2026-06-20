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
    // Left sidebar (frameLeft): a chrome panel in the same hue family as the
    // background, offset in lightness. Light neutral grey; Dark lifted; Midnight
    // brighter navy; Sepia a warm parchment tan (between background and border)
    // so the column stays in the warm palette instead of clashing grey.
    readonly property color sidebar: isMidnight ? "#09286f" : isDark ? "#333333" : isSepia ? "#ece0c2" : "#ededed"
    // Conversations list surface: joins the sidebar chrome in the dark/sepia/
    // midnight themes, but an off-white in Light so the middle column reads with
    // the editor instead of as the grey sidebar chrome.
    readonly property color listBackground: isDark || isMidnight || isSepia ? sidebar : "#f7f7f7"

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

    // --- Row washes (nav/session selection) --------------------------
    // Translucent NEUTRAL washes for sidebar/list rows - a few percent off the
    // chrome `sidebar` surface, never the accent. Selection reads as the row
    // "pressed into" the panel, not a
    // bright pill. Dark/Midnight lighten over their dark surface; Sepia stays a
    // warm neutral tan (no sienna); Light is a plain grey.
    readonly property color rowHover: isMidnight ? "#123882" : isDark ? "#3d3d3d" : isSepia ? "#e3d4ad" : "#e6e6e6"
    readonly property color rowActive: isMidnight ? "#1a4499" : isDark ? "#474747" : isSepia ? "#d8c79a" : "#dcdcdc"
    // Selected row when the list is unfocused (a touch weaker than rowActive).
    readonly property color rowActiveInactive: isMidnight ? "#163a8c" : isDark ? "#424242" : isSepia ? "#dfd0a4" : "#e1e1e1"

    // --- Accent (selected text, cursor, focus) ------------------------------
    // Per-theme so every highlight tracks the palette:
    // blue, Sepia uses a warm sienna that matches the parchment palette, Midnight
    // uses its legible brighter blue. Drives toggles, selection fills, icons, the
    // "+" buttons, the Send icon, font-style label, focus rings, etc.
    readonly property color accent: isMidnight ? "#9fb3e6" : isSepia ? "#b06a2c" : isDark ? "#757575" : "#2383e2"

    // --- Sidebar (NodeTreeView delegates) -----------------------------------
    // text-forward: unselected rows sit at a SECONDARY (muted) tone and
    // brighten to primary `text` when selected/hovered. This is the default
    // (unselected) folder/All/Tag label color.
    readonly property color sidebarText: isMidnight ? "#aebfe6" : isDark ? "#9a9a9a" : isSepia ? "#6b5836" : "#5f5f5c"
    // Selected row fill - a neutral wash (NOT the accent), so the row reads as
    // pressed into the panel. Nav rows additionally draw a `border` hairline.
    readonly property color sidebarSelection: rowActive
    // Selected row text/icon/count - just brightens to primary foreground.
    readonly property color sidebarSelectedText: text
    // Row hover - the lighter neutral wash.
    readonly property color sidebarHover: rowHover
    // Folder/tag icon color when unselected - monochrome muted (not accent).
    readonly property color sidebarIcon: textMuted
    // Folders/Tags separator label - accent-colored:
    // section headers are the one place accent appears prominently.
    readonly property color separatorText: accent
    // Separator "+" add button - monochrome muted (with darker hover/pressed).
    readonly property color addButton: textMuted
    readonly property color addButtonHover: Qt.darker(textMuted, 1.25)
    readonly property color addButtonPressed: Qt.darker(textMuted, 1.4)

    // --- Agent tree (supervision-tree node rows) ----------------------------
    // Per-depth indent step and the chevron/twistie gutter width. The tree is
    // arbitrary-depth, so indentation is just depth * treeIndent (no caps).
    readonly property int treeIndent: 14
    readonly property int twistieSize: 14
    // Node lifecycle state dot. Running tracks the accent; finished is muted;
    // unknown shows no dot (see Sidebar.stateColor).
    readonly property color stateRunning: accent
    readonly property color stateFinished: countText

    // --- Conversations list (NoteListView delegate) -------------------------
    // Notes-bar scope title - styled as an accent section header in the component.
    readonly property color listTitle: accent
    // Note row title default - SECONDARY tone; brightens to `text` on hover/select.
    readonly property color listText: isMidnight ? "#cdd9f5" : isDark ? "#b0b0b0" : isSepia ? "#5a4a2e" : "#4a4a48"
    // Note snippet (tertiary).
    readonly property color listSnippet: "#8e9296"
    // Selected note row - neutral wash (fill-only, no border); weaker when unfocused.
    readonly property color listSelection: rowActive
    readonly property color listSelectionInactive: rowActiveInactive
    // Row separator (kept for reference; rows use spacing + fills, no divider).
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

    // --- Agent transcript blocks (reasoning / tool calls + sub-renderers) ---
    // A tool/reasoning card is a subtle raised surface with a hairline border,
    // matching the code-card vocabulary. Status colors track the lifecycle:
    // running borrows the accent, ok is a calm green, error is the danger red.
    readonly property color toolSurface: surfaceRaised
    readonly property color toolBorder: border
    // Header strip behind the tool title row (a touch deeper than the body).
    readonly property color toolHeader: isMidnight ? "#0e1d40" : isDark ? "#242424" : isSepia ? "#efe4c8" : "#eef0f2"
    readonly property color statusRunning: accent
    readonly property color statusOk: isDarkMode ? "#5fbf73" : "#2f9e44"
    readonly property color statusError: danger
    // Reasoning reads as quiet aside text on a faint surface.
    readonly property color reasoningText: textMuted
    readonly property color reasoningSurface: surfaceRaised
    // Unified-diff line washes (GitHub-like, theme-aware).
    readonly property color diffAddBackground: isDarkMode ? "#10301c" : "#e6ffec"
    readonly property color diffDelBackground: isDarkMode ? "#3a1d1d" : "#ffebe9"
    readonly property color diffAddText: isDarkMode ? "#7ee787" : "#1a7f37"
    readonly property color diffDelText: isDarkMode ? "#ff7b72" : "#cf222e"
    readonly property color diffHunkText: accent
    // ANSI 16-color SGR palette (indices 0-7 normal, 8-15 bright). A single
    // palette legible on both light and dark surfaces; AnsiText maps fg/bg index
    // to these, falling back to `text` for the default (-1).
    readonly property var ansiPalette: [
        "#3b3b3b", "#c0392b", "#2f9e44", "#b8860b",
        "#2383e2", "#a347ba", "#1098ad", "#a8a8a8",
        "#6b6b6b", "#f06a6a", "#5fbf73", "#e0a93b",
        "#6fa0ff", "#cf8ee0", "#3bc9db", "#f0f0f0"
    ]

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

    // --- Status bar ------------------------------------
    // A thin chrome strip below the columns. uses the sidebar surface for
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
    // Chrome control radius (icon buttons, fields, menus, swatches). Squared
    // toward the status bar's flat 4px icon-button radius.
    readonly property int radius: 4

    // Inset rounded selection rows: the highlight
    // pill is inset from the column edges and rounded, instead of full-bleed.
    readonly property int rowRadius: 6
    readonly property int rowInset: 6
    readonly property int rowVInset: 1

    // Control hover/selection transition (~100ms).
    readonly property int motionFast: 100

    // Tracked uppercase section labels (FOLDERS/TAGS, settings headers): the
    // refined chrome-label signature (~0.12em at 11px, 0.16em).
    readonly property int labelSize: 11
    readonly property real labelTracking: 1.2

    // Block-editor renderer metrics (theme-independent constants, upstream names).
    readonly property int pageMargin: 24
    readonly property int blockPadding: 10
    readonly property int contentSpacing: 18
    readonly property int smallSpacing: 6
    readonly property int hairline: 1
    readonly property int radiusSmall: 8
    readonly property int bodyFontSize: 15
    readonly property int captionFontSize: 13

    readonly property int sidebarWidth: 232
    readonly property int listWidth: 288

    // footer is 20px; 22 reads better at desktop DPI with our 11px labels.
    readonly property int statusBarHeight: 22
}
