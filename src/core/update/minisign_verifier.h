// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QByteArray>
#include <QString>

namespace update {

// Detached-signature verification for the release feed, implementing the real
// minisign format (Ed25519, prehashed variant) against a pinned public key.
//
// Self-contained on purpose: the Windows artifact ships no OpenSSL (Schannel
// only) and the Linux static build resolves TLS via a runtime dlopen, so the
// crypto cannot lean on the platform TLS stack. It rides the vendored Monocypher
// (BLAKE2b-512 prehash + standard Ed25519), which builds identically everywhere.
//
// The pinned key is the compile-time trust anchor (packaging/UPDATES.md): TOFU is
// rejected, so a manifest that does not verify against the built-in pin is
// discarded with no "trust this key?" prompt.
class MinisignVerifier {
public:
    struct Result {
        bool ok = false;
        // Human-readable failure reason (empty on success). Never leaks key bytes.
        QString error;
        // The authenticated trusted comment (empty unless ok). The feed client
        // enforces it carries "daemon <channel> <version>" matching the parsed
        // manifest, which is what blocks a cross-channel / downgrade replay of an
        // otherwise-valid signature onto a different manifest.
        QString trustedComment;
    };

    // Verify `signature` (the raw bytes of a `.minisig` file) over `message` (the
    // exact manifest bytes) against `pinnedPublicKeyBase64` (a minisign public key
    // string, e.g. "RWR...").
    //
    // Enforces: the pinned key parses; the signature is the prehashed "ED"
    // algorithm (legacy pure "Ed" is rejected); the signature's key id matches the
    // pinned key; the Ed25519 signature over BLAKE2b-512(message) verifies; and the
    // global signature over (signature || trusted_comment) verifies (authenticating
    // the trusted comment). Fails closed on any parse or verification error.
    [[nodiscard]] static Result verify(const QByteArray& message, const QByteArray& signature,
                                       const QString& pinnedPublicKeyBase64);
};

} // namespace update
