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
| md4qt | `345bba2` | MIT | Igor Mironchik | invent.kde.org/libraries/md4qt |
| earcut.hpp | `64530ea` | ISC | (c) 2015 Mapbox | github.com/mapbox/earcut.hpp |
| KSyntaxHighlighting | `ccb31f7` | MIT (1) | The KDE community | invent.kde.org/frameworks/syntax-highlighting |
| MicroTeX | `0e3707f` | MIT | NanoMichael and contributors | github.com/NanoMichael/MicroTeX |
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

## In-tree vendored / generated code

| Path | License (SPDX) | Copyright | Notes |
| --- | --- | --- | --- |
| `src/core/daemon/codec/vendor/` | Apache-2.0 | (c) 2020 Nordic Semiconductor ASA | zcbor 0.9.1 C runtime; files carry upstream SPDX + copyright headers. |
| `src/core/daemon/codec/generated/` | Apache-2.0 | (c) 2026 Jarrad Hope | zcbor codegen output from the first-party `daemon-api` CDDL contract; drift-checked, declared via REUSE.toml. |
| `src/tui/vendor/Tui/` | BSL-1.0 | (c) 2020 Martin Hostettler | Tui Widgets private headers, vendored verbatim; copyright declared via REUSE.toml. |

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

## GPL-2.0 obligation (qmltermwidget)

The embedded-terminal feature links **qmltermwidget**, which is licensed under
**GPL-2.0-only**. When the GUI is distributed as a combined work that includes
this plugin, the combined binary is effectively subject to GPL-2.0: recipients
must be offered the complete corresponding source under GPL-2.0 terms, and the
GPL-2.0 text (<https://www.gnu.org/licenses/old-licenses/gpl-2.0.html>; shipped
with the qmltermwidget source carried in the build artifact) must accompany the
distribution.

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
