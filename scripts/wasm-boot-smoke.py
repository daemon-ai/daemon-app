# SPDX-FileCopyrightText: 2026 Jarrad Hope
# SPDX-License-Identifier: MPL-2.0
#
# Event-driven headless CDP harness for the wasm artifact set (stdlib only).
#
#   # boot smoke (default scenario): serve a dir, prove main() ran
#   python3 scripts/wasm-boot-smoke.py <dir> [--port N] [--shot FILE]
#                                      [--timeout SEC] [--chromium BIN]
#                                      [--expect-renderer PREFIX]
#
#   # reload-survival e2e (W6):
#   #   base mode - no daemon, proves the reload + browser-storage substrate
#   python3 scripts/wasm-boot-smoke.py --scenario reload --mode base <dir>
#   #   strict mode - self-boots the daemon serving a patched bundle copy and
#   #   asserts the full re-auth / warm-cache / first-run-survival sentinel chain.
#   #   --seed-session warms node-side state (via --daemon-cli-bin) before load 1
#   #   so the cache survives to load2 with rows>0.
#   python3 scripts/wasm-boot-smoke.py --scenario reload --mode strict \
#       --daemon-bin <daemon> --wasm-dir <wasm-bundle> --login user:pass \
#       --seed-session --daemon-cli-bin <daemon-cli>
#   #   or point at an already-running daemon origin instead of self-booting
#   python3 scripts/wasm-boot-smoke.py --scenario reload --mode strict \
#       --url http://127.0.0.1:PORT --login user:pass
#
# The boot scenario serves <dir> over localhost, drives headless chromium via
# the DevTools protocol, and waits for the app's own boot banner
# ("AppServiceGraph") on the console - a positive, load-independent signal that
# main() ran - then captures a screenshot. Time-budget screenshotting
# (--virtual-time-budget) is inherently racy: the budget expires while V8 is
# still compiling the module on a loaded machine, yielding a spinner screenshot
# and no console.
#
# The reload scenario loads the app, seeds browser-side state, issues Page.reload
# in the SAME chromium profile (one --user-data-dir per run), and asserts the
# state survived. Strict mode consumes the pinned DAEMON_APP_* sentinels; base
# mode only needs the boot banner + a localStorage round-trip, so it is runnable
# against the base build before the sentinel emitters land.
#
# Exit 0 = booted / survived; anything else = failed (fatal console output,
# timeout, missing sentinel, or chromium/daemon died).
import argparse
import base64
import http.client
import http.server
import json
import os
import re
import shutil
import signal
import socket
import struct
import subprocess
import sys
import tempfile
import threading
import time
import functools

FATAL_RE = re.compile(
    r"RuntimeError|CompileError|LinkError|Aborted\(|"
    r"failed to asynchronously prepare wasm|Application exit"
)
BOOT_MARKER = "AppServiceGraph"
# The branded shell logs its WebGL-probe verdict before qtLoad; the smoke
# records it and can assert it (--expect-renderer), so a silent fall-through
# to the software scenegraph cannot masquerade as a healthy GL boot.
RENDERER_RE = re.compile(r"daemon shell: renderer=(.+)")

# Console sentinels the reload scenario consumes, pinned byte-for-byte to
# src/core/platform/wasm_contracts.h (Stream B emits them; this harness only
# reads them). Keep these literals identical to that header.
SENTINEL_READY_OK = "DAEMON_APP_READY ok"
SENTINEL_AUTH_RESUMED = "DAEMON_APP_AUTH resumed"
SENTINEL_AUTH_SCRAM = "DAEMON_APP_AUTH scram"
SENTINEL_FIRSTRUN_DONE = "DAEMON_APP_FIRSTRUN done"
CACHE_ROWS_RE = re.compile(r"DAEMON_APP_CACHE rows=(\d+)")

# The plain-literal env seam the branded shell exposes (daemon-app.html); the
# reload scenario patches a served copy of the bundle to inject the readiness
# probe + the automation login hook before the daemon serves it.
ENV_SEAM = "const environment = {};"


# --- Minimal WebSocket client (RFC 6455, client side) ----------------------
class WebSocket:
    def __init__(self, url):
        m = re.match(r"ws://([^:/]+):(\d+)(/.*)", url)
        host, port, path = m.group(1), int(m.group(2)), m.group(3)
        self.sock = socket.create_connection((host, port))
        key = base64.b64encode(os.urandom(16)).decode()
        handshake = (
            f"GET {path} HTTP/1.1\r\nHost: {host}:{port}\r\n"
            "Upgrade: websocket\r\nConnection: Upgrade\r\n"
            f"Sec-WebSocket-Key: {key}\r\nSec-WebSocket-Version: 13\r\n\r\n"
        )
        self.sock.sendall(handshake.encode())
        resp = b""
        while b"\r\n\r\n" not in resp:
            resp += self.sock.recv(4096)
        if b" 101 " not in resp.split(b"\r\n", 1)[0]:
            raise RuntimeError(f"websocket handshake failed: {resp[:200]!r}")
        self.buf = b""

    def send(self, payload: str):
        data = payload.encode()
        header = bytearray([0x81])  # FIN + text frame
        mask = os.urandom(4)
        n = len(data)
        if n < 126:
            header.append(0x80 | n)
        elif n < 1 << 16:
            header.append(0x80 | 126)
            header += struct.pack(">H", n)
        else:
            header.append(0x80 | 127)
            header += struct.pack(">Q", n)
        header += mask
        masked = bytes(b ^ mask[i % 4] for i, b in enumerate(data))
        self.sock.sendall(bytes(header) + masked)

    def _read_exact(self, n):
        while len(self.buf) < n:
            chunk = self.sock.recv(65536)
            if not chunk:
                raise ConnectionError("websocket closed")
            self.buf += chunk
        out, self.buf = self.buf[:n], self.buf[n:]
        return out

    def recv(self) -> str:
        # Reassemble one message (server frames arrive unmasked).
        message = b""
        while True:
            b0, b1 = self._read_exact(2)
            opcode = b0 & 0x0F
            ln = b1 & 0x7F
            if ln == 126:
                (ln,) = struct.unpack(">H", self._read_exact(2))
            elif ln == 127:
                (ln,) = struct.unpack(">Q", self._read_exact(8))
            payload = self._read_exact(ln)
            if opcode == 0x9:  # ping -> pong
                self.sock.sendall(b"\x8a\x80" + os.urandom(4))
                continue
            if opcode == 0x8:
                raise ConnectionError("websocket close frame")
            message += payload
            if b0 & 0x80:  # FIN
                return message.decode()


class CDP:
    def __init__(self, ws_url):
        self.ws = WebSocket(ws_url)
        self.next_id = 1
        self.events = []

    def call(self, method, params=None, timeout=30):
        mid = self.next_id
        self.next_id += 1
        self.ws.send(json.dumps({"id": mid, "method": method, "params": params or {}}))
        deadline = time.time() + timeout
        while time.time() < deadline:
            msg = json.loads(self.ws.recv())
            if msg.get("id") == mid:
                if "error" in msg:
                    raise RuntimeError(f"{method}: {msg['error']}")
                return msg.get("result", {})
            self.events.append(msg)
        raise TimeoutError(method)

    def drain(self, timeout):
        """Yield events for up to `timeout` seconds."""
        deadline = time.time() + timeout
        while self.events:
            yield self.events.pop(0)
        self.ws.sock.settimeout(1.0)
        while time.time() < deadline:
            try:
                msg = json.loads(self.ws.recv())
            except socket.timeout:
                continue
            if "method" in msg:
                yield msg


def console_text(event) -> str:
    p = event.get("params", {})
    if event.get("method") == "Runtime.consoleAPICalled":
        return " ".join(str(a.get("value", "")) for a in p.get("args", []))
    if event.get("method") == "Log.entryAdded":
        return p.get("entry", {}).get("text", "")
    if event.get("method") == "Runtime.exceptionThrown":
        d = p.get("exceptionDetails", {})
        return "EXCEPTION " + json.dumps(d.get("exception", {}))[:400]
    return ""


# --- shared harness plumbing -----------------------------------------------
def free_port() -> int:
    """An ephemeral loopback port the kernel just handed out (racy but fine for
    a single-run harness; the daemon/http.server binds it immediately after)."""
    with socket.socket() as s:
        s.bind(("127.0.0.1", 0))
        return s.getsockname()[1]


def serve_static(directory, port):
    """Serve `directory` over loopback in a daemon thread; returns the server."""

    class Quiet(http.server.SimpleHTTPRequestHandler):
        def log_message(self, *a):
            pass

    server = http.server.ThreadingHTTPServer(
        ("127.0.0.1", port), functools.partial(Quiet, directory=directory)
    )
    threading.Thread(target=server.serve_forever, daemon=True).start()
    return server


def launch_chromium(chromium):
    """Start headless chromium with a fresh persistent profile; returns
    (process, profile_dir). The profile dir is one --user-data-dir per run, so
    a Page.reload against it exercises real origin-storage survival."""
    profile = tempfile.mkdtemp(prefix="wasm-smoke-")
    proc = subprocess.Popen(
        [
            chromium,
            "--headless=new",
            "--remote-debugging-port=0",
            f"--user-data-dir={profile}",
            "--enable-unsafe-swiftshader",
            "--window-size=1400,900",
            "about:blank",
        ],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
    return proc, profile


def open_cdp(profile):
    """Wait for chromium's DevTools port, open a fresh page target, and return a
    CDP session with Runtime/Log/Page domains enabled."""
    port_file = os.path.join(profile, "DevToolsActivePort")
    for _ in range(100):
        if os.path.exists(port_file) and open(port_file).read().strip():
            break
        time.sleep(0.1)
    dbg_port = int(open(port_file).read().split()[0])
    conn = http.client.HTTPConnection("127.0.0.1", dbg_port)
    conn.request("PUT", "/json/new?about:blank")
    target = json.loads(conn.getresponse().read())
    cdp = CDP(target["webSocketDebuggerUrl"])
    cdp.call("Runtime.enable")
    cdp.call("Log.enable")
    cdp.call("Page.enable")
    return cdp


def stop_chromium(proc):
    proc.terminate()
    try:
        proc.wait(5)
    except subprocess.TimeoutExpired:
        proc.kill()


def screenshot(cdp, path, log):
    """Drain a few seconds so the first frame paints, then capture a PNG."""
    for event in cdp.drain(3):
        text = console_text(event)
        if text:
            print(text, file=log, flush=True)
    shot = cdp.call("Page.captureScreenshot", {"format": "png"}, timeout=30)
    with open(path, "wb") as f:
        f.write(base64.b64decode(shot["data"]))


def evaluate(cdp, expression):
    """Runtime.evaluate returning a by-value JS result."""
    result = cdp.call(
        "Runtime.evaluate", {"expression": expression, "returnByValue": True}
    )
    return result.get("result", {}).get("value")


def watch_console(cdp, timeout, log, needles):
    """Drain console output until every needle fires, a fatal line appears, or
    the timeout lapses. `needles` is a list of (name, predicate) pairs; returns
    (found: {name: text}, fatal: str | None, renderer: str | None)."""
    found = {}
    fatal = None
    renderer = None
    for event in cdp.drain(timeout):
        text = console_text(event)
        if not text:
            continue
        print(text, file=log, flush=True)
        match = RENDERER_RE.search(text)
        if match:
            renderer = match.group(1)
        if FATAL_RE.search(text):
            fatal = text
            break
        for name, pred in needles:
            if name not in found and pred(text):
                found[name] = text
        if len(found) == len(needles):
            break
    return found, fatal, renderer


# --- reload-survival scenario (W6) ------------------------------------------
def patch_bundle(src_dir, dst_dir, env):
    """Copy the wasm bundle to `dst_dir` and inject `env` into the branded
    shell's `const environment = {};` seam (a served copy, never the store
    path). The pre-compressed daemon-app.html siblings are dropped so the daemon
    negotiates the patched identity copy rather than a stale .br/.gz."""
    shutil.copytree(src_dir, dst_dir)
    # copytree preserves the store's read-only dir mode; make every copied
    # directory writable so siblings can be dropped and the tree cleaned up.
    for root, _dirs, _files in os.walk(dst_dir):
        os.chmod(root, 0o755)
    html_path = os.path.join(dst_dir, "daemon-app.html")
    with open(html_path, encoding="utf-8") as f:
        html = f.read()
    if ENV_SEAM not in html:
        raise RuntimeError(
            f"env seam {ENV_SEAM!r} not found in {html_path} (shell layout changed?)"
        )
    injected = "const environment = " + json.dumps(env) + ";"
    html = html.replace(ENV_SEAM, injected, 1)
    # copytree preserves the store's read-only mode; make the served copy writable
    # before rewriting it (and so teardown can clean the tree up).
    os.chmod(html_path, 0o644)
    with open(html_path, "w", encoding="utf-8") as f:
        f.write(html)
    for sibling in ("daemon-app.html.br", "daemon-app.html.gz"):
        stale = os.path.join(dst_dir, sibling)
        if os.path.exists(stale):
            os.remove(stale)


def start_daemon(daemon_bin, web_root, addr, login):
    """Boot the debug daemon serving `web_root` on `addr`, with an isolated data
    dir/socket, the deterministic mock provider (so the first-run node gate
    auto-completes), and the first-admin seed matching `login`. Returns
    (process, tempdir). The daemon owns its own session group so teardown can
    reap any workers it spawns."""
    user, _, password = login.partition(":")
    tmp = tempfile.mkdtemp(prefix="e2e-node-", dir="/tmp")
    data = os.path.join(tmp, "state")
    home = os.path.join(tmp, "home")
    for path in (data, home):
        os.makedirs(path)
    env = os.environ.copy()
    env.update(
        {
            "DAEMON_WEB__ADDR": addr,
            "DAEMON_WEB__ROOT": web_root,
            "DAEMON_DATA_DIR": data,
            "DAEMON_SOCKET_PATH": os.path.join(tmp, "d.sock"),
            # Deterministic in-tree provider + a non-empty model so the seeded
            # default profile is CONFIGURED and the client's first-run node gate
            # (A2) auto-skips the wizard, persisting setupComplete.
            "DAEMON_MODEL_PROVIDER": "mock",
            "DAEMON_MODEL": "mock-model",
            # First-admin seed (env-first): matches the DAEMON_APP_LOGIN hook so
            # the browser completes SCRAM once on the first load.
            "DAEMON_ADMIN_USERNAME": user,
            "DAEMON_ADMIN_PASSWORD": password,
            "HOME": home,
            "XDG_DATA_HOME": os.path.join(home, ".local", "share"),
            "XDG_CONFIG_HOME": os.path.join(home, ".config"),
            "XDG_CACHE_HOME": os.path.join(home, ".cache"),
            "RUST_LOG": env.get("RUST_LOG", "info"),
        }
    )
    log = open(os.path.join(tmp, "daemon.log"), "w")
    proc = subprocess.Popen(
        [daemon_bin], env=env, stdout=log, stderr=log, start_new_session=True
    )
    return proc, tmp


def seed_session(cli_bin, sock, session_id="e2e-seed"):
    """Create durable node-side session activity BEFORE the first browser load so
    the client's initial roster sync writes rows into the SQLite cache - which the
    debounced FS.syncfs then persists to IDBFS, so the reload's pre-fetch cache
    count (load2) is a non-vacuous >0. daemon-cli speaks the node's local admin api
    socket (no SCRAM). `assign` creates+wakes a durable session (a daemon_sessions
    roster row); the mock-provider `submit` turn additionally seeds conversation/
    transcript rows (best-effort - the roster row alone satisfies the assertion)."""
    base = [cli_bin, "--socket", sock]
    subprocess.run(
        base + ["assign", session_id],
        check=True,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
        timeout=30,
    )
    try:
        subprocess.run(
            base + ["submit", session_id, "hello"],
            check=True,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            timeout=30,
        )
    except (subprocess.SubprocessError, OSError):
        pass  # the durable session from `assign` already gives the roster a row


def stop_daemon(proc):
    if proc is None:
        return
    try:
        os.killpg(os.getpgid(proc.pid), signal.SIGTERM)
    except ProcessLookupError:
        return
    try:
        proc.wait(5)
    except subprocess.TimeoutExpired:
        try:
            os.killpg(os.getpgid(proc.pid), signal.SIGKILL)
        except ProcessLookupError:
            pass


def wait_for_origin(addr, timeout):
    """Poll GET /daemon-app.html on `addr` until it returns 200 (the web front's
    liveness signal - `GET /` returns the GUI entry page) or the deadline lapses."""
    host, _, port = addr.partition(":")
    deadline = time.time() + timeout
    while time.time() < deadline:
        try:
            conn = http.client.HTTPConnection(host, int(port), timeout=2)
            conn.request("GET", "/daemon-app.html")
            if conn.getresponse().status == 200:
                return True
        except OSError:
            pass
        time.sleep(0.2)
    return False


MARKER_KEY = "daemonAppReloadMarker"
MARKER_VALUE = "survived"


def run_reload(args, log, log_path):
    strict = args.mode == "strict"
    server = daemon = daemon_tmp = patched = None
    try:
        if args.url:
            origin = args.url.rstrip("/")
        elif strict:
            if not args.daemon_bin or not args.wasm_dir:
                print(
                    "reload/strict needs --daemon-bin and --wasm-dir (or --url)",
                    file=sys.stderr,
                )
                return 2
            patched = tempfile.mkdtemp(prefix="e2e-web-")
            web_root = os.path.join(patched, "web")
            patch_bundle(
                args.wasm_dir,
                web_root,
                {
                    "DAEMON_APP_WAIT_READY": str(int(args.timeout * 1000)),
                    "DAEMON_APP_LOGIN": args.login,
                },
            )
            port = args.port or free_port()
            addr = f"127.0.0.1:{port}"
            daemon, daemon_tmp = start_daemon(
                args.daemon_bin, web_root, addr, args.login
            )
            if not wait_for_origin(addr, 30):
                print(
                    f"reload: daemon origin {addr} did not answer within 30s "
                    f"(daemon log under {daemon_tmp})",
                    file=sys.stderr,
                )
                return 1
            # Seed node-side session activity before load 1 so the first sync warms
            # the client cache (and thence IDBFS) - makes load2's pre-fetch >0 real.
            if args.seed_session:
                if not args.daemon_cli_bin:
                    print(
                        "reload/strict --seed-session needs --daemon-cli-bin",
                        file=sys.stderr,
                    )
                    return 2
                try:
                    seed_session(args.daemon_cli_bin, os.path.join(daemon_tmp, "d.sock"))
                except (subprocess.SubprocessError, OSError) as exc:
                    print(f"reload: seed-session failed: {exc}", file=sys.stderr)
                    return 1
            origin = f"http://{addr}"
        else:
            if not args.dir:
                print(
                    "reload/base needs a bundle dir (positional) or --url",
                    file=sys.stderr,
                )
                return 2
            port = args.port or free_port()
            server = serve_static(args.dir, port)
            origin = f"http://127.0.0.1:{port}"

        proc, profile = launch_chromium(args.chromium)
        try:
            cdp = open_cdp(profile)
            url = f"{origin}/daemon-app.html"

            # ---- first load ----
            cdp.call("Page.navigate", {"url": url})
            if strict:
                load1 = [
                    ("ready", lambda t: SENTINEL_READY_OK in t),
                    ("scram", lambda t: SENTINEL_AUTH_SCRAM in t),
                    ("firstrun", lambda t: SENTINEL_FIRSTRUN_DONE in t),
                    ("cache", lambda t: CACHE_ROWS_RE.search(t) is not None),
                ]
            else:
                load1 = [("boot", lambda t: BOOT_MARKER in t)]
            found, fatal, renderer = watch_console(cdp, args.timeout, log, load1)
            if fatal is not None:
                print(f"reload: FATAL on first load: {fatal[:300]}", file=sys.stderr)
                return 1
            missing = [name for name, _ in load1 if name not in found]
            if missing:
                print(
                    f"reload: first load missing {missing} within {args.timeout}s "
                    f"(log: {log_path})",
                    file=sys.stderr,
                )
                return 1
            rows1 = 0
            if strict:
                # The load1 cache sentinel is emitted at boot, BEFORE the first
                # roster sync (a nested wait is forbidden on single-threaded wasm),
                # so on a first-ever load the freshly-created cache is legitimately
                # empty (rows=0). Persistence is proven by load2's pre-fetch count
                # (>0 = synced rows survived via IDBFS), which --seed-session makes
                # non-vacuous. Record load1's count for the report; don't assert >0.
                rows1 = int(CACHE_ROWS_RE.search(found["cache"]).group(1))

            # Seed a browser-storage marker: localStorage is the backend QSettings
            # rides on wasm, so its survival across the reload is the same
            # substrate that carries ui/* prefs + setupComplete.
            evaluate(
                cdp,
                f"window.localStorage.setItem({MARKER_KEY!r}, {MARKER_VALUE!r})",
            )

            # ---- reload in the same profile ----
            # Give the roster -> cache -> IDBFS pipeline time to land before reload.
            # On connect-ready the client runs refreshSessions() (near-instant), but
            # the cache write-back is a debounced FS.syncfs (kIdbfsSyncDebounceMs=3s
            # in src/core/platform/wasm_persistence.cpp). Reloading too soon tears the
            # page down before that syncfs completes, so load2 would preload an empty
            # IDBFS. In strict mode wait out the debounce (+ margin) so the seeded
            # roster is durably in IndexedDB; base only needs the localStorage marker.
            settle_s = 7.0 if strict else 1.0
            for _ in cdp.drain(settle_s):  # flush late first-load lines; let syncfs land
                pass
            cdp.events.clear()
            cdp.call("Page.reload", {})

            # ---- second load ----
            if strict:
                load2 = [
                    ("ready", lambda t: SENTINEL_READY_OK in t),
                    ("resumed", lambda t: SENTINEL_AUTH_RESUMED in t),
                    # Require a POSITIVE cache count: the app emits two cache lines - a
                    # boot line on the empty pre-auth default namespace (always 0) and a
                    # post-auth line on the per-user db, which on a resume is the
                    # IDBFS-preloaded copy. Matching the >0 line (not merely "present")
                    # pins the assertion to the pre-fetch per-user reading that proves
                    # the SQLite cache survived the reload.
                    (
                        "cache",
                        lambda t: (m := CACHE_ROWS_RE.search(t)) is not None
                        and int(m.group(1)) > 0,
                    ),
                ]
            else:
                load2 = [("boot", lambda t: BOOT_MARKER in t)]
            found2, fatal2, _ = watch_console(cdp, args.timeout, log, load2)
            if fatal2 is not None:
                print(f"reload: FATAL on second load: {fatal2[:300]}", file=sys.stderr)
                return 1
            missing2 = [name for name, _ in load2 if name not in found2]
            if missing2:
                # A silent resume is the whole point: if 'resumed' is missing the
                # app fell back to a fresh SCRAM (setupComplete/token was lost).
                print(
                    f"reload: second load missing {missing2} within {args.timeout}s "
                    f"(log: {log_path})",
                    file=sys.stderr,
                )
                return 1
            if strict:
                rows2 = int(CACHE_ROWS_RE.search(found2["cache"]).group(1))
                if rows2 <= 0:
                    print(
                        f"reload: second-load cache rows not positive ({rows2}) - "
                        "IDBFS did not survive",
                        file=sys.stderr,
                    )
                    return 1

            marker = evaluate(cdp, f"window.localStorage.getItem({MARKER_KEY!r})")
            if marker != MARKER_VALUE:
                print(
                    f"reload: localStorage marker lost across reload "
                    f"(got {marker!r}, want {MARKER_VALUE!r})",
                    file=sys.stderr,
                )
                return 1

            screenshot(cdp, args.shot, log)
            detail = f"mode={args.mode}; marker survived; renderer: {renderer}"
            if strict:
                detail += f"; cache rows load1={rows1} load2={rows2}"
            print(f"reload: OK ({detail}; log: {log_path}, shot: {args.shot})")
            return 0
        finally:
            stop_chromium(proc)
    finally:
        if server is not None:
            server.shutdown()
        stop_daemon(daemon)
        for path in (patched, daemon_tmp):
            if path is not None:
                shutil.rmtree(path, ignore_errors=True)


# --- boot smoke scenario (default) ------------------------------------------
def run_boot(args, log, log_path):
    server = serve_static(args.dir, args.port)
    proc, profile = launch_chromium(args.chromium)
    try:
        cdp = open_cdp(profile)
        cdp.call(
            "Page.navigate", {"url": f"http://127.0.0.1:{args.port}/daemon-app.html"}
        )

        booted = fatal = renderer = None
        for event in cdp.drain(args.timeout):
            text = console_text(event)
            if not text:
                continue
            print(text, file=log, flush=True)
            renderer_match = RENDERER_RE.search(text)
            if renderer_match:
                renderer = renderer_match.group(1)
            if FATAL_RE.search(text):
                fatal = text
                break
            if BOOT_MARKER in text:
                booted = text
                break

        if fatal is not None:
            print(f"boot-smoke: FATAL console output: {fatal[:300]}", file=sys.stderr)
            return 1
        if booted is None:
            print(
                f"boot-smoke: no boot marker within {args.timeout}s "
                f"(log: {log_path})",
                file=sys.stderr,
            )
            return 1
        if args.expect_renderer is not None and not (renderer or "").startswith(
            args.expect_renderer
        ):
            print(
                f"boot-smoke: renderer mismatch: expected {args.expect_renderer}*, "
                f"shell chose {renderer!r} (log: {log_path})",
                file=sys.stderr,
            )
            return 1

        screenshot(cdp, args.shot, log)
        print(
            f"boot-smoke: OK (marker seen; renderer: {renderer}; "
            f"log: {log_path}, shot: {args.shot})"
        )
        return 0
    finally:
        stop_chromium(proc)
        server.shutdown()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("dir", nargs="?", help="wasm bundle dir to serve (boot; reload/base)")
    ap.add_argument(
        "--scenario",
        choices=("boot", "reload"),
        default="boot",
        help="boot smoke (default) or reload-survival e2e",
    )
    ap.add_argument(
        "--mode",
        choices=("base", "strict"),
        default="base",
        help="reload: 'base' (no daemon, substrate only) or 'strict' (full sentinels)",
    )
    ap.add_argument("--url", default=None, help="reload: load from this origin instead of self-serving")
    ap.add_argument("--daemon-bin", default=None, help="reload/strict: debug daemon binary to self-boot")
    ap.add_argument("--wasm-dir", default=None, help="reload/strict: bundle to patch + serve via the daemon")
    ap.add_argument("--login", default="e2e:e2e-passphrase", help="reload/strict: user:pass for admin seed + login hook")
    ap.add_argument(
        "--seed-session",
        action="store_true",
        help="reload/strict: create a durable node session via daemon-cli before "
        "load 1 (warms the client cache -> IDBFS so load2's pre-fetch count is >0)",
    )
    ap.add_argument(
        "--daemon-cli-bin",
        default=None,
        help="reload/strict: daemon-cli binary used by --seed-session",
    )
    ap.add_argument("--port", type=int, default=0, help="listen port (0 = ephemeral; boot defaults to 8901)")
    ap.add_argument("--shot", default="/tmp/size-smoke.png")
    ap.add_argument("--timeout", type=float, default=90)
    ap.add_argument("--chromium", default=os.environ.get("CHROMIUM", "chromium"))
    ap.add_argument(
        "--expect-renderer",
        default=None,
        help="fail unless the shell's renderer log starts with this "
        "(e.g. 'webgl' matches webgl2/webgl, 'software' the fallback)",
    )
    args = ap.parse_args()

    log_path = args.shot.rsplit(".", 1)[0] + ".log"
    log = open(log_path, "w")

    if args.scenario == "reload":
        return run_reload(args, log, log_path)
    if args.dir is None:
        print("boot scenario needs a bundle dir (positional)", file=sys.stderr)
        return 2
    if not args.port:
        args.port = 8901
    return run_boot(args, log, log_path)


if __name__ == "__main__":
    sys.exit(main())
