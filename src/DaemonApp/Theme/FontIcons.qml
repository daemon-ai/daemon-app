pragma Singleton
import QtQuick

// Loads the bundled fonts and exposes their family names plus the FontAwesome
// glyph map (ported from Daino's FontIconLoader.qml + FontIconsCodes.qml).
// Use like: font.family: FontIcons.faSolid; text: FontIcons.fa_gear
QtObject {
    id: icons

    // Font files are bundled as module resources (see Theme/CMakeLists.txt),
    // resolved relative to this singleton's location in the module.
    property FontLoader _interLoader: FontLoader { source: "fonts/Inter.ttf" }
    property FontLoader _faSolidLoader: FontLoader { source: "fonts/fa-solid-900.ttf" }
    property FontLoader _faRegularLoader: FontLoader { source: "fonts/fa-regular-400.ttf" }
    property FontLoader _faBrandsLoader: FontLoader { source: "fonts/fa-brands-400.ttf" }

    // Display font for all app text.
    readonly property string display: _interLoader.name
    // Icon font families.
    readonly property string faSolid: _faSolidLoader.name
    readonly property string faRegular: _faRegularLoader.name
    readonly property string faBrands: _faBrandsLoader.name

    // --- Glyph codes (FontAwesome 6 Free) -----------------------------------
    readonly property string fa_magnifying_glass: "\uf002"
    readonly property string fa_trash: "\uf1f8"
    readonly property string fa_plus: "\uf067"
    readonly property string fa_minus: "\uf068"
    readonly property string fa_ellipsis_h: "\uf141"
    readonly property string fa_pen_to_square: "\uf044"
    readonly property string fa_gear: "\uf013"
    readonly property string fa_sliders: "\uf1de"
    readonly property string fa_arrow_left_to_line: "\uf33e"
    readonly property string fa_arrow_right_to_line: "\uf340"
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
    readonly property string fa_face_sad_tear: "\uf5b4"
    readonly property string fa_star: "\uf005"
    readonly property string fa_link: "\uf0c1"
    readonly property string fa_list_ul: "\uf0ca"
    readonly property string fa_square_check: "\uf14a"
    readonly property string fa_folder: "\uf07b"
    readonly property string fa_tag: "\uf02b"
    readonly property string fa_comments: "\uf086"
    readonly property string fa_box_archive: "\uf187"
    readonly property string fa_paper_plane: "\uf1d8"
}
