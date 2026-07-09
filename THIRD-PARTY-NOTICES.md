<!--
SPDX-FileCopyrightText: 2026 Jarrad Hope
SPDX-License-Identifier: MPL-2.0
-->

# Third-Party Notices - daemon-app

`daemon-app` itself is licensed under the Mozilla Public License 2.0 (see
[`LICENSE`](LICENSE)). The MPL files are **not** marked "Incompatible With
Secondary Licenses" (MPL Exhibit B is intentionally omitted), so they may be
combined with GPL/LGPL components.

This document lists the third-party software that the GUI (`daemon-app`) and TUI
(`daemon-tui`) link against or vendor. Components are pinned in
[`flake.nix`](flake.nix) (fetched at build time) or committed in-tree under
`src/.../vendor/` and `src/.../generated/`. Binary distributions of the clients
must carry the corresponding attributions and license texts.

## Build-time dependencies (pinned in flake.nix)

| Component | Pinned rev | License (SPDX) | Copyright | Upstream |
| --- | --- | --- | --- | --- |
| Qt 6 (qtbase, qtdeclarative, qtshadertools, qtsvg, qtwebsockets, qttools) | nixpkgs (3) | LGPL-3.0-only (3) | The Qt Company Ltd. and contributors | qt.io |
| md4qt | `345bba2` | MIT | Igor Mironchik | invent.kde.org/libraries/md4qt |
| earcut.hpp | `64530ea` | ISC | (c) 2015 Mapbox | github.com/mapbox/earcut.hpp |
| KSyntaxHighlighting | `ccb31f7` | MIT (1) | The KDE community | invent.kde.org/frameworks/syntax-highlighting |
| MicroTeX | `0e3707f` | MIT | (c) 2020 Nano Michael and contributors | github.com/NanoMichael/MicroTeX |
| tinyxml2 | nixpkgs | Zlib | (c) Lee Thomason | github.com/leethomason/tinyxml2 |
| QSimpleUpdater | `6c78600` | MIT | (c) Alex Spataru | github.com/alex-spataru/QSimpleUpdater |
| QAutoStart | `a27a028` | BSD-3-Clause | (c) Skycoder42 | github.com/Skycoder42/QAutoStart |
| QWindowKit | `c783b89` | Apache-2.0 | (c) 2023-2025 Stdware Collections; (c) 2021-2023 wangwenx190 (Yuhang Zhao) | github.com/stdware/qwindowkit |
| QxtGlobalShortcut | `0e47e8c` | BSD-3-Clause | (c) 2006-2011 the LibQxt project | github.com/hluk/qxtglobalshortcut |
| **qmltermwidget** | `master` | **GPL-2.0-only** (2) | Filippo Scognamiglio; qtermwidget / Konsole contributors | github.com/Swordfish90/qmltermwidget |
| Tui Widgets | `31ccc7a` | BSL-1.0 | (c) Martin Hostettler | github.com/tuiwidgets/tuiwidgets |
| PosixSignalManager | `6fa40e4` | BSL-1.0 | (c) Martin Hostettler | github.com/textshell/posixsignalmanager |
| termpaint | nixpkgs | BSL-1.0 | (c) Martin Hostettler | github.com/textshell/termpaint |

(1) The KSyntaxHighlighting library code is MIT. The bundled syntax-definition
and theme files it ships carry their own per-file licenses (MIT, LGPL, GPL, and
others); see the upstream `LICENSES/` directory and each definition's header.

(2) See the **GPL-2.0 obligation** section below.

(3) Qt is used under **LGPL-3.0-only** (Qt is dual-licensed; nothing here uses
the commercial license, and no GPL-only Qt module is linked). The dynamic
desktop builds link the shared nixpkgs Qt, which satisfies LGPL §4(d) by
ordinary dynamic linking (the user can replace the library). The **portable
static build** links Qt statically and has its own obligations - see the
**Statically linked portable build (LGPL-3.0)** section below. The wasm and
portable builds compile the same Qt version from the upstream source tarballs
pinned by nixpkgs.

## In-tree vendored / generated code

| Path | License (SPDX) | Copyright | Notes |
| --- | --- | --- | --- |
| `src/core/daemon/codec/vendor/` | Apache-2.0 | (c) 2020 Nordic Semiconductor ASA | zcbor 0.9.1 C runtime; files carry upstream SPDX + copyright headers. |
| `src/core/daemon/codec/generated/` | Apache-2.0 | (c) 2026 Jarrad Hope | zcbor codegen output from the first-party `daemon-api` CDDL contract; drift-checked, declared via REUSE.toml. |
| `src/tui/vendor/Tui/` | BSL-1.0 | (c) 2020 Martin Hostettler | Tui Widgets private headers, vendored verbatim; copyright declared via REUSE.toml. |
| `src/tools/updater/vendor/` | LicenseRef-PublicDomain | Brad Conte (brad AT bradconte.com) | SHA-256 (`sha256.c`/`sha256.h`) from github.com/B-Con/crypto-algorithms, released into the public domain; used by the `daemon-updater` helper to re-verify a staged artifact's digest before the atomic swap. Vendored near-verbatim (one `#include` swapped for the standard `<string.h>` for portability; see `vendor/LICENSE`). Not a side-channel-resistant implementation — used only for local integrity, never for secrecy. |
| `src/core/update/vendor/monocypher/` | BSD-2-Clause OR CC0-1.0 | (c) 2017-2019 Loup Vaillant | Monocypher 4.0.2 (BLAKE2b-512 + Ed25519); the self-contained minisign crypto for the updater, so signature verification needs no platform TLS. Files carry upstream SPDX headers; `LICENCE.md` ships alongside. |

## Bundled fonts (assets/fonts/)

The following fonts are shipped as Qt resources. They are third-party and retain
their own licenses (not MPL-2.0):

| Font file(s) | License (SPDX) | Copyright |
| --- | --- | --- |
| `InterVariable.ttf` | OFL-1.1 | The Inter Project Authors |
| `IbarraRealNova.ttf`, `IbarraRealNova-Italic.ttf` | OFL-1.1 | The Ibarra Real Nova Project Authors |
| `Trykker-Regular.ttf` | OFL-1.1 | The Trykker Project Authors |
| `iAWriterDuoS-*`, `iAWriterMonoS-*`, `iAWriterQuattroS-*` | OFL-1.1 | iA Inc. |
| `fa-brands-400.ttf`, `fa-solid-900.ttf` (Font Awesome Free) | OFL-1.1 | Fonticons, Inc. |
| `material-symbols-outlined.ttf` | Apache-2.0 | Google LLC |

The SIL Open Font License text is at [`LICENSES/OFL-1.1.txt`](LICENSES/OFL-1.1.txt).
OFL requires that the copyright notice and license accompany the font files and
that the fonts not be sold by themselves; Reserved Font Names must be respected.

## Statically linked portable build (LGPL-3.0)

The `portable` package (`daemon-app-portable-x86_64.tar.zst`) links Qt
**statically**. Qt's LGPL-3.0 permits this only when the recipient can relink
the application against a modified Qt (LGPL-3.0 §4(d)(0), via GPLv3 §6's
"Corresponding Source" mechanism, or §4(d)(1) object-file provision). This
distribution satisfies that through source availability:

- The complete corresponding source of the application (this repository, MPL-2.0)
  and the exact Qt configuration that built the artifact (`nix/qt-from-source.nix`
  + `nix/portable.nix`, pinning the upstream Qt 6 source tarballs) are published
  alongside every release. Rebuilding the artifact with a modified Qt is
  `nix build .#portable` after editing that configuration - no proprietary
  ingredient is required for a relink.
- This notices file ships inside the portable package
  (`share/doc/daemon-app/THIRD-PARTY-NOTICES.md`) so the artifact itself
  documents the obligation and points at the sources.
- The LGPL-3.0 text is at `LICENSES/LGPL-3.0-only.txt` in the Qt source
  tarball and at <https://www.gnu.org/licenses/lgpl-3.0.html>.

What the static artifact contains, beyond the dynamic builds' set:

- Qt's **bundled third-party copies** compiled into the binary: pcre2 (BSD-3-Clause),
  harfbuzz (MIT), libpng (Libpng), libjpeg-turbo (IJG/BSD-3-Clause/Zlib), zlib
  (Zlib), double-conversion (BSD-3-Clause), sqlite (public domain), md4c (MIT),
  and the Wayland client protocol definitions (MIT/HPND). Their notices ship in
  the Qt source tarball's `LICENSES/` directory and the respective
  `src/3rdparty/*` subtrees.
- freetype and fontconfig are **not** bundled: the portable binary links the
  host system's copies dynamically (like glibc, the X11/xcb/wayland client
  stack, libxkbcommon, and GL/EGL).

The POSIX static desktop build (Linux portable / AppImage / DEB / RPM, and the
static macOS .app) is feature-complete: it **includes** the GPL-2.0 qmltermwidget
embedded terminal (statically compiled in - so the GPL-2.0 obligation below DOES
attach to these artifacts) and a real OS keychain via **qtkeychain**
(BSD-3-Clause, built static; Linux uses the D-Bus Secret Service backend, macOS
uses Security.framework - no license obligation). Only the WASM, Windows, and
mobile (Android/iOS) builds compile the stub terminal panel + QSettings token
store (no Unix PTY / no OS secret store on those targets).

## Rust daemon shipped alongside (daemon-node)

Packaged distributions that bundle the Rust daemon next to the clients ship
`daemon-node`, which is first-party and dual-licensed **MIT OR Apache-2.0**
((c) 2026 Jarrad Hope; see `LICENSE-MIT` / `LICENSE-APACHE` in the daemon-node
repository). Its own third-party crate attributions are documented in the
daemon-node repository (`cargo deny` gates the license set).

## GPL-2.0 obligation (qmltermwidget)

The embedded-terminal feature links **qmltermwidget**, which is licensed under
**GPL-2.0-only**. When the GUI is distributed as a combined work that includes
this plugin, the combined binary is effectively subject to GPL-2.0: recipients
must be offered the complete corresponding source under GPL-2.0 terms, and the
GPL-2.0 text (<https://www.gnu.org/licenses/old-licenses/gpl-2.0.html>; shipped
with the qmltermwidget source carried in the build artifact) must accompany the
distribution. This applies to the dynamic desktop builds AND the POSIX static
desktop builds (Linux portable/AppImage/DEB/RPM + static macOS), which now
statically compile qmltermwidget in. Only the WASM, Windows, and mobile
(Android/iOS) builds compile the stub terminal panel and therefore do **not**
carry this obligation.

This is compatible with `daemon-app`'s MPL-2.0 licensing because MPL-2.0 §3.3
permits distributing a Larger Work that combines MPL "Covered Software" with a
work under a Secondary License (GPL family), provided the MPL files are not
marked Incompatible With Secondary Licenses (they are not). MPL-covered files
remain under MPL; the combined-work distribution additionally satisfies GPL-2.0.

If GPL-2.0 obligations are undesirable for a particular distribution, build the
clients without the embedded terminal, or ship qmltermwidget as a separately
distributed, independently-installed plugin.

## Apache-2.0 attribution

QWindowKit and the vendored zcbor runtime are Apache-2.0. Per Apache-2.0 §4(d),
their attribution notices (reproduced above) must be carried in distributions
that include them.
