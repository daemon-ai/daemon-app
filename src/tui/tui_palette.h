#pragma once

#include <Tui/ZPalette.h>

// A dark Tui palette approximating the GUI's Dark theme tokens (Theme.qml). This
// demonstrates the Theme.qml -> ZPalette mapping the feasibility study calls out:
// the GUI's hex color tokens become named terminal palette entries. Only the
// handful of roles the spike actually surfaces are overridden; everything else
// falls back to the built-in classic theme.
Tui::ZPalette daemonDarkPalette();
