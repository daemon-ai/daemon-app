// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "update/minisign_verifier.h"

#include <array>
#include <QByteArrayView>
#include <QList>

// Vendored Monocypher (src/core/update/vendor/monocypher), included as a SYSTEM
// header so the analyzers skip it. Provides BLAKE2b-512 (crypto_blake2b) and
// standard Ed25519 (crypto_ed25519_check) - exactly the minisign primitives.
#include <monocypher-ed25519.h>
#include <monocypher.h>

namespace update {

namespace {

// A minisign public key decodes to: 2-byte signature algorithm ("Ed") + 8-byte
// key id + 32-byte Ed25519 public key.
constexpr int kKeyIdLen = 8;
constexpr int kEd25519PkLen = 32;
constexpr int kEd25519SigLen = 64;
constexpr int kBlake2bLen = 64;
constexpr int kPubKeyBlobLen = 2 + kKeyIdLen + kEd25519PkLen; // 42
constexpr int kSigBlobLen = 2 + kKeyIdLen + kEd25519SigLen;   // 74
constexpr int kGlobalSigBlobLen = kEd25519SigLen;             // 64

MinisignVerifier::Result fail(const QString& reason) {
    return {false, reason, QString()};
}

// Strict base64 decode: rejects anything that is not canonical base64 (minisign
// blobs are fixed-length, so a lenient decode would mask corruption).
bool decodeBase64(const QByteArray& in, QByteArray& out) {
    const QByteArray::FromBase64Result r = QByteArray::fromBase64Encoding(
        in, QByteArray::Base64Encoding | QByteArray::AbortOnBase64DecodingErrors);
    if (r.decodingStatus != QByteArray::Base64DecodingStatus::Ok) {
        return false;
    }
    out = *r;
    return true;
}

} // namespace

MinisignVerifier::Result MinisignVerifier::verify(const QByteArray& message,
                                                  const QByteArray& signature,
                                                  const QString& pinnedPublicKeyBase64) {
    // --- pinned public key -----------------------------------------------------
    QByteArray pubBlob;
    if (!decodeBase64(pinnedPublicKeyBase64.trimmed().toLatin1(), pubBlob) ||
        pubBlob.size() != kPubKeyBlobLen) {
        return fail(QStringLiteral("pinned public key is malformed"));
    }
    // Bytes 0..1 are the algorithm ("Ed"); 2..9 the key id; 10..41 the Ed25519 pk.
    const QByteArray pubKeyId = pubBlob.mid(2, kKeyIdLen);
    std::array<uint8_t, kEd25519PkLen> pk{};
    for (int i = 0; i < kEd25519PkLen; ++i) {
        pk[static_cast<size_t>(i)] = static_cast<uint8_t>(pubBlob.at(2 + kKeyIdLen + i));
    }

    // --- .minisig: 4 lines (untrusted comment, sig, trusted comment, global sig) -
    // Split on '\n' and tolerate a trailing '\r' (CRLF) / trailing blank lines.
    QList<QByteArray> lines = signature.split('\n');
    for (QByteArray& line : lines) {
        if (line.endsWith('\r')) {
            line.chop(1);
        }
    }
    while (!lines.isEmpty() && lines.last().isEmpty()) {
        lines.removeLast();
    }
    if (lines.size() < 4) {
        return fail(QStringLiteral("signature file is truncated"));
    }

    const QByteArray trustedCommentPrefix = QByteArrayLiteral("trusted comment: ");
    if (!lines.at(2).startsWith(trustedCommentPrefix)) {
        return fail(QStringLiteral("signature file missing trusted comment"));
    }
    const QByteArray trustedCommentBytes = lines.at(2).mid(trustedCommentPrefix.size());

    // --- signature blob: algorithm + key id + 64-byte signature ----------------
    QByteArray sigBlob;
    if (!decodeBase64(lines.at(1), sigBlob) || sigBlob.size() != kSigBlobLen) {
        return fail(QStringLiteral("signature line is malformed or truncated"));
    }
    // Require the prehashed algorithm "ED" (0x45 0x44); reject legacy pure "Ed".
    if (sigBlob.at(0) != 'E' || sigBlob.at(1) != 'D') {
        return fail(QStringLiteral("unsupported signature algorithm (require prehashed ED)"));
    }
    if (sigBlob.mid(2, kKeyIdLen) != pubKeyId) {
        return fail(QStringLiteral("signature key id does not match the pinned key"));
    }
    const QByteArray sig = sigBlob.mid(2 + kKeyIdLen, kEd25519SigLen);

    // --- global signature blob (64 raw signature bytes) ------------------------
    QByteArray globalSig;
    if (!decodeBase64(lines.at(3), globalSig) || globalSig.size() != kGlobalSigBlobLen) {
        return fail(QStringLiteral("global signature is malformed or truncated"));
    }

    // --- verify the file signature over BLAKE2b-512(message) -------------------
    std::array<uint8_t, kBlake2bLen> digest{};
    crypto_blake2b(digest.data(), digest.size(),
                   reinterpret_cast<const uint8_t*>(message.constData()),
                   static_cast<size_t>(message.size()));
    if (crypto_ed25519_check(reinterpret_cast<const uint8_t*>(sig.constData()), pk.data(),
                             digest.data(), digest.size()) != 0) {
        return fail(QStringLiteral("manifest signature does not verify"));
    }

    // --- verify the global signature over (signature || trusted_comment) -------
    // This authenticates the trusted comment: it cannot be swapped independently
    // of the file signature it is bound to.
    QByteArray globalMessage;
    globalMessage.reserve(sig.size() + trustedCommentBytes.size());
    globalMessage.append(sig);
    globalMessage.append(trustedCommentBytes);
    if (crypto_ed25519_check(reinterpret_cast<const uint8_t*>(globalSig.constData()), pk.data(),
                             reinterpret_cast<const uint8_t*>(globalMessage.constData()),
                             static_cast<size_t>(globalMessage.size())) != 0) {
        return fail(QStringLiteral("trusted comment signature does not verify"));
    }

    return {true, QString(), QString::fromUtf8(trustedCommentBytes)};
}

} // namespace update
