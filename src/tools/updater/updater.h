// SPDX-FileCopyrightText: 2026 Jarrad Hope
// SPDX-License-Identifier: MPL-2.0
//
// daemon-updater: a tiny standalone self-update apply helper. The app stages +
// verifies a new artifact, spawns this helper with a spec, and quits; the helper
// waits for the app to exit, atomically applies the update, and relaunches. It is
// the only process that mutates the installed application — the app never
// self-mutates. See packaging/UPDATES.md for the design and this directory's
// README/CMakeLists for the frozen CLI contract.

#ifndef DAEMON_UPDATER_UPDATER_H
#define DAEMON_UPDATER_UPDATER_H

namespace daemon_updater {

// Parse argv, apply the requested mode, and return one of the contract exit
// codes (0 applied, 2 verify failed, 3 apply failed + rolled back, 4 apply
// failed + inconsistent, 5 bad args / wait-pid timeout). Never throws.
int run(int argc, char** argv) noexcept;

} // namespace daemon_updater

#endif // DAEMON_UPDATER_UPDATER_H
