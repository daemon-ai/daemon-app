pragma Singleton
import QtQuick
import DaemonApp.ThemeCore

// Single source of truth for the palette across all four themes (Light / Dark /
// Sepia / Midnight). One `theme` switch drives every color token so the whole
// app recolors live. The base color hexes now live once in the shared C++
// ThemePalette (DaemonApp.ThemeCore); each base token below binds
// ThemeTokens.colorFor(theme, "<name>") so it stays reactive (the binding
// depends on `theme`) while the GUI and the TUI read the exact same values.
// Derived aliases and the layout metrics stay QML-only - the TUI needs neither.
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
    readonly property color background: ThemeTokens.colorFor(theme, "background")
    // Middle column (sessions list) shares the main background.
    readonly property color surface: background
    // Left sidebar (frameLeft): a chrome panel in the same hue family as the
    // background, offset in lightness. Light neutral grey; Dark lifted; Midnight
    // brighter navy; Sepia a warm parchment tan (between background and border)
    // so the column stays in the warm palette instead of clashing grey.
    readonly property color sidebar: ThemeTokens.colorFor(theme, "sidebar")
    // Sessions list surface: joins the sidebar chrome in the dark/sepia/
    // midnight themes, but an off-white in Light so the middle column reads with
    // the editor instead of as the grey sidebar chrome.
    readonly property color listBackground: ThemeTokens.colorFor(theme, "listBackground")

    // --- Lines / borders ----------------------------------------------------
    // Subtle separators between list rows (editor tokens #ededec / #3a3a3a).
    readonly property color border: ThemeTokens.colorFor(theme, "border")
    // The vertical splitter between columns (rgb(217,217,217) / rgb(54,55,57)).
    readonly property color splitter: ThemeTokens.colorFor(theme, "splitter")

    // --- Text ---------------------------------------------------------------
    // Editor / general body text (themeData text token). Midnight = Psyche cream.
    readonly property color text: ThemeTokens.colorFor(theme, "text")
    readonly property color textMuted: ThemeTokens.colorFor(theme, "textMuted")
    // Notes-count label (same gray in Light/Dark/Sepia; periwinkle in Midnight).
    readonly property color countText: ThemeTokens.colorFor(theme, "countText")

    // --- Interaction states (hover / pressed / selection) -------------------
    // Editor top-bar tokens: highlight / pressed.
    readonly property color hover: ThemeTokens.colorFor(theme, "hover")
    readonly property color pressed: ThemeTokens.colorFor(theme, "pressed")

    // --- Row washes (nav/session selection) --------------------------
    // Translucent NEUTRAL washes for sidebar/list rows - a few percent off the
    // chrome `sidebar` surface, never the accent. Selection reads as the row
    // "pressed into" the panel, not a
    // bright pill. Dark/Midnight lighten over their dark surface; Sepia stays a
    // warm neutral tan (no sienna); Light is a plain grey.
    readonly property color rowHover: ThemeTokens.colorFor(theme, "rowHover")
    readonly property color rowActive: ThemeTokens.colorFor(theme, "rowActive")
    // Selected row when the list is unfocused (a touch weaker than rowActive).
    readonly property color rowActiveInactive: ThemeTokens.colorFor(theme, "rowActiveInactive")

    // --- Accent (selected text, cursor, focus) ------------------------------
    // Per-theme so every highlight tracks the palette:
    // blue, Sepia uses a warm sienna that matches the parchment palette, Midnight
    // uses its legible brighter blue. Drives toggles, selection fills, icons, the
    // "+" buttons, the Send icon, font-style label, focus rings, etc.
    readonly property color accent: ThemeTokens.colorFor(theme, "accent")

    // --- Sidebar (NodeTreeView delegates) -----------------------------------
    // text-forward: unselected rows sit at a SECONDARY (muted) tone and
    // brighten to primary `text` when selected/hovered. This is the default
    // (unselected) folder/All/Tag label color.
    readonly property color sidebarText: ThemeTokens.colorFor(theme, "sidebarText")
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

    // --- Sessions list (NoteListView delegate) -------------------------
    // Notes-bar scope title - styled as an accent section header in the component.
    readonly property color listTitle: accent
    // Note row title default - SECONDARY tone; brightens to `text` on hover/select.
    readonly property color listText: ThemeTokens.colorFor(theme, "listText")
    // Note snippet (tertiary).
    readonly property color listSnippet: ThemeTokens.colorFor(theme, "listSnippet")
    // Selected note row - neutral wash (fill-only, no border); weaker when unfocused.
    readonly property color listSelection: rowActive
    readonly property color listSelectionInactive: rowActiveInactive
    // Row separator (kept for reference; rows use spacing + fills, no divider).
    readonly property color listSeparator: ThemeTokens.colorFor(theme, "listSeparator")

    // --- Search field (per searchEdit in main-window.css) -------------------
    readonly property color searchBackground: ThemeTokens.colorFor(theme, "searchBackground")
    readonly property color searchBorder: ThemeTokens.colorFor(theme, "searchBorder")
    readonly property color searchFocusBorder: ThemeTokens.colorFor(theme, "searchFocusBorder")
    readonly property color searchText: ThemeTokens.colorFor(theme, "searchText")
    readonly property color searchSelection: ThemeTokens.colorFor(theme, "searchSelection")

    // --- Markdown / block editor (Transcript renderer) ----------------------
    // These tokens carry the ported BlockEditor's vocabulary so the upstream QML
    // components work by only adding `import DaemonApp.Theme`; the mapping to the
    // theme-aware palette lives here (one place) instead of scattered edits.
    // Inline-code and code-fence background (fed to the C++ projector palette).
    readonly property color codeBackground: ThemeTokens.colorFor(theme, "codeBackground")
    readonly property color codeText: text
    // Markdown links reuse the accent.
    readonly property color link: accent
    // Subtle raised surface (code/table-header/diagram backgrounds).
    readonly property color surfaceRaised: ThemeTokens.colorFor(theme, "surfaceRaised")
    // Muted body text (alias of textMuted in the block-editor vocabulary).
    readonly property color mutedText: textMuted
    // Text selection in the renderer.
    readonly property color selection: searchSelection
    readonly property color selectionText: text
    readonly property color transparent: "transparent"
    // Active (focused) block highlight.
    readonly property color activeBlockBackground: ThemeTokens.colorFor(theme, "activeBlockBackground")
    readonly property color activeBlockBorder: ThemeTokens.colorFor(theme, "activeBlockBorder")

    // --- Agent transcript blocks (reasoning / tool calls + sub-renderers) ---
    // A tool/reasoning card is a subtle raised surface with a hairline border,
    // matching the code-card vocabulary. Status colors track the lifecycle:
    // running borrows the accent, ok is a calm green, error is the danger red.
    readonly property color toolSurface: surfaceRaised
    readonly property color toolBorder: border
    // Header strip behind the tool title row (a touch deeper than the body).
    readonly property color toolHeader: ThemeTokens.colorFor(theme, "toolHeader")
    readonly property color statusRunning: accent
    readonly property color statusOk: ThemeTokens.colorFor(theme, "statusOk")
    readonly property color statusError: danger
    // Pending state (e.g. a tool awaiting approval) borrows the amber warning tone.
    readonly property color statusWarning: warning
    // Reasoning reads as quiet aside text on a faint surface.
    readonly property color reasoningText: textMuted
    readonly property color reasoningSurface: surfaceRaised
    // Unified-diff line washes (GitHub-like, theme-aware).
    readonly property color diffAddBackground: ThemeTokens.colorFor(theme, "diffAddBackground")
    readonly property color diffDelBackground: ThemeTokens.colorFor(theme, "diffDelBackground")
    readonly property color diffAddText: ThemeTokens.colorFor(theme, "diffAddText")
    readonly property color diffDelText: ThemeTokens.colorFor(theme, "diffDelText")
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

    // --- Message / role layer (user bubbles, system + process notices) ------
    // User messages sit in a glass bubble: a faint accent-tinted surface with a
    // hairline border, distinct from the full-width assistant stream. System and
    // process notices are quiet centered chrome on the raised surface.
    readonly property color bubbleUser: ThemeTokens.colorFor(theme, "bubbleUser")
    readonly property color bubbleUserBorder: ThemeTokens.colorFor(theme, "bubbleUserBorder")
    readonly property color bubbleUserText: text
    // Per-turn role header: a small avatar chip + the role name ("You"/"Daemon").
    // The avatars are quiet tinted chips; the name reads in the muted chrome ink.
    readonly property color roleAvatarUser: accent
    readonly property color roleAvatarAssistant: ThemeTokens.colorFor(theme, "roleAvatarAssistant")
    readonly property color roleAvatarUserIcon: ThemeTokens.colorFor(theme, "roleAvatarUserIcon")
    readonly property color roleAvatarAssistantIcon: textMuted
    readonly property color roleName: textMuted
    // Optional left accent rail down a user turn (A1). A muted accent hairline.
    readonly property color userRail: bubbleUserBorder
    // Footer action row (copy / regenerate / branch) under an assistant message.
    readonly property color messageFooterText: textMuted
    readonly property color messageFooterHover: hover
    // System notice (steer / slash / generic) - centered, muted, faint surface.
    readonly property color systemNoticeSurface: surfaceRaised
    readonly property color systemNoticeBorder: border
    readonly property color systemNoticeText: textMuted
    // Process notification - a terminal-icon notice with a collapsible body.
    readonly property color processNoticeSurface: surfaceRaised
    readonly property color processNoticeBorder: border
    readonly property color processNoticeIcon: accent

    // --- Icons --------------------------------------------------------------
    // IconButton default glyph color (dark themes use a blue accent).
    readonly property color iconColor: ThemeTokens.colorFor(theme, "iconColor")
    // Muted toolbar glyphs = toolButtonColor (rgb 162,163,164 dark / 100,100,100).
    readonly property color iconMuted: ThemeTokens.colorFor(theme, "iconMuted")

    // --- Theme swatch chips (ThemeChooserButton) ----------------------------
    readonly property color chipLight: ThemeTokens.colorFor(theme, "chipLight")
    readonly property color chipDark: ThemeTokens.colorFor(theme, "chipDark")
    readonly property color chipSepia: ThemeTokens.colorFor(theme, "chipSepia")
    // Recognizable royal blue for the Midnight swatch.
    readonly property color chipMidnight: ThemeTokens.colorFor(theme, "chipMidnight")

    // --- Status bar ------------------------------------
    // A thin chrome strip below the columns. uses the sidebar surface for
    // its footer background; we follow that so it reads as one chrome band and
    // picks up the Midnight navy. The strip is slightly distinct from `sidebar`
    // to separate it from the left column where they meet.
    readonly property color statusBarBackground: ThemeTokens.colorFor(theme, "statusBarBackground")
    // Default (tertiary) label color in the bar.
    readonly property color statusBarText: ThemeTokens.colorFor(theme, "statusBarText")
    // Item hover fill.
    readonly property color statusBarHover: ThemeTokens.colorFor(theme, "statusBarHover")
    // Tone colors for degraded / offline / failure states (amber / red).
    readonly property color warning: ThemeTokens.colorFor(theme, "warning")
    readonly property color danger: ThemeTokens.colorFor(theme, "danger")

    // --- Typography ---------------------------------------------------------
    // Sizes by point with a per-platform offset. Set from Qt.platform.os at
    // startup (Main.qml); defaults to the desktop-first "Other" profile.
    property string platform: "Other"
    readonly property int pointSizeOffset: platform === "Apple" ? 0 : -3

    // --- Touch / density ----------------------------------------------------
    // Set true on Android/iOS at startup (Main.qml). Bumps interactive chrome
    // to finger-sized targets; desktop keeps its dense defaults (touch === false)
    // so there is no desktop regression.
    property bool touch: false

    // Minimum finger target (Material/HIG ~44-48dp); desktop falls back to the
    // dense 28px chrome height.
    readonly property int tapTargetMin: touch ? 48 : 28
    // Icon button footprint (desktop default 30x28).
    readonly property int iconButtonWidth: touch ? 48 : 30
    readonly property int iconButtonHeight: touch ? 44 : 28
    // Selectable list / tree rows (desktop default 28px).
    readonly property int rowHeight: touch ? 48 : 28

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
