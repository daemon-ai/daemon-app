// SPDX-FileCopyrightText: 2026 Jarrad Hope
// SPDX-License-Identifier: MPL-2.0
//
// Thin SHA-256 façade over the vendored implementation. Isolated in its own TU
// so the vendored sha256.h (which typedefs BYTE/WORD) is never pulled into the
// same translation unit as <windows.h> (whose windef.h defines conflicting
// BYTE/WORD). The apply engine only ever sees this clean C++ signature.

#ifndef DAEMON_UPDATER_DIGEST_H
#define DAEMON_UPDATER_DIGEST_H

#include <string>

namespace daemon_updater {

// Compute the lowercase-hex SHA-256 of a regular file. Returns false if the
// file cannot be read; hexOut is only written on success.
bool sha256File(const std::string& path, std::string& hexOut);

} // namespace daemon_updater

#endif // DAEMON_UPDATER_DIGEST_H
