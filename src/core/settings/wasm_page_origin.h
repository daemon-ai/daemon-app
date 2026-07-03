// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QUrl>

namespace settings {

// The URL of the page hosting the wasm instance (window.location.href), query string included -
// the input deriveWsDefault() resolves the browser connection target from. Compiled only under
// EMSCRIPTEN (wasm_page_origin.cpp); desktop builds never reference it.
[[nodiscard]] QUrl wasmPageUrl();

} // namespace settings
