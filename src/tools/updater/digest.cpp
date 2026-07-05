// SPDX-FileCopyrightText: 2026 Jarrad Hope
// SPDX-License-Identifier: MPL-2.0

#include "digest.h"

#include <array>
#include <cstddef>
#include <fstream>
#include <ios>
#include <string_view>
#include <vector>

extern "C" {
#include <sha256.h>
}

namespace {

constexpr std::size_t kReadChunk = 1U << 16U; // 64 KiB
constexpr std::string_view kHexDigits = "0123456789abcdef";

std::string toLowerHex(const std::array<unsigned char, SHA256_BLOCK_SIZE>& digest) {
    std::string out;
    out.resize(static_cast<std::size_t>(SHA256_BLOCK_SIZE) * 2);
    for (std::size_t i = 0; i < static_cast<std::size_t>(SHA256_BLOCK_SIZE); ++i) {
        out[(i * 2)] = kHexDigits[digest[i] >> 4U];
        out[(i * 2) + 1] = kHexDigits[digest[i] & 0x0FU];
    }
    return out;
}

} // namespace

namespace daemon_updater {

bool sha256File(const std::string& path, std::string& hexOut) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return false;
    }
    SHA256_CTX ctx;
    sha256_init(&ctx);
    std::vector<char> buf(kReadChunk);
    while (in) {
        in.read(buf.data(), static_cast<std::streamsize>(buf.size()));
        const std::streamsize got = in.gcount();
        if (got > 0) {
            sha256_update(&ctx, reinterpret_cast<const BYTE*>(buf.data()),
                          static_cast<std::size_t>(got));
        }
    }
    if (in.bad()) {
        return false;
    }
    std::array<unsigned char, SHA256_BLOCK_SIZE> digest{};
    sha256_final(&ctx, digest.data());
    hexOut = toLowerHex(digest);
    return true;
}

} // namespace daemon_updater
