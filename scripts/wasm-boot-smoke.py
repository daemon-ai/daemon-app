# SPDX-FileCopyrightText: 2026 Jarrad Hope
# SPDX-License-Identifier: MPL-2.0
#
# Event-driven headless boot check for the wasm artifact set (stdlib only).
#
#   python3 scripts/wasm-boot-smoke.py <dir> [--port N] [--shot FILE]
#                                      [--timeout SEC] [--chromium BIN]
#
# Serves <dir> over localhost, drives headless chromium via the DevTools
# protocol, and waits for the app's own boot banner ("AppServiceGraph") on
# the console - a positive, load-independent signal that main() ran - then
# captures a screenshot. Time-budget screenshotting (--virtual-time-budget)
# is inherently racy: the budget expires while V8 is still compiling the
# module on a loaded machine, yielding a spinner screenshot and no console.
#
# Exit 0 = booted; anything else = failed (fatal console output, timeout,
# or chromium died).
import argparse
import base64
import http.client
import http.server
import json
import os
import re
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


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("dir")
    ap.add_argument("--port", type=int, default=8901)
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

    class Quiet(http.server.SimpleHTTPRequestHandler):
        def log_message(self, *a):
            pass

    server = http.server.ThreadingHTTPServer(
        ("127.0.0.1", args.port), functools.partial(Quiet, directory=args.dir)
    )
    threading.Thread(target=server.serve_forever, daemon=True).start()

    profile = tempfile.mkdtemp(prefix="wasm-smoke-")
    proc = subprocess.Popen(
        [
            args.chromium,
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
    try:
        port_file = os.path.join(profile, "DevToolsActivePort")
        for _ in range(100):
            if os.path.exists(port_file) and open(port_file).read().strip():
                break
            time.sleep(0.1)
        dbg_port = int(open(port_file).read().split()[0])

        conn = http.client.HTTPConnection("127.0.0.1", dbg_port)
        conn.request("PUT", f"/json/new?{'about:blank'}")
        target = json.loads(conn.getresponse().read())
        cdp = CDP(target["webSocketDebuggerUrl"])

        cdp.call("Runtime.enable")
        cdp.call("Log.enable")
        cdp.call("Page.enable")
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

        # Give the first frame a moment to paint, then screenshot.
        for event in cdp.drain(3):
            t = console_text(event)
            if t:
                print(t, file=log, flush=True)
        shot = cdp.call("Page.captureScreenshot", {"format": "png"}, timeout=30)
        with open(args.shot, "wb") as f:
            f.write(base64.b64decode(shot["data"]))
        print(
            f"boot-smoke: OK (marker seen; renderer: {renderer}; "
            f"log: {log_path}, shot: {args.shot})"
        )
        return 0
    finally:
        proc.terminate()
        try:
            proc.wait(5)
        except subprocess.TimeoutExpired:
            proc.kill()
        server.shutdown()


if __name__ == "__main__":
    sys.exit(main())
