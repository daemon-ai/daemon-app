// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once
// The msg-fence transcript projection over mirror `TranscriptBlock` window items (seq order),
// ported grammar-for-grammar from the legacy `CachedSessionStore::content()` and enriched by G2
// (args_summary + detail_kind/detail_body). Factored out of MirrorSessionStore so the ONE
// projection grammar has a single home: `MirrorSessionStore::content()` reads it, and the mock
// seeder derives the legacy delegate's baseline content from the SAME blocks through it — so the
// two sides are byte-equal by construction (A8's ONE-bundle invariant), never a duplicated fold.

#include "mirror/generated/entities_gen.h"

#include <QString>
#include <vector>

namespace daemonapp::daemon {

// Fold a seq-ordered TranscriptBlock stream into the msg-fence markdown the transcript surfaces
// render (tool fences fold their result; the `todo` tool's blocks are suppressed like the engine's
// live composer feed). Trimmed; empty input yields an empty string.
[[nodiscard]] QString projectTranscriptBlocks(const std::vector<mirror::TranscriptBlock>& blocks);

} // namespace daemonapp::daemon
