// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QIcon>

namespace platform {

// The branded application icon (window/taskbar chrome + system tray).
//
// Prefers the installed hicolor theme icon ("daemon-app") so a Linux desktop's
// themed/HiDPI variant wins, then falls back to the multi-size PNGs embedded in
// the app resource (":/icons/daemon-app-<N>.png", added by the App target on
// desktop). Returns a null QIcon where neither source is present (mobile / the
// browser, which brand through the launcher icon / favicon instead) - assigning
// a null icon is harmless.
QIcon appIcon();

} // namespace platform
