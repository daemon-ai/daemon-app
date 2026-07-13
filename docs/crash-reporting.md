<!--
SPDX-License-Identifier: MPL-2.0
SPDX-FileCopyrightText: 2026 Jarrad Hope
-->

# Crash reporting (Sentry)

Cross-platform crash reporting for the whole `daemon` product — the `daemon-app` GUI/TUI **and**
the `daemon-node` process tree (`daemon`, `daemon-infer`, `daemon-train-worker`, `daemon-cli`, and
the metta/pytool workers). One Sentry SaaS project (`daemon-ai/daemon`), consent-gated, opt-in.

This doc is the source of truth for the architecture, the consent model, the DSN/env contract, the
symbol-upload pipeline, the Windows backend decision, the macOS verification checklist, and the
privacy note. It lives in `daemon-app/docs/` but covers all three repos.

## Why Sentry + minidumps

Both C++ (Qt) and Rust compile to native code, so the common denominator is the **minidump**: a
handler in each process captures a minidump on a native fault (identical for C++ and Rust), and the
backend symbolicates it against uploaded debug info. Rust *panics* are a second, distinct failure
mode caught by a panic hook. Sentry covers both in one project:

- **C++ / Qt** — `sentry-native` (crashpad/breakpad backend) captures native crashes.
- **Rust** — the `sentry` crate captures panics; `sentry-rust-minidump` captures native crashes via
  an out-of-process monitor.

See `daemon-app/references/crash-reporting.md` (in the main checkout) for the full vendor
evaluation that led here.

## Architecture — one project, correlated by tags

```
daemon-app (GUI/TUI)  --- sentry-native (crashpad handler) --------\
                                                                    >-- Sentry project daemon-ai/daemon
daemon-node (host)    --- sentry + sentry-rust-minidump (monitor) --/
  ├─ placed-child / transport-server (same daemon binary, role tag)
  ├─ daemon-infer (infer-worker)      spawned; DSN+consent+session+pid injected
  ├─ daemon-train-worker (train-worker)          "
  ├─ metta / pytool workers                      "
  └─ daemon-cli (panic-only, no minidump monitor)
```

Every reporting process tags its events with `component` (the role: `gui` / `tui` / `host` /
`placed-child` / `transport-server` / `infer-worker` / `train-worker` / `cli`). Node workers
additionally carry `session_id` + `parent_pid`, injected by the spawning node into their
environment, so a worker crash correlates with the node placement that started it.

### daemon-app (C++)

- `src/core/crash/crash_reporter.{h,cpp}` (`daemon-app::crash`) wraps sentry-native. It compiles to
  no-op stubs on wasm/mobile and when the SDK is absent (`DAEMON_APP_HAVE_SENTRY`), so the GUI + TUI
  mains call the SAME seam everywhere.
- Init happens in `main()` right after the app object + name/version are set (so `QStandardPaths` +
  `applicationDirPath()` resolve) and **before** the QML engine / TUI root — covering the whole
  scene load and run. `QApplication` is already constructed at that point; the deliberate, accepted
  tradeoff is that a crash in the very first pre-init frames is not captured. (`sentry_init` before
  `QApplication` is possible but the database/handler paths need `QStandardPaths`, which needs the
  application name — the net capture window lost is negligible.)
- The database lives under `QStandardPaths::AppDataLocation/crash-db`; the handler is
  `applicationDirPath()/crashpad_handler[.exe]` — the SAME co-location contract the node binary
  uses (`local_daemon_launcher.cpp`).
- **Breadcrumbs**: the GUI installs a `qInstallMessageHandler` that forwards `qWarning`/`qCritical`/
  `qFatal` into the Sentry breadcrumb trail then chains to the prior handler; the TUI extends its
  existing `tuiMessageHandler` (which must never write to stderr — breadcrumbs are in-memory only).

### daemon-node (Rust)

- `crates/substrate/daemon-telemetry/src/crash.rs`: `init_crash_reporting(component) -> CrashGuard`
  (panic integration + native minidump monitor) and `init_panic_reporting(component)` (panic only,
  for the CLI). Release string = `daemon-node@{daemon_common::VERSION}`.
- Called from `bins/daemon` (role-tagged after clap parse), `daemon-infer`, `daemon-train-worker`
  (both before clap/stdio), and `daemon-cli` (panic-only).
- **Minidump monitor re-exec**: `sentry-rust-minidump` → `minidumper-child` spawns the monitor by
  re-exec'ing the current binary with an extra `--crash-reporter-server=<socket>` arg; in that copy
  `spawn()` runs the server loop then `exit(0)`. Consequences honoured by the call sites:
  - the `daemon` binary declares a hidden global `--crash-reporter-server` clap arg so the monitor
    copy is not rejected before it reaches init; the workers init *before* clap so the monitor
    short-circuits first.
  - the monitor is a plain child (no `kill_on_drop`, no process group of its own): invisible to
    `ProcessProvisioner` accounting, self-exits on a stale-socket timeout once its parent is gone.
- `panic = "unwind"` is preserved (never changed to abort).

## Consent model — opt-in, dedicated, distinct from telemetry

Crash reporting has its **own** consent toggle ("Send crash reports"), separate from the existing
telemetry consent, **default OFF**.

- **Node wire API (v41)**: `CrashConsentGet` / `CrashConsentSet{enabled}` → `CrashConsent{enabled}`,
  mirroring the telemetry-consent verbs. Persisted in the node store (`crash_consent` single-row
  table, default OFF). On change the node updates the `DAEMON_CRASH_CONSENT` it injects into future
  worker spawns.
- **App seam**: `IFeedback::crashReportingEnabled` / `setCrashReportingEnabled` (GUI
  `AdvancedSection.qml` row + the TUI settings surface, one shared C++ seam). On toggle the app
  (a) calls the wire API AND (b) flips the LOCAL Sentry consent immediately + caches it in
  QSettings (`crash/consent`), so the reporter arms correctly at the next startup before any node
  connection exists.
- **Two capture disciplines**:
  - *App*: initialized with `require_user_consent = 1` — capture is ARMED immediately (a crash right
    after launch is caught) but every UPLOAD waits for consent. `giveConsent()`/`revokeConsent()`
    flip the gate; a dump captured while consent is off stays in the local `crash-db` and is only
    sent once consent is granted.
  - *Node*: consent-off means Sentry is simply **not initialized** (no local retention on the node
    side — the app owns dump retention).
- `send_default_pii` is **never** enabled.

## DSN / environment contract

The DSN is a public ingest key — safe to embed.

```
https://500fed6a24304a5615c66ef479824fe6@o4511727459827712.ingest.de.sentry.io/4511727469199441
```

| Var | Consumer | Meaning |
| --- | --- | --- |
| `DAEMON_APP_SENTRY_DSN` | daemon-app | compiled default (`-DDAEMON_APP_SENTRY_DSN`); env overrides at runtime. Empty ⇒ crash reporting compiled out. |
| `DAEMON_SENTRY_DSN` | daemon-node | runtime only (NOT compiled in). Absent ⇒ disabled no-op. |
| `DAEMON_CRASH_CONSENT` | daemon-node | `1` ⇒ consent granted. Set by packaging / propagated to workers; `crash_consent_set` updates this process's copy for future spawns. |
| `DAEMON_SESSION_ID` / `DAEMON_PARENT_PID` | daemon-node workers | correlation tags injected by the spawning node. |

Threading (superproject `flake.nix`, owned at the bundle layer): `bundleWithDaemon`'s `wrapProgram`
`--set-default`s both `DAEMON_APP_SENTRY_DSN` and `DAEMON_SENTRY_DSN` (so the nix bundle's spawned
daemon + workers get the DSN); the installers compile `-DDAEMON_APP_SENTRY_DSN` into the app
(`nodeBundleFlags` / `bundledDmg`; NSIS inherits the daemon-app child default).

> **Known follow-up**: in the OS installers (deb/rpm/NSIS/DMG) the app is launched without the nix
> wrapper, so `DAEMON_SENTRY_DSN` is not in the app's env — the spawned *node* therefore has no DSN
> unless the app injects its compiled DSN into the daemon's environment at spawn
> (`LocalDaemonLauncher`). The app itself reports (compiled DSN); wiring the node's DSN for installer
> builds is the recommended next step.

## Backends per platform + the Windows decision

`cmake/Dependencies.cmake` selects `SENTRY_BACKEND` (override with `-DDAEMON_APP_SENTRY_BACKEND`):

| Platform | Backend | Status |
| --- | --- | --- |
| Linux | **crashpad** | **Proven**: `libsentry.a` + `crashpad_handler` build under the Nix devShell (curl transport, static). |
| macOS | **crashpad** (SDK default) | Correct by construction; must be verified on a mac (see checklist). |
| Windows (MinGW cross) | **breakpad** | Decision — see below. |

**Windows backend decision: breakpad (not crashpad).** Crashpad's Windows handler is built on
mini_chromium + GN, which assume the **MSVC** toolchain; this repo's only Windows lane is the MinGW
cross (`pkgsCross.mingwW64`), and adding an MSVC lane is out of scope. Breakpad builds under MinGW
and still produces uploadable minidumps. `inproc` is the documented last-resort fallback. Only the
crashpad backend ships a `crashpad_handler` executable, so `Packaging.cmake` co-locates it **only**
when the backend is crashpad — i.e. NSIS ships it only if the Windows decision ever changes to
crashpad. *This session proved the Linux crashpad build; the MinGW breakpad build was not yet
verified end-to-end — see the quality-gate notes / final report.*

## Packaging — ship the handler co-located

`Packaging.cmake` installs `crashpad_handler` next to the app in every desktop package
(deb/rpm → `/opt/daemon/bin`, AppImage → `usr/bin`, DMG → `Contents/MacOS`, NSIS → `bin\`,
portable → `bin/`), matching `crash::defaultHandlerPath()`. Guarded on both the target existing AND
the backend being crashpad.

## Symbols (CI) — non-negotiable for release builds

Release binaries are stripped, so unsymbolicated minidumps are near-useless. `.github/workflows/
release.yml` uploads debug info per build job and finalizes releases in publish, all guarded on
`SENTRY_AUTH_TOKEN` (skipped gracefully on forks/secret-less runs); `SENTRY_ORG=daemon-ai`,
`SENTRY_PROJECT=daemon`.

- **Rust**: a dedicated `release-symbols` Cargo profile (`inherits = "release"`, `strip = false`,
  `debug = "line-tables-only"`) is built as a SEPARATE, upload-only step — the shipped stripped
  `--release` binaries are byte-identical and small; the release-symbols binaries are upload-only,
  never packaged (closure-size impact on the product: none).
- **C++**: the daemon-app artifact lane's `RelWithDebInfo + separateDebugInfo` debug files.
  `sentry-cli debug-files upload --include-sources` matches them to the stripped binaries by
  build-id. *TODO(human): expose a daemon-app `-sym` output and point the CI step at it.*
- **publish**: `sentry-cli releases new/finalize` for `daemon-app@<ver>` and `daemon-node@<ver>`.

**Human setup required**: create the `SENTRY_AUTH_TOKEN` repository secret (project-scoped, with
`project:releases` + debug-file write). Without it the steps no-op.

## Validation — local crash corpus

- **App**: `DAEMON_APP_CRASH_TEST=segv|abort|throw` (hidden, opt-in) crashes the process after the
  handler is armed, so a minidump can be verified in the local `crash-db`. With consent OFF the dump
  is captured but NOT uploaded. Do NOT point automated tests at the real DSN — one manual
  verification event is fine.
- **Node**: `daemon-telemetry` has a unit test asserting the DSN/consent gates (disabled cases) +
  the correlation-env shape without side-effectful init.

## macOS verification checklist (must be done on a mac)

1. Build the DMG lane on `aarch64-darwin`; confirm `crashpad_handler` lands in
   `daemon-app.app/Contents/MacOS` beside the app + node binaries.
2. Confirm `crash::defaultHandlerPath()` resolves to it (applicationDirPath == `Contents/MacOS`).
3. Trigger `DAEMON_APP_CRASH_TEST=segv`; confirm a minidump is captured in `crash-db` and (with
   consent on) uploaded.
4. Upload dSYMs via `sentry-cli debug-files upload` and confirm a symbolicated stack in Sentry.
5. Verify the co-located node binaries' install-name relocation (the existing DMG backstop) does not
   disturb the handler.

## Privacy note

A minidump is a snapshot of process memory: it can contain stack + selected heap memory, and thus
potentially user-entered content, tokens, paths, or environment data. Therefore:

- Upload is **opt-in** (default OFF), via a dedicated, clearly-labelled consent separate from
  telemetry. Dumps are held locally until consent is granted (app) / not initialized at all (node).
- `send_default_pii` is never enabled; breadcrumbs carry only the Qt log trail (which the app
  already routes to stderr/journald), not user content.
- Transport is HTTPS to Sentry's EU ingest (`ingest.de.sentry.io`).
- Retention is governed by the Sentry project's settings; keep dump retention tight and never use
  dump contents as analytics.
