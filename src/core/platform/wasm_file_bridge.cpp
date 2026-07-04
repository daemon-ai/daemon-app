// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "platform/wasm_file_bridge.h"

#include <utility>

// Compiled on every platform so shared call sites link against a definition; no-op off wasm (the
// desktop file flows keep using QtQuick.Dialogs FileDialog + QFile, and copy uses QClipboard).
#ifdef __EMSCRIPTEN__

#include <cstddef>
#include <cstdint>
#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <map>
#include <QByteArray>
#include <QCoreApplication>
#include <QString>
#include <string>

namespace platform {
namespace {

// Pending open-picker callbacks, keyed by a monotonic id. The id (an int) is the only thing that
// crosses into JS; the browser's change/FileReader handlers call back into the embind functions
// below with that id, so no C++ pointer is ever exposed to JS.
using OpenCallback = std::function<void(const QString&, const QByteArray&)>;

std::map<int, OpenCallback>& pendingOpens() {
    static std::map<int, OpenCallback> map;
    return map;
}

int nextOpenId() {
    static int counter = 0;
    return ++counter;
}

// Copy a JS Uint8Array into a QByteArray without any manual heap juggling: a typed_memory_view
// aliases the QByteArray's storage, and Uint8Array.prototype.set copies the JS bytes into it.
QByteArray toByteArray(const emscripten::val& uint8Array) {
    const auto length = uint8Array["length"].as<unsigned>();
    QByteArray bytes(static_cast<qsizetype>(length), Qt::Uninitialized);
    if (length > 0) {
        emscripten::val view = emscripten::val(emscripten::typed_memory_view(
            static_cast<std::size_t>(length), reinterpret_cast<std::uint8_t*>(bytes.data())));
        view.call<void>("set", uint8Array);
    }
    return bytes;
}

// embind entry points invoked from the picker JS (registered on Module below). They deliver the
// result onto a fresh Qt event-loop turn so `cb` never runs re-entrant from inside the browser
// change/load event (mirrors the "queued signal" contract).
void fileOpenResult(int id, const std::string& name, const emscripten::val& data) {
    const auto it = pendingOpens().find(id);
    if (it == pendingOpens().end()) {
        return;
    }
    OpenCallback cb = std::move(it->second);
    pendingOpens().erase(it);
    const QString fileName = QString::fromStdString(name);
    const QByteArray bytes = toByteArray(data);
    QMetaObject::invokeMethod(
        QCoreApplication::instance(),
        [cb = std::move(cb), fileName, bytes]() { cb(fileName, bytes); }, Qt::QueuedConnection);
}

void fileOpenCancel(int id) {
    pendingOpens().erase(id);
}

} // namespace

void openFileContent(const QString& acceptFilter, OpenCallback cb) {
    if (!cb) {
        return;
    }
    const int id = nextOpenId();
    pendingOpens().emplace(id, std::move(cb));

    // Stash the accept filter as a JS string so EM_ASM needs no pointer/UTF8 decoding (keeps the
    // inline JS free of runtime-method dependencies that may be tree-shaken).
    emscripten::val::global("window").set("__daemonapp_pending_accept",
                                          emscripten::val(acceptFilter.toStdString()));
    // <input type=file> + FileReader. `Module.daemonapp_fileOpen{Result,Cancel}` are the embind
    // functions registered below; they take the JS string + Uint8Array directly (no heap copy).
    // clang-format off
    EM_ASM({
        var id = $0;
        var accept = window.__daemonapp_pending_accept || "";
        delete window.__daemonapp_pending_accept;
        var input = document.createElement('input');
        input.type = 'file';
        if (accept) { input.accept = accept; }
        input.style.position = 'fixed';
        input.style.left = '-10000px';
        input.style.opacity = '0';
        document.body.appendChild(input);
        var removeInput = function() { if (input.parentNode) { input.parentNode.removeChild(input); } };
        input.addEventListener('change', function() {
            if (!input.files || input.files.length === 0) {
                removeInput();
                Module.daemonapp_fileOpenCancel(id);
                return;
            }
            var file = input.files[0];
            var reader = new FileReader();
            reader.onload = function() {
                removeInput();
                Module.daemonapp_fileOpenResult(id, file.name, new Uint8Array(reader.result));
            };
            reader.onerror = function() {
                removeInput();
                Module.daemonapp_fileOpenCancel(id);
            };
            reader.readAsArrayBuffer(file);
        }, { once: true });
        input.click();
    }, id);
    // clang-format on
}

void saveFileContent(const QByteArray& bytes, const QString& suggestedName) {
    emscripten::val Uint8Array = emscripten::val::global("Uint8Array");
    // The typed_memory_view aliases the wasm heap; wrap it in a fresh Uint8Array so the Blob owns a
    // standalone copy that outlives this call.
    emscripten::val heapView = emscripten::val(
        emscripten::typed_memory_view(static_cast<std::size_t>(bytes.size()),
                                      reinterpret_cast<const std::uint8_t*>(bytes.constData())));
    emscripten::val copy = Uint8Array.new_(heapView);

    emscripten::val parts = emscripten::val::array();
    parts.call<void>("push", copy);
    emscripten::val blob = emscripten::val::global("Blob").new_(parts);

    emscripten::val url = emscripten::val::global("URL");
    emscripten::val objectUrl = url.call<emscripten::val>("createObjectURL", blob);

    emscripten::val document = emscripten::val::global("document");
    emscripten::val anchor = document.call<emscripten::val>("createElement", std::string("a"));
    anchor.set("href", objectUrl);
    anchor.set("download", suggestedName.toStdString());
    anchor["style"].set("display", "none");
    document["body"].call<void>("appendChild", anchor);
    anchor.call<void>("click");
    document["body"].call<void>("removeChild", anchor);
    url.call<void>("revokeObjectURL", objectUrl);
}

bool writeClipboardText(const QString& text) {
    emscripten::val::global("window").set("__daemonapp_clip_text",
                                          emscripten::val(text.toStdString()));
    // clang-format off
    const int ok = EM_ASM_INT({
        var text = window.__daemonapp_clip_text || "";
        delete window.__daemonapp_clip_text;
        try {
            if (navigator.clipboard && window.isSecureContext) {
                // Fire-and-forget: the async write rides the current user gesture. We report
                // success optimistically (a rejected promise can't unwind synchronously here).
                navigator.clipboard.writeText(text);
                return 1;
            }
        } catch (e) { /* fall through to the legacy path */ }
        try {
            var ta = document.createElement('textarea');
            ta.value = text;
            ta.setAttribute('readonly', '');
            ta.style.position = 'fixed';
            ta.style.left = '-10000px';
            ta.style.opacity = '0';
            document.body.appendChild(ta);
            ta.focus();
            ta.select();
            var res = document.execCommand('copy');
            document.body.removeChild(ta);
            return res ? 1 : 0;
        } catch (e) { return 0; }
    });
    // clang-format on
    return ok != 0;
}

// clang-format off
EMSCRIPTEN_BINDINGS(daemonapp_file_bridge) {
    emscripten::function("daemonapp_fileOpenResult", &fileOpenResult);
    emscripten::function("daemonapp_fileOpenCancel", &fileOpenCancel);
}
// clang-format on

} // namespace platform

#else // desktop / mobile: real filesystem via FileDialog + QFile; this bridge is unused.

namespace platform {

void openFileContent(const QString& /*acceptFilter*/,
                     std::function<void(const QString& name, const QByteArray& bytes)> cb) {
    // No browser picker off wasm; drop the callback. Move-from keeps the by-value sink signature
    // (the wasm impl stores it) clean of performance-unnecessary-value-param.
    [[maybe_unused]] const auto dropped = std::move(cb);
}

void saveFileContent(const QByteArray& /*bytes*/, const QString& /*suggestedName*/) {}

bool writeClipboardText(const QString& /*text*/) {
    return false;
}

} // namespace platform

#endif // __EMSCRIPTEN__
