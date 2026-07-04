# Windows packaging (MinGW cross, static Qt, NSIS)

The Windows desktop build is cross-compiled from Linux with
`pkgsCross.mingwW64` (x86_64-w64-mingw32, nixpkgs from the logos-co
`mingw-integration` fork pinned in `flake.nix`) against a statically linked
Qt built from source (`nix/qt-from-source.nix`, flavor `mingw-static` in
`nix/windows.nix`). The result is ONE self-contained `daemon-app.exe` -
Qt modules, QPA plugins (windows + offscreen), QML, fonts, sqlite and the
full mingw runtime (libgcc/libstdc++/mcfgthread via Qt's
`FEATURE_static_runtime`) are all linked in; the only DLL imports are
Windows inbox system libraries (gated by `checks.windows-sanity`). TLS is
Schannel (native Windows), so there is no OpenSSL to ship or service.

## Build

```sh
nix build .#windows-portable   # bin/daemon-app.exe + share/ (portable layout)
nix build .#nsis               # daemon-<version>-win64.exe NSIS installer
nix build .#qt-mingw-static    # just the cross Qt prefix (debugging)
nix flake check                # includes checks.windows-sanity (PE gate)
```

`packages.nsis` configures the app once (same derivation as
`windows-portable`'s payload) and runs `cpack -G NSIS` with `makensis` from
nixpkgs on the build host. Installer config lives in
`cmake/CPackPerGen.cmake` (NSIS branch) + `cmake/Packaging.cmake`:
`$PROGRAMFILES64\Daemon` install root, Start-Menu shortcut, optional
Desktop icon (installer checkbox), license page (LICENSE + third-party
notices), uninstall-before-install on upgrades, uninstaller that removes
shortcuts + registry entries.

The `DAEMON_APP_BUNDLED_DAEMON` / `_DAEMON_INFER` / `_DAEMON_CLI` cache
vars work here exactly like in the Linux artifacts: the superproject hands
prebuilt sibling **Windows** binaries in and they land in `bin\` next to
`daemon-app.exe` (`LocalDaemonLauncher` looks next-to-exe first). Until the
Rust cross workstream lands, the installer ships the GUI alone - that is a
complete, useful product: the connection picker speaks to remote/WebSocket
daemons out of the box.

## Wine smoke (best-effort, NOT a gate)

```sh
nix run .#windows-smoke
```

Boots `daemon-app.exe` under wine with `QT_QPA_PLATFORM=offscreen` (the
offscreen QPA is compiled in) and the `DAEMON_APP_WAIT_READY` sentinel
contract from `main.cpp` (prints `DAEMON_APP_READY ok`, exits 0), then
runs the NSIS installer silently (`/S`) into a scratch prefix and boots
the installed exe. Wine is emulation, not Windows: results are recorded
honestly but failures do not gate the workstream. Note the exe is a GUI
subsystem binary (no console window); the sentinel is still captured
because the harness redirects stdout - an *interactive* console will show
no output, which is expected.

## Code signing (documented hook; v1 ships unsigned)

Unsigned binaries trip SmartScreen ("Windows protected your PC" - users
must click "More info" -> "Run anyway") and some AV heuristics. For a
public release, sign BOTH the app exe and the installer with
[osslsigncode](https://github.com/mtrojnar/osslsigncode) (packaged in
nixpkgs) after `nix build`:

```sh
# Prereqs: an Authenticode code-signing certificate (EV recommended -
# instant SmartScreen reputation; OV builds reputation over time) as
# cert.pem + key.pem (or a PKCS#11 URI for an HSM-held EV key).
osslsigncode sign -certs cert.pem -key key.pem \
  -n "Daemon" -i https://daemon.io \
  -t http://timestamp.digicert.com \
  -in daemon-app.exe -out daemon-app-signed.exe

osslsigncode sign -certs cert.pem -key key.pem \
  -n "Daemon Installer" -i https://daemon.io \
  -t http://timestamp.digicert.com \
  -in daemon-1.2.3-win64.exe -out daemon-1.2.3-win64-signed.exe
```

Sign the inner exe FIRST, then rebuild/repack the installer around it (or
sign the artifacts a release pipeline publishes; store outputs are
read-only, so signing is a post-build step by design). Always timestamp
(`-t`), so signatures outlive certificate expiry.

## What a Windows operator must verify per release

1. `checks.windows-sanity` green (PE32+ GUI-subsystem exe, DLL floor,
   icon resource, installer produced).
2. Install on a real Windows 10/11 x64 box: installer UI, Start-Menu +
   optional Desktop shortcut, app boots, uninstall removes everything.
3. Upgrade path: install an older version, run the new installer
   (uninstall-before-install must clean the old payload).
4. If signed: `signtool verify /pa daemon-app.exe` + SmartScreen behavior.

## Updates

NSIS maps onto the `SelfApply` update capability in the
`packaging/UPDATES.md` design (the release-feed workstream, landing
separately): the app downloads the new installer and executes it; NSIS
`CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL` keeps the tree clean across
versions. Until that lands, updates are manual (download + run installer).

## Known caveats (v1)

* **Math rendering**: the MicroTeX resource dir ships in the layout
  (`share\daemon-app\microtex-res`), but until the `pkg/size-wins` branch
  (runtime probing of exactly that path) merges, the binary still probes a
  baked build path that cannot exist on Windows; math blocks render a
  caught per-formula error instead. Everything else is unaffected. The
  merge train resolves this without further work here.
* **Managed local daemon**: spawning/adopting a local daemon is Unix-only
  today (`LocalDaemonLauncher` pid/SIGTERM handling); on Windows use a
  remote/WebSocket daemon, or the bundled `daemon.exe` once the sibling
  workstream lands its Windows port of the spawn path.
* **Terminal panel**: the static gate substitutes the same terminal stub
  as the browser build (QMLTermWidget is a shared-Qt qmake plugin and GPL;
  see THIRD-PARTY-NOTICES.md).
