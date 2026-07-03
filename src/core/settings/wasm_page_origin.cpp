// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "settings/wasm_page_origin.h"

// CMake compiles this TU only under EMSCRIPTEN; the guard additionally keeps the desktop
// clang-tidy sweep (which visits every tracked src/*.cpp) off the emscripten headers.
#ifdef __EMSCRIPTEN__

#include <emscripten/val.h>
#include <string>

namespace settings {

QUrl wasmPageUrl() {
    // window.location always exists in the browsing contexts Qt-for-wasm runs in; the guard
    // covers exotic embeddings (e.g. a worker) where the global is absent.
    const emscripten::val location = emscripten::val::global("window")["location"];
    if (location.isUndefined() || location.isNull()) {
        return {};
    }
    return QUrl(QString::fromStdString(location["href"].as<std::string>()));
}

} // namespace settings

#endif // __EMSCRIPTEN__
