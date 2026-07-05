# Release feed & auto-update

Status: **implemented.** The signed `manifest.json` feed, the pinned-key
minisign verifier, the SemVer-gated feed client, the verified downloader, the
GUI+TUI Notify surface, and the `SelfApply` self-update path (AppImage, Windows
NSIS, macOS DMG) all ship. `deb`/`rpm`/`portable-tar` stay `Notify` and `apk`/
`wasm` stay `None` by design. Still future work: notarization, zsync delta
transport, and deb/rpm repo channels — flagged as such throughout and in
[Implemented vs. future](#implemented-vs-future).

The moving parts:

- feed generator + signer — `scripts/release-manifest.sh` (+ the legacy mirror
  `scripts/sync-updates-json.sh`);
- client — `src/core/update/` (`UpdateManager`, `FeedClient`,
  `MinisignVerifier`, `semver`, `UpdateManifest`, `UpdateDownloader`, and the
  three `self_apply_*` backends);
- apply helper — `src/tools/updater/` (`daemon-updater`, no-Qt C++17);
- per-platform packaging + validation — `nix/portable.nix`, `nix/windows.nix`,
  `flake.nix` `mkDarwinArtifacts`, `packaging/windows/UPDATE-VALIDATION.md`,
  `packaging/macos/README.md`.

## The capability dial

Update capability is a **per-artifact dial fixed at package time** — a compile
definition (`DAEMON_APP_UPDATE_CAPABILITY`, default `None`) baked into the
binary, not a runtime setting. Each step strictly contains the previous one:

| Dial | The app may... |
|---|---|
| `None` | nothing — no feed fetch, no UI. The platform owns updates. |
| `Notify` | fetch + verify the feed, tell the user a newer version exists, link the notes. |
| `DownloadAndOpen` | additionally download the artifact, verify size + sha256, and hand it to the OS (open installer / reveal file). The **user** completes the install. |
| `SelfApply` | additionally replace itself and relaunch via the `daemon-updater` helper, with a `DownloadAndOpen` fallback on any precondition failure. |

Two dials exist per artifact and they compose:

- **Compiled dial** (`DAEMON_APP_UPDATE_CAPABILITY`): the ceiling, set per
  package job; nothing at runtime can raise it.
- **Feed dial** (`updateCapability` in `manifest.json`): advisory per release.
- **Effective capability = min(compiled, feed row)** — computed in
  `UpdateManager::evaluateManifest` (`minCapability`). The feed can therefore
  emergency-lower a fleet (publish `Notify` to disable a misbehaving
  self-apply) but can never escalate a shipped binary. When no artifact row
  matches the host, the effective capability caps at `Notify` (notes link only).

### Current per-artifact dials (what ships today)

| Artifact | Compiled ceiling | Feed row | Effective | Where |
|---|---|---|---|---|
| AppImage | `SelfApply` | `SelfApply` | **`SelfApply`** | `nix/portable.nix` (shared Linux static binary) |
| deb | `SelfApply` (shared binary) | `Notify` | **`Notify`** | feed row from `release-manifest.sh` |
| rpm | `SelfApply` (shared binary) | `Notify` | **`Notify`** | feed row from `release-manifest.sh` |
| portable tar | `SelfApply` (shared binary) | `Notify` | **`Notify`** | feed row from `release-manifest.sh` |
| NSIS `.exe` (Windows) | `SelfApply` | `SelfApply` | **`SelfApply`** | `nix/windows.nix` |
| DMG `.app` (macOS) | `SelfApply` | `SelfApply` | **`SelfApply`** | `flake.nix` `mkDarwinArtifacts` |
| APK (Android, sideload) | `None` | — | **`None`** | dial unset → default `None` (see the TLS note below) |
| WASM / app stores | `None` | (not in feed) | **`None`** | dial unset → default `None`; the browser/store owns delivery |

The **one static-Qt Linux binary** feeds the AppImage, deb, rpm, and portable
tarball, so it compiles a single `SelfApply` ceiling. The compiled artifact
kind is `portable-tar`; the **AppImage case is detected at runtime** via
`$APPIMAGE` (`buildArtifactKind()` in `update_manager.cpp`). Because the feed
rows for deb/rpm/portable-tar are `Notify`, `min()` keeps them notify-only —
they can never self-apply from the shared binary even though its ceiling is
`SelfApply`.

**Android = `None` (not `Notify`), with a TLS reason.** The debug APK's Qt
build ships **no TLS stack** (`nix/android.nix`: `INPUT_openssl` is off; TLS on
Android needs a separately-downloaded bundled `libssl`/`libcrypto` pair — `ws://`
works, `wss://` is a documented caveat). The feed is served over HTTPS from
GitHub, so even a `Notify`-only feed fetch would fail on Android. The dial is
therefore left unset (defaulting to `None`, fully inert): no feed fetch, no UI.
Revisit when Android TLS lands. The Play Store owns in-store updates regardless.

## Verification

Detached-signature verification runs in the **client**, before any parsing,
against a **compile-time pinned public key** (`DAEMON_APP_UPDATE_PUBKEY`).

- **Algorithm: real minisign format over vendored [Monocypher](https://monocypher.org/)**
  (single-file C, CC0/BSD; provides both BLAKE2b-512 prehash and Ed25519). It
  builds identically on Schannel-Windows, static-Linux (TLS via runtime
  `dlopen`), and macOS with zero new system deps — the crypto cannot lean on the
  platform TLS stack. Monocypher is listed in `THIRD-PARTY-NOTICES.md`.
- `MinisignVerifier::verify` enforces, fail-closed on any error:
  1. the pinned key parses;
  2. the signature is the **prehashed `"ED"`** algorithm (legacy pure `"Ed"` is
     rejected);
  3. the signature's **key id matches** the pinned key;
  4. the Ed25519 signature over **BLAKE2b-512(manifest bytes)** verifies;
  5. the **global signature** over `(signature || trusted_comment)` verifies,
     authenticating the trusted comment.
- **Anti-replay via the trusted comment.** The authenticated trusted comment
  carries `daemon <channel> <version>`; `UpdateManager::onFetched` requires it
  to match the parsed manifest's channel and version, so a valid `.minisig`
  cannot be replayed onto a different channel's or version's manifest.
- **TOFU is rejected.** The client never learns a key from the network; a
  manifest that does not verify against the built-in pin is discarded — no
  "trust this new key?" prompt, no first-use grace.
- **HTTPS is required** for the compiled feed as defense in depth, but the
  signature — not the transport — authenticates the feed. `file://` and
  loopback `http://` are additionally allowed for tests (`FeedClient::isAllowedUrl`).

The current release-feed public key (pinned at compile time in every non-inert
package job; the matching secret lives only in the `daemon-ai/daemon`
`MINISIGN_SECRET_KEY` CI secret):

```
RWRXpowS90Fy+TYhRsrBbQNSDvjbtJpqi9T89OGqSNTLkOa5vn62hK0o
```

Key rotation = ship a release (signed with the old key) whose binaries pin the
new key; the old key keeps signing that release's channel until the fleet has
moved.

## Feeds

- **`manifest.json` (+ detached `manifest.json.minisig`) — the real feed.**
  Fetched from
  `https://github.com/daemon-ai/daemon/releases/latest/download/manifest.json`
  (`DAEMON_APP_UPDATE_FEED_URL`). `FeedClient` follows the
  `releases/latest/download/…` 302 to the versioned asset URL and reports the
  **resolved** manifest URL, so artifact `file` fields resolve relative to it
  and a feed relocates (bucket/CDN/mirror) without rewrites. It sends
  `If-None-Match` with the ETag cached in settings and treats a 304 as
  up-to-date (re-surfacing a persisted offered version if still an upgrade);
  it retries a bounded number of times on transient network failure. The client
  transports the two blobs only — `UpdateManager` verifies, then parses.
- **`packaging/UPDATES.json` — legacy/simple mirror** in QSimpleUpdater's
  format, patched from a manifest by `scripts/sync-updates-json.sh` (and
  `just set-version` writes its `latest-version`). It is **unsigned and MUST NOT
  gate any download or apply decision**.

### Feed evaluation (`UpdateManager`)

1. Fetch `manifest.json` + `.minisig` (ETag conditional).
2. **Verify** the detached signature against the pinned key.
3. **Parse** strict schema 1 (`parseManifest`): rejects non-object JSON, an
   unknown major schema, a `product` mismatch, and a `channel` mismatch — each a
   fail-closed discard.
4. **Anti-replay** trusted-comment check (above).
5. **SemVer monotonicity** (`semver::isUpgrade`): offer only
   `manifest.version > currentVersion`. The running build's version carries a
   git build-metadata suffix (`X.Y.Z+<n>.g<hash>[.dirty]`) which is **stripped**
   before comparison (SemVer 2.0.0 §10). A prerelease ranks below its release
   core; a malformed feed version fails closed and never registers as an upgrade.
6. **Artifact selection** (`selectArtifact`): `os` + `arch` + `kind` must match;
   `arch: unknown` is never selected; on Linux an artifact whose `glibcFloor`
   exceeds the host glibc is skipped. First match wins.
7. Set effective capability = `min(compiled, feed row)` and move to
   `UpdateAvailable` (or `UpToDate`).

Polling: check-on-start ~15 s after launch, then daily; a persisted auto-check
toggle (default on) gates the automatic checks; a manual `check()` always runs.
`lastCheck`/`etag`/`dismissedVersion`/`autoCheck`/`latestVersion`/`notesUrl`
live in the client-local settings store.

## manifest.json schema (`schema: 1`)

```json
{
  "schema": 1,
  "product": "daemon",
  "channel": "stable",
  "version": "0.1.0",
  "released": "2026-07-03T21:58:12Z",
  "notesUrl": "https://…/releases/0.1.0",
  "artifacts": [
    {
      "kind": "appimage",
      "os": "linux",
      "arch": "x86_64",
      "file": "daemon-app-0.1.0-x86_64.AppImage",
      "size": 133742048,
      "sha256": "88573790…",
      "glibcFloor": "2.28",
      "updateCapability": "SelfApply",
      "zsync": "daemon-app-0.1.0-x86_64.AppImage.zsync"
    }
  ]
}
```

Top level:

| Field | Type | Meaning |
|---|---|---|
| `schema` | int | manifest format version; clients reject majors they don't know (currently `1`). |
| `product` | string | always `"daemon"`; rejects a manifest served for the wrong product. |
| `channel` | string | lowercase `[a-z0-9-]` channel id (`stable`, `beta`, …). Must equal the channel the client is configured for (`DAEMON_APP_UPDATE_CHANNEL`, default `stable`). |
| `version` | string | SemVer of this release (no build metadata — that is per-build). |
| `released` | string | ISO 8601 UTC publish time (`SOURCE_DATE_EPOCH` honored for reproducibility). |
| `notesUrl` | string | release-notes page; may be empty. |
| `artifacts` | array | one entry per downloadable artifact. |

Per artifact:

| Field | Type | Meaning |
|---|---|---|
| `kind` | enum | `appimage \| deb \| rpm \| nsis \| dmg \| portable-tar \| apk`. |
| `os` | enum | `linux \| windows \| macos \| android`. |
| `arch` | enum | `x86_64 \| aarch64` (`unknown` if the generator could not parse it — such entries are never auto-selected). |
| `file` | string | artifact name, resolved **relative to the manifest's own (post-redirect) URL** so a feed relocates without rewrites. |
| `size` | int | exact byte size; a cheap pre-gate before hashing. |
| `sha256` | string | hex digest; MUST match the completed download before anything touches the file. |
| `glibcFloor` | string, optional | linux only: minimum glibc the binary needs (from a `<file>.glibc` sidecar at generation time). The client skips artifacts its host can't run. |
| `updateCapability` | enum | the feed dial: `None \| Notify \| DownloadAndOpen \| SelfApply`. |
| `zsync` | string, optional | zsync control file for delta downloads (AppImage; **transport not implemented** — the field is carried, `AppImageUpdate`/the `gh-releases-zsync` line consume it, the in-client downloader does not). |

## Downloader

`UpdateDownloader` fetches a verified-manifest artifact with the full integrity
chain, into a **private per-version staging dir**:

```
<AppDataLocation>/updates/<version>/<file>
```

- the staging dir is created **`0700`**;
- bytes stream to a **`<file>.part`** temp, **resumed via HTTP `Range`** if a
  partial from a prior run is present and still shorter than the expected size.
  (This is a **deliberate deviation** from the original randomized-temp-name
  plan: a deterministic `.part` name is what makes resume possible.)
- a **size pre-gate** aborts early on a byte-count mismatch, then a full
  **sha256 gate** (`QCryptographicHash::Sha256`) must match the signed manifest
  **before** the file is exposed;
- only then is the temp atomically renamed to the final path. Nothing
  downstream ever sees a half-written or unverified file. A cancel keeps the
  `.part` for a later resume.

**Startup hygiene** (`UpdateManager::setSettings`): sweep stale `.part` temps
(older than 7 days) and `updates/<v>` dirs at or below the current version
(`UpdateDownloader::cleanupStale`); remove leftover AppImage `.daemon-update-*`
staging dirs beside `$APPIMAGE` (`cleanupAppImageStaging`); on macOS sweep
leftover `.daemon-app-update-*` staging dirs and the parked old bundles inside
(`macos::cleanupStagingArtifacts`). All best-effort.

## The `daemon-updater` helper

The app never mutates itself. It stages + verifies, hands a spec to the tiny
`daemon-updater` helper (plain C++17, no Qt, POSIX/Win32 behind `#ifdef`), and
quits. The helper waits for the app PID to exit, applies atomically, relaunches,
and self-cleans. It is the **only** process that mutates the installed
application.

**Frozen CLI contract** (`src/tools/updater/updater.cpp`):

```
daemon-updater --wait-pid <pid> --mode <rename-swap|two-move|nsis-silent>
               --staged <path> [--target <path>] [--sha256 <hex>]
               [--old-aside <path>] [--log <path>] [--timeout <secs>]
               [--relaunch -- <argv...>]
```

- **`rename-swap`** (AppImage, POSIX): same-fs check, then `RENAME_EXCHANGE`
  (`renameat2`) with a plain `rename(2)` fallback over the target; the running
  mount pins the old inode, so it is safe even before exit (but `--wait-pid` is
  still honored). Displaced old parked at `--old-aside` or deleted.
- **`two-move`** (macOS `.app` bundle, generic dir/file): move target aside,
  move staged into place, **rollback** (aside → target) on failure; a rollback
  failure is the loud "inconsistent" exit.
- **`nsis-silent`** (Windows): relocate the helper to `%TEMP%` (an exe can't
  overwrite its own install tree), wait, run the staged installer `/S`, relaunch.

**Re-verify before mutate.** For a regular staged file, `--sha256` is required
and re-checked immediately before mutating (closes TOCTOU). For a directory
staged path (the `.app` bundle), `--sha256` must be **absent** — the `0700`
staging dir owns bundle integrity.

**Exit codes:** `0` applied (+ relaunched if requested), `2` staged verify
failed (nothing mutated), `3` apply failed + rolled back, `4` apply failed +
rollback failed (state inconsistent, loud), `5` bad args / wait-pid timeout
(nothing mutated).

**Packaging.** `src/tools/updater/CMakeLists.txt` installs it into every desktop
payload: `usr/bin` in the AppImage, `bin\` in the NSIS install, and
`Contents/MacOS` inside the macOS `.app` (routed there on `APPLE` so it is not
dropped when the user drags only the bundle to `/Applications`). It is excluded
from the wasm/Android/iOS builds. A Linux integration test
(`src/tools/updater/tests/run_tests.sh`) exercises rename-swap, two-move +
rollback, sha256 mismatch, wait-pid + timeout, and detached relaunch.

**Honest caveats:**

- **PID reuse.** Between spawn and exit a reused PID could at worst make the
  helper wait a little longer for an unrelated process; it never mutates early.
- **Directory mode takes no `--sha256`** (the `0700` staging dir owns the
  bundle's integrity — a directory has no single digest to re-check).
- **Windows self-delete is best-effort** — the relocated `%TEMP%` copy schedules
  its own deletion on reboot (`MOVEFILE_DELAY_UNTIL_REBOOT`); a running exe
  cannot delete itself immediately.

## Apply strategies per artifact

`UpdateManager::apply()` routes `SelfApply` to the per-platform backend; every
precondition failure degrades to the shared **`DownloadAndOpen`** hand-off
(open the installer for `.exe`/`.dmg`, reveal the directory otherwise) with a
user-visible reason — a ready update is never dead-ended.

- **AppImage (`SelfApply`)** — `self_apply_appimage`: canonicalize `$APPIMAGE`,
  copy `daemon-updater` **out** of the about-to-vanish squashfs mount into a
  private staging dir on the **target filesystem**, copy the verified downloaded
  image alongside it (same-fs, so the helper's atomic rename never crosses a
  mount), spawn the copied helper detached with the frozen `rename-swap`
  invocation (`--wait-pid <appPid> --relaunch -- $APPIMAGE`), and quit. Any
  precondition failure (no `$APPIMAGE`, helper missing, un-writable AppImage
  directory, copy failure) degrades to `DownloadAndOpen`. Non-AppImage Linux
  kinds never reach the self-apply path (their feed rows are `Notify`).
  *Nix E2E note:* the type2 static-pie AppImage runtime segfaults in the nix
  sandbox, so the E2E boots the extracted payload via the store loader (same as
  the repo's AppImage boot smoke); the self-apply code path, `$APPIMAGE`
  handling, helper discovery, and the real `RENAME_EXCHANGE` swap are exercised
  exactly as in production.
- **Windows NSIS (`SelfApply`)** — `self_apply_windows` +
  `daemon-updater --mode nsis-silent`. The installer is **per-user**
  (`%LOCALAPPDATA%\Programs\Daemon`, `RequestExecutionLevel user`, HKCU
  uninstall hive + per-user Start Menu — the VS Code / Chrome "user setup"
  model), which is what makes an unattended `/S` self-apply **promptless**: an
  `asInvoker` manifest, zero `requireAdministrator`, no UAC. The helper
  self-relocates to `%TEMP%`, waits for the app to exit, runs the installer `/S`
  (uninstall-before-install is on), and relaunches. **Under `/S` the NSIS finish
  hooks do not run — the helper owns the relaunch.** No per-machine → per-user
  migration is needed (nothing shipped per-machine). Real-Windows validation is
  the runbook step: `packaging/windows/UPDATE-VALIDATION.md`.
- **macOS DMG (`SelfApply`)** — `self_apply_macos` +
  `daemon-updater --mode two-move`. **Guards decide SelfApply vs. the
  `DownloadAndOpen` fallback BEFORE any mutation** (the verdict + path
  predicates are pure and unit-tested on Linux, `tst_self_apply_macos`): refuse
  when translocated by Gatekeeper (`/AppTranslocation/`), on a read-only volume
  (launched from the DMG under `/Volumes`), when the application folder is
  unwritable, or when staging would cross a filesystem. On a pass: `hdiutil
  attach` the verified dmg, `ditto` the single `.app` into a fresh `0700` dir on
  the **target filesystem** (`ditto` preserves symlinks/perms/xattrs so the
  bundle + its ad-hoc signature survive), `hdiutil detach` (force-retry), strip
  `com.apple.quarantine` defensively, and superficially verify the staged bundle
  (`Contents/MacOS/daemon-app` executable, `Info.plist`
  `CFBundleShortVersionString` == the manifest version). Then spawn the
  in-bundle helper (`two-move`, `--old-aside` a same-volume park, `--relaunch`)
  and quit; the old bundle is parked then swept on the next start. **Ad-hoc
  signing (team id nil) relaunches cleanly through Gatekeeper on the machine
  that performed the update** (the download is not quarantine-flagged and
  `ditto` from the mount adds no quarantine, so `LSFileQuarantineEnabled` never
  gates), validated E2E on the M1. **Notarization is future work** and only
  tightens verification; if a future host policy ever refuses an ad-hoc
  relaunch, the dial degrades cleanly to `DownloadAndOpen`.
- **deb / rpm (`Notify`)** — banner + notes link only; replacement belongs to
  the package manager / a future repo channel.
- **portable tar (`Notify`)** — banner + notes link only; the unpack location
  is unknown to the app.

The apply-action label is capability-driven and shared by both front ends
(`applyActionLabel`): **"Install & restart"** for a self-applying build,
**"Open"** for the `DownloadAndOpen` hand-off. The GUI banner binds it; the TUI
update surface is Notify-only in the landed core, so there is no TUI apply
button yet.

## Environment / test levers

Both are for the E2E harnesses and validation; a normal build ignores them.

- **`DAEMON_APP_UPDATE_FEED_URL_OVERRIDE`** — point the client at a local signed
  feed. Honored **only on a non-inert build** (`capability != None`), and the
  content is **still gated by the pinned key** — the override changes *where*
  the feed is fetched, never *whether* its signature must verify. Loopback
  `http://` and `file://` are accepted here (see `FeedClient::isAllowedUrl`).
- **`DAEMON_APP_UPDATE_E2E`** (+ `DAEMON_APP_UPDATE_E2E_LOG`) — an auto-drive
  hook that advances check → download → apply with no UI and records
  `E2E boot/apply/uptodate/notify version=` sentinels, so the swap can be proven
  headless. A normal build never sets it.

### E2E entry points

- **AppImage:** `nix run .#updater-appimage-e2e` — builds two real,
  SelfApply-signed AppImages A/B pinned to a throwaway test key, publishes a
  minisign-signed manifest for B on `127.0.0.1`, runs A under the auto-drive,
  and asserts the on-disk `.AppImage` is byte-for-byte B, executable, and boots
  reporting the new version.
- **macOS:** `nix develop --command bash packaging/macos/e2e-selfapply.sh` (the
  M1 runbook; see `packaging/macos/README.md`) — builds two ad-hoc-signed
  SelfApply DMGs A/B with a throwaway test key, signs a loopback feed for B,
  installs A, drives the swap, and asserts version B, the parked-then-swept old
  bundle, the two-move helper log, and no residual quarantine.
- **Windows:** `packaging/windows/UPDATE-VALIDATION.md` — the real-machine
  runbook. Depth 1 drives `daemon-updater.exe` directly (frozen contract,
  per-user `/S`, relaunch, sha256 tamper check); Depth 2 is the full UI E2E with
  a throwaway test key, a locally-signed feed, and the feed-URL override.

**Throwaway keys only.** Every E2E generates a fresh minisign keypair; the
production secret is never on a dev box.

## Release flow

`scripts/release-manifest.sh <artifact-dir> <version> <channel>` discovers each
known artifact by filename, records `size` + `sha256` + arch, consumes sidecars
(`<file>.glibc` → `glibcFloor` on linux; `<file>.zsync` → the `zsync` URL), and
emits one `updateCapability` row per kind:

| kind | `updateCapability` emitted |
|---|---|
| `appimage` | `SelfApply` |
| `nsis` (`.exe`) | `SelfApply` |
| `dmg` | `SelfApply` |
| `deb` / `rpm` / `portable-tar` | `Notify` |
| `apk` | `Notify` (Android's dial is `None` client-side; academic here) |
| `*-wasm.tar.*` | skipped (WASM = `None`; the browser owns delivery) |

The value it records is the artifact's **target ceiling**; the client combines
it with the compiled dial as `min()`. Signing is a detached minisign signature:
set **`MINISIGN_SECRET_KEY_FILE`** to the release secret and the script writes
`<out>.minisig` with the trusted comment `daemon <channel> <version>`; unset, it
warns and emits an **unsigned** manifest (dev only — publishing unsigned is a CI
failure, since clients reject an unverifiable feed). The secret lives **only in
the `daemon-ai/daemon` CI secret store**, pointed at via `MINISIGN_SECRET_KEY_FILE`
in the signing job; it is never committed and (per
`packaging/windows/UPDATE-VALIDATION.md`) never kept on a dev box. The public
key is pinned in every non-inert build.

The AppImage delta line — `gh-releases-zsync|daemon-ai|daemon|latest|daemon-*x86_64.AppImage.zsync`
— is embedded by `cmake/CPackPerGen.cmake` so `AppImageUpdate` can find the
latest release's `.zsync` control file; the in-client downloader does not yet
consume it (see [Implemented vs. future](#implemented-vs-future)).

## Threat model

| Threat | Consequence if unmitigated | Mitigation (implemented) |
|---|---|---|
| Unauthenticated feed | an updater that opens an installer from unsigned JSON is a **remote-code-execution primitive** | Ed25519/minisign-signed manifest verified against a compile-time pinned key before any parse; no TOFU. Drives the whole design. |
| DNS / bucket / CDN compromise | attacker serves a hostile "newer" release | same signature gate: hosting is untrusted by construction; the signature, not the transport, authenticates the feed. HTTPS is kept as defense in depth. |
| Downgrade attack | attacker replays an old, signed, vulnerable release | **SemVer monotonicity** (`semver::isUpgrade`, build-metadata stripped): only `version > current` is offered; the trusted-comment channel/version pin blocks cross-channel/version replay. |
| Partial / corrupted download | half-written binary applied | `size` pre-gate then full `sha256` gate before the file is exposed; the helper **re-verifies** the staged sha256 immediately before mutating (TOCTOU). |
| Sig/manifest mismatch swap | pairing yesterday's `.minisig` with today's manifest | the detached signature covers the exact manifest bytes; any mismatch fails verification. |
| Unsafe in-place swap | a broken/half-applied install | per-platform guards decide SelfApply-vs-fallback **before** mutating; the helper is atomic (rename / two-move + rollback) and honors `--wait-pid`; any failure degrades to `DownloadAndOpen`. |
| Malicious *artifact* behind a valid manifest | compromised CI signs bad bits | out of scope for the feed layer; mitigated by release-pipeline integrity + platform code signing (notarization is future work). Noted so nobody assumes the feed alone proves artifact provenance. |

## Implemented vs. future

**Implemented (ships today):**

- signed `manifest.json` feed generator + signer, ETag-conditional fetch,
  strict schema-1 parse, SemVer monotonicity, artifact selection;
- minisign (Monocypher) verification against the pinned key, anti-replay;
- verified downloader (0700 staging, `.part` + Range resume, size + sha256
  gates, startup hygiene);
- GUI banner + TUI footer/banner Notify parity, check-now, auto-check toggle;
- `SelfApply` on AppImage, Windows NSIS (per-user), and macOS DMG (ad-hoc), via
  the `daemon-updater` helper, each with a `DownloadAndOpen` fallback;
- `DownloadAndOpen` as the universal degrade tier.

**Future work (honestly not done):**

- **macOS notarization** + Developer ID signing + stapling (only the ad-hoc lane
  is validated; `entitlements.plist` not yet authored — see
  `packaging/macos/README.md`);
- **zsync delta transport** in the client (the manifest carries the control-file
  URL and the AppImage `gh-releases-zsync` line is embedded, but the downloader
  fetches whole files);
- **deb/rpm repo channels** (those artifacts stay `Notify`; the package manager
  owns replacement until a repo lands);
- a **non-Nix-linked (static-Qt) macOS payload** to drop the dynamic-Qt
  `/nix/store` frameworks and the library-validation entitlement.
