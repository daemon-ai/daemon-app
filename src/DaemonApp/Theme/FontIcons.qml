pragma Singleton
import QtQuick

// Loads the bundled fonts and exposes their family names plus the glyph map.
// using the exact bundled Inter + FontAwesome + Material Symbols fonts.
//
// NOTE: fa-regular is intentionally NOT loaded. FontAwesome's solid and regular
// faces share the family name "Font Awesome 6 Free"; loading both makes Qt
// resolve the Regular face for solid glyphs (which it lacks) -> tofu boxes.
// Solid is the only face the UI uses, so loading it alone keeps `faSolid`
// unambiguous.
QtObject {
    id: icons

    property FontLoader _interLoader: FontLoader { source: "fonts/InterVariable.ttf" }
    property FontLoader _faSolidLoader: FontLoader { source: "fonts/fa-solid-900.ttf" }
    property FontLoader _faBrandsLoader: FontLoader { source: "fonts/fa-brands-400.ttf" }
    property FontLoader _mtLoader: FontLoader { source: "fonts/material-symbols-outlined.ttf" }
    property FontLoader _monoLoader: FontLoader { source: "fonts/iAWriterMonoS-Regular.ttf" }

    // Display font for all app text.
    readonly property string display: _interLoader.name
    // Monospace font for code spans / fences in the markdown renderer.
    readonly property string mono: _monoLoader.name
    // Icon font families.
    readonly property string faSolid: _faSolidLoader.name
    readonly property string faBrands: _faBrandsLoader.name
    readonly property string mtSymbols: _mtLoader.name

    // --- Glyph codes (FontAwesome 6 Free, solid) ----------------------------
    readonly property string fa_magnifying_glass: "\uf002"
    readonly property string fa_trash: "\uf1f8"
    readonly property string fa_plus: "\uf067"
    readonly property string fa_minus: "\uf068"
    readonly property string fa_ellipsis_h: "\uf141"
    readonly property string fa_pen_to_square: "\uf044"
    readonly property string fa_gear: "\uf013"
    readonly property string fa_sliders: "\uf1de"
    // arrow-left/right-to-line are FontAwesome Pro; Free uses angles-left/right
    // for the same "collapse to edge" affordance.
    readonly property string fa_angles_left: "\uf100"
    readonly property string fa_angles_right: "\uf101"
    readonly property string fa_angles_up: "\uf102"
    readonly property string fa_angles_down: "\uf103"
    readonly property string fa_caret_right: "\uf0da"
    readonly property string fa_caret_down: "\uf0d7"
    readonly property string fa_caret_left: "\uf0d9"
    readonly property string fa_undo_alt: "\uf2ea"
    readonly property string fa_chevron_down: "\uf078"
    readonly property string fa_chevron_right: "\uf054"
    readonly property string fa_chevron_left: "\uf053"
    readonly property string fa_circle: "\uf111"
    readonly property string fa_circle_xmark: "\uf057"
    readonly property string fa_circle_check: "\uf058"
    readonly property string fa_xmark: "\uf00d"
    readonly property string fa_bell: "\uf0f3"
    readonly property string check: "\uf00c"
    readonly property string fa_star: "\uf005"
    readonly property string fa_link: "\uf0c1"
    readonly property string fa_list_ul: "\uf0ca"
    readonly property string fa_square_check: "\uf14a"
    readonly property string fa_heading: "\uf1dc"
    readonly property string fa_font: "\uf031"
    readonly property string fa_image: "\uf03e"
    readonly property string fa_indent: "\uf03c"
    // Conversation-domain glyphs (full FontAwesome).
    readonly property string fa_folder: "\uf07b"
    readonly property string fa_tag: "\uf02b"
    readonly property string fa_comments: "\uf086"
    readonly property string fa_box_archive: "\uf187"
    readonly property string fa_paper_plane: "\uf1d8"

    // Agent-tree node-kind glyphs (cosmetic; one per AgentNodeKind).
    readonly property string fa_robot: "\uf544"   // engine / agent leaf
    readonly property string fa_server: "\uf233"  // host
    readonly property string fa_sitemap: "\uf0e8" // orchestrator / fleet

    // --- Agent transcript block glyphs --------------------------------------
    // Reasoning disclosure + tool-call header/tone icons.
    readonly property string fa_brain: "\uf5dc"
    readonly property string fa_wrench: "\uf0ad"
    readonly property string fa_copy: "\uf0c5"
    readonly property string fa_globe: "\uf0ac"
    readonly property string fa_code: "\uf121"
    // clarify prompt + image-result download affordances.
    readonly property string fa_circle_question: "\uf059"
    readonly property string fa_download: "\uf019"

    // --- Status bar glyphs --------------------------------------------------
    readonly property string fa_terminal: "\uf120"
    readonly property string fa_signal: "\uf012"
    readonly property string fa_circle_exclamation: "\uf06a"
    readonly property string fa_wand_magic_sparkles: "\uf72b"
    readonly property string fa_spinner: "\uf110"
    readonly property string fa_clock: "\uf017"
    readonly property string fa_bolt: "\uf0e7"
    readonly property string fa_hashtag: "\uf292"

    // --- Composer glyphs ----------------------------------------------------
    // Send (primary), stop (filled square), queue (layer group), mention (@),
    // attach (paperclip), and dictation (reserved; voice is not wired yet).
    readonly property string fa_arrow_up: "\uf062"
    readonly property string fa_square: "\uf0c8"
    readonly property string fa_layer_group: "\uf5fd"
    readonly property string fa_at: "\uf1fa"
    readonly property string fa_paperclip: "\uf0c6"
    readonly property string fa_microphone: "\uf130"

    // --- Material Symbols ---------------------------------------------------
    readonly property string mt_view_kanban: "\ueb7f"
    readonly property string mt_article: "\uef42"
    readonly property string mt_globe: "\ue894"
    readonly property string mt_code: "\ue86f"
}
