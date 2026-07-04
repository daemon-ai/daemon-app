# Release feed & auto-update design

Status: **design + scaffolding only.** The signed feed tooling
(`scripts/release-manifest.sh`, `scripts/sync-updates-json.sh`) and the
`UpdateManager` capability surface (`src/core/update`) exist; every
download/apply path is an explicit not-implemented stub. See
[What is NOT implemented](#what-is-not-implemented-now).

## The capability dial

Update capability is a **per-artifact dial fixed at package time** — a compile
definition baked into the binary, not a runtime setting. Each step strictly
contains the previous one:

| Dial | The app may... |
|---|---|
| `None` | nothing — no feed fetch, no UI. The platform owns updates. |
| `Notify` | fetch + verify the feed, tell the user a newer version exists, link the notes. |
| `DownloadAndOpen` | additionally download the artifact, verify its sha256, and hand it to the OS (open installer / mount dmg). The **user** completes the install. |
| `SelfApply` | additionally replace itself and relaunch without user interaction beyond consent. |

Per-artifact targets (settled):

| Artifact | Dial | Rationale |
|---|---|---|
| AppImage | `SelfApply` | self-contained file; verify → rename over `$APPIMAGE` → relaunch. |
| NSIS `.exe` (Windows) | `SelfApply` | run the signed installer silently (`/S`). |
| DMG `.app` (macOS) | `DownloadAndOpen` first, `SelfApply` later | open-and-instruct v1; in-place `.app` swap needs quarantine/codesign care. |
| deb / rpm | `Notify` | the distro repo owns package replacement (a repo lands in a later workstream). |
| portable tar | `Notify` | unpacked location unknown; user manages it. |
| APK (Android, sideload) | `Notify` | debug-signed sideload artifact; the Play Store owns in-store updates, so the feed only notifies. |
| WASM / app stores | `None` | the platform (browser cache / store) owns delivery. |

Two dials exist per artifact and they compose:

- **Compiled dial** (`DAEMON_APP_UPDATE_CAPABILITY`, default `None`): the
  ceiling. Set per package job; nothing at runtime can raise it.
- **Feed dial** (`updateCapability` in `manifest.json`): advisory per release.
- **Effective capability = min(compiled, feed).** The feed can therefore
  emergency-lower a fleet (e.g. publish `Notify` to disable a misbehaving
  self-apply) but can never escalate a shipped binary.

Rollout: day 1, every package job compiles `DownloadAndOpen` **or lower**
(deb/rpm/tar `Notify`, dmg/AppImage/NSIS at most `DownloadAndOpen`). Raising an
artifact to its target dial later is a one-line change in its package job — no
schema, feed, or client-code change.

## Feeds

- **`manifest.json` (+ detached `manifest.json.minisig`) — the real feed.**
  One per channel, e.g. `https://<host>/daemon/<channel>/manifest.json`.
  Generated and signed by `scripts/release-manifest.sh` in release CI.
- **`packaging/UPDATES.json` — legacy/simple mirror** in QSimpleUpdater's
  format, kept for QSimpleUpdater-shaped consumers and as a human-readable
  pointer. Patched from a manifest by `scripts/sync-updates-json.sh`;
  `just set-version` (superproject) also writes its `latest-version`. It is
  unsigned and MUST NOT gate any download or apply decision.

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
| `channel` | string | lowercase `[a-z0-9-]` channel id (`stable`, `beta`, …). Must equal the channel the client is configured for. |
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
| `file` | string | artifact name, resolved **relative to the manifest's own URL** so a feed relocates without rewrites. |
| `size` | int | exact byte size; cheap pre-gate before hashing. |
| `sha256` | string | hex digest; MUST match the completed download before anything touches the file. |
| `glibcFloor` | string, optional | linux only: minimum glibc the binary needs (from a `<file>.glibc` sidecar at generation time). Client skips artifacts its host can't run. |
| `updateCapability` | enum | the feed dial (see above): `None \| Notify \| DownloadAndOpen \| SelfApply`. |
| `zsync` | string, optional | zsync control file for delta downloads (AppImage; transport not implemented yet). |

## Signing & verification

- Algorithm: **Ed25519 via [minisign](https://jedisct1.github.io/minisign/)**,
  detached signature `manifest.json.minisig`.
- **Who signs:** release CI holds the secret key in its secret store
  (`MINISIGN_SECRET_KEY_FILE`); `release-manifest.sh` signs at publish time.
  Keys are generated with `minisign -G` and **never committed** to any repo.
- **Who verifies:** the app. The minisign **public key is pinned in the binary
  at compile time** (it will ride the same package-time define mechanism as the
  dial). Feed → verify signature against the pinned key → only then parse.
  The current release-feed public key (pin this at compile time; the matching
  secret lives only in the `daemon-ai/daemon` `MINISIGN_SECRET_KEY` CI secret):

  ```
  RWRXpowS90Fy+TYhRsrBbQNSDvjbtJpqi9T89OGqSNTLkOa5vn62hK0o
  ```
- **TOFU is explicitly rejected.** The client never learns a key from the
  network; a manifest that doesn't verify against the compiled-in pin is
  discarded — there is no "trust this new key?" prompt and no first-use grace.
- The signature's **trusted comment** carries `daemon <channel> <version>`, so
  a valid `.minisig` cannot be replayed onto a different channel's or
  version's manifest.
- Unsigned manifests are for **local development only**; the generator prints
  an explicit warning and CI must treat publishing one as a failure.
- Key rotation = ship a release (signed with the old key) whose binaries pin
  the new key; the old key keeps signing that release's channel until the
  fleet has moved.

## Threat model

| Threat | Consequence if unmitigated | Mitigation |
|---|---|---|
| Unauthenticated feed | an updater that opens an installer from unsigned JSON is a **remote-code-execution primitive** | Ed25519-signed manifest, pubkey pinned in the binary; no TOFU. This drives the whole design. |
| DNS / bucket / CDN compromise | attacker serves a hostile "newer" release | same signature gate: hosting is untrusted by construction; a valid signature, not the transport, authenticates the feed. HTTPS remains required as defense-in-depth. |
| Downgrade attack | attacker replays an old, signed, vulnerable release | **version monotonicity requirement**: client only ever offers `version > currentVersion` (SemVer compare, not string); equal/lower signed manifests are ignored. Channel pin in the trusted comment blocks cross-channel replay. |
| Partial / corrupted download | half-written binary applied | `sha256` (+ `size` pre-check) gate: nothing is opened, executed, or renamed into place until the digest of the *complete* file matches the signed manifest. |
| Sig/manifest mismatch swap | pairing yesterday's `.minisig` with today's manifest | detached signature covers the exact bytes; any mismatch fails verification. |
| Malicious *artifact* behind a valid manifest | compromised CI signs bad bits | out of scope for the feed layer; mitigated by release-pipeline integrity + platform code signing (NSIS/notarization), noted here so nobody assumes the feed alone proves artifact provenance. |

## Apply strategies per artifact (design; not implemented)

- **AppImage (`SelfApply`)** — download to a temp path on the same filesystem
  as `$APPIMAGE`; verify sha256; `chmod +x`; atomically `rename(2)` over
  `$APPIMAGE`; re-exec the new image with the previous argv. Rollback = the
  rename never happened (temp file discarded). zsync delta transport arrives
  later; the manifest already carries the control-file URL.
- **NSIS (`SelfApply`)** — download `.exe`; verify sha256; run the **signed**
  installer silently (`/S`); installer handles in-use file swap; app exits as
  the installer starts and relaunches via the installer's finish hook.
- **DMG (`DownloadAndOpen` v1)** — download; verify sha256; mount/open the dmg
  and instruct the user to drag to Applications. (`SelfApply` later: swap the
  `.app` bundle + handle quarantine/codesign, only once notarization is wired.)
- **deb/rpm (`Notify`)** — banner + notes link only; replacement belongs to the
  package manager / future repo.
- **portable tar (`Notify`)** — banner + notes link only.

## What is NOT implemented now

Everything that touches the network or the filesystem is deliberately absent:

- feed fetch / polling / scheduling (no `QNetworkAccessManager` anywhere in
  `src/core/update`);
- signature verification in the client (pubkey pin define + verify call);
- artifact download, resume, sha256 gating;
- any apply path (AppImage rename/re-exec, NSIS `/S`, DMG open);
- zsync delta transport;
- version-monotonicity comparator in the client;
- any GUI/TUI surface (deliberate: per AGENTS.md parity rules, UI lands
  GUI+TUI together in the wiring workstream).

What exists today: the signed-feed generator + verifier and mirror-sync
scripts, this design, and the `update::UpdateManager` scaffold
(`src/core/update`) exposing the compiled dial (`capability`), `currentVersion`
and `feedUrl`, whose `check()`/`download()`/`apply()` emit honest
`not implemented` errors.
