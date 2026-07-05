# Windows auto-update (SelfApply) — real-machine validation runbook

Wine (the `windows-smoke` app) validates packaging shape and the per-user
install location, but it is **emulation, not Windows**: it cannot faithfully
exercise the detached `daemon-updater` helper's wait-pid → `/S` → relaunch dance
or UAC behaviour. This runbook is the human validation step on a real
Windows 10/11 x64 box. Do it before trusting a `SelfApply` release.

Two depths:

- **Depth 1 — helper mechanics (no rebuild, ~10 min).** Drive `daemon-updater.exe`
  directly to prove the frozen contract, the silent per-user reinstall, and the
  relaunch on real Windows.
- **Depth 2 — full E2E through the UI (~30 min).** Build test-keyed installers,
  serve a local signed feed, and click Notify → Download → **Install & restart**.

> **Never use the production signing key.** The production minisign secret lives
> only in the `daemon-ai/daemon` CI secret store — it is never on a dev box and
> never in `~/.secrets`. All validation uses a throwaway test keypair.

---

## What "correct" looks like (assert all of these)

- The installer runs **with no UAC prompt** (per-user, `RequestExecutionLevel user`).
- The tree lands under `%LOCALAPPDATA%\Programs\Daemon\` (i.e.
  `C:\Users\<you>\AppData\Local\Programs\Daemon\`), with `bin\daemon-app.exe` and
  `bin\daemon-updater.exe`.
- The uninstall entry is in **HKCU** (Add/Remove Programs, per-user), updated to
  the new version after an update.
- After **Install & restart**, the app relaunches on its own and
  `About` / the status bar reports **version B**.
- The helper log records each step (see [Helper log](#helper-log)).

---

## Depth 1 — helper mechanics (no rebuild)

Proves the exact production flow minus the feed/UI. Uses whatever installer you
already have.

1. **Install version A** (any current per-user installer, e.g.
   `daemon-<A>-win64.exe`): double-click, confirm **no UAC**, confirm it lands in
   `%LOCALAPPDATA%\Programs\Daemon\`. Launch it once.

2. **Stage a "new" installer** — copy a *different* version's installer
   (`daemon-<B>-win64.exe`) somewhere, e.g. `%TEMP%\daemon-<B>-win64.exe`, and
   compute its SHA-256 (this is what the helper re-verifies before mutating):

   ```powershell
   $staged = "$env:TEMP\daemon-<B>-win64.exe"
   $sha = (Get-FileHash $staged -Algorithm SHA256).Hash.ToLower()
   $app = "$env:LOCALAPPDATA\Programs\Daemon\bin\daemon-app.exe"
   $helper = "$env:LOCALAPPDATA\Programs\Daemon\bin\daemon-updater.exe"
   $log = "$env:TEMP\daemon-updater-test.log"
   ```

3. **Run the frozen invocation** exactly as the app would (start the app first so
   there is a live PID to wait on, then run the helper against that PID):

   ```powershell
   Start-Process $app
   $pid = (Get-Process daemon-app | Select-Object -First 1).Id
   & $helper --wait-pid $pid --mode nsis-silent --staged $staged --sha256 $sha `
             --log $log --relaunch -- $app
   # Now close the running daemon-app window (or Stop-Process -Id $pid).
   ```

   The helper: relocates itself to `%TEMP%`, waits for the app PID to exit,
   re-verifies `$sha`, runs `$staged /S` (no UAC), and relaunches `$app`.

4. **Assert**: no UAC prompt; the app reappears; `About`/status bar shows
   **version B**; `%LOCALAPPDATA%\Programs\Daemon\bin\daemon-app.exe` is the B
   build; `type $log` shows `verify ok`, `nsis installer /S exit 0`,
   `relaunch ok`, `done applied`.

5. **Tamper check** (optional but recommended): rerun step 3 with a wrong
   `--sha256` (flip a hex digit). The helper must log `verify MISMATCH` and exit
   non-zero **without** running the installer — nothing is mutated.

---

## Depth 2 — full E2E through the UI

Exercises the whole chain: feed fetch → minisign verify → download → sha256 gate
→ **Install & restart** → helper → relaunch.

Because the minisign public key is pinned at compile time (TOFU is rejected), a
locally-signed test feed only verifies against a build that pins the matching
**test** public key. So Depth 2 needs test-keyed installers.

### 1. Generate a throwaway test keypair (on the build host)

```sh
minisign -G -p /tmp/test-minisign.pub -s /tmp/test-minisign.key -W   # -W: no password (test only)
tail -n1 /tmp/test-minisign.pub   # the base64 public key to pin below
```

### 2. Build test-keyed installers vA and vB (Linux, nix cross build)

Temporarily point the Windows build's pinned pubkey at your test key and pick two
low versions. Edit `nix/windows.nix` `appMingw` and set:

```
-DDAEMON_APP_UPDATE_CAPABILITY=SelfApply   # already the default
-DDAEMON_APP_UPDATE_PUBKEY=<paste the test pub key base64>
```

Then build each version (the version string comes from `VERSION` / the flake's
`versionStr`; use `just set-version daemon-app 0.0.1` then `0.0.2`, or override
`versionStr`), producing `daemon-0.0.1-win64.exe` and `daemon-0.0.2-win64.exe`:

```sh
nix build .#nsis          # -> result/daemon-<ver>-win64.exe
```

**Revert the `nix/windows.nix` edit afterwards** — the production build must keep
pinning the production key `RWRXpowS90Fy+TYhRsrBbQNSDvjbtJpqi9T89OGqSNTLkOa5vn62hK0o`.

### 3. Build the signed test feed (advertising vB)

Put the vB installer alone in a directory and generate + sign the manifest with
the **test secret key**:

```sh
mkdir -p /tmp/feed && cp result/daemon-0.0.2-win64.exe /tmp/feed/
MINISIGN_SECRET_KEY_FILE=/tmp/test-minisign.key \
  scripts/release-manifest.sh /tmp/feed 0.0.2 stable \
    --notes-url https://example.invalid/notes
# writes /tmp/feed/manifest.json (+ .minisig). The nsis row auto-advertises SelfApply.
```

Serve it over HTTP(S). The client requires HTTPS for the compiled feed, but the
runtime **override** accepts what you give it; a plain local `http://` server is
fine for validation:

```sh
cd /tmp/feed && python3 -m http.server 8000
```

Copy `daemon-0.0.1-win64.exe` and note the feed URL
`http://<build-host>:8000/manifest.json` for the Windows box (same LAN, or run
the server on Windows).

### 4. On Windows: install vA, point the app at the test feed

1. Install `daemon-0.0.1-win64.exe` (no UAC; lands in `%LOCALAPPDATA%\Programs\Daemon`).
2. Set the feed override (test-only escape hatch; only honored on a non-inert build):

   ```powershell
   [Environment]::SetEnvironmentVariable(
     "DAEMON_APP_UPDATE_FEED_URL_OVERRIDE", "http://<build-host>:8000/manifest.json", "User")
   ```
3. Launch the app.

### 5. Click through the flow

- An **Update available: v0.0.1 → v0.0.2** banner appears (auto-check fires ~15 s
  after start, or use the Settings → Updates check-now action).
- Click **Download** → the banner shows download progress, then
  *"Update v0.0.2 is ready to install"*.
- Click **Install & restart** (the SelfApply verb — proves `applyActionLabel`).
- The app quits, the installer runs silently (no UAC), the app relaunches.

### 6. Assert

- **No UAC prompt** at any point.
- `About` / status bar reports **0.0.2** after relaunch.
- `%LOCALAPPDATA%\Programs\Daemon\bin\daemon-app.exe` is the 0.0.2 build.
- The HKCU Add/Remove Programs entry now reads 0.0.2 (uninstall-before-install
  cleaned 0.0.1).
- The [helper log](#helper-log) shows the full apply.

### 7. Clean up

Remove the env override, uninstall, delete `/tmp/test-minisign.*` and `/tmp/feed`.

---

## Helper log

`daemon-updater` appends one greppable line per step to the `--log` path and
mirrors every line to stderr. In the production flow the app passes
`%APPDATA%\daemon-app\daemon-app\updates\daemon-updater.log`
(`QStandardPaths::AppDataLocation` + `updates\`, the same tree the installer was
staged into). Key lines to expect on success:

```
daemon-updater: relocated to temp and re-spawned; exiting parent
daemon-updater: wait-pid target exited
daemon-updater: verify ok sha256=<hex>
daemon-updater: nsis installer /S exit 0
daemon-updater: relaunch ok
daemon-updater: done applied
```

Exit codes (frozen contract): `0` applied, `2` staged verify failed (nothing
mutated), `3` apply failed + rolled back, `4` apply failed + inconsistent (loud),
`5` bad args / wait-pid timeout.

## Degrade path

If `daemon-updater.exe` is missing from `bin\`, no installer is staged, or the
checksum is absent, `apply()` does **not** dead-end: it surfaces a visible reason
(*"self-apply unavailable (…); opening the installer instead"*) and falls back to
opening the downloaded installer for the user (DownloadAndOpen). Validate this by
temporarily renaming `bin\daemon-updater.exe` before clicking **Install &
restart**.
