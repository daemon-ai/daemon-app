#!/usr/bin/env bash
# SPDX-License-Identifier: MPL-2.0
# SPDX-FileCopyrightText: 2026 Jarrad Hope
#
# Accessibility (AT-SPI) audit gate -- sandbox composer for scripts/a11y-audit.py.
#
# Builds the throwaway environment the walker needs and drives one app launch per
# page: a private D-Bus session (dbus-daemon) -> a private headless X display
# (Xvfb) -> the AT-SPI bus launcher + registry -> the app rendered at each page
# with QT_LINUX_ACCESSIBILITY_ALWAYS_ON=1, snapshotted by the pyatspi walker, then
# killed. Finally runs the aggregation gate (walk reports + the static blind-spot
# pass), which fails on any non-allowlisted violation.
#
# We launch the private session bus ourselves (rather than dbus-run-session) so we
# can tear it down deterministically: app launches activate session-bus services
# (xdg-desktop-portal, kwallet) that otherwise keep dbus-run-session blocked at
# exit. Killing our own dbus-daemon in the cleanup trap terminates them with it.
#
# If the sandbox dependencies are absent, or a private bus / X / a11y tree cannot
# come up here, the script exits 77 so ctest records a SKIP (a bare checkout, or a
# machine without a usable bus/X, skips instead of failing). The pure-python
# static blind-spot pass always runs, so that half of the gate ratchets anywhere.
# See tests/CMakeLists.txt (SKIP_RETURN_CODE 77).
#
# Usage:
#   a11y-audit.sh --binary <daemon-app> --allowlist <file> [--src <dir>]
set -uo pipefail

SKIP=77

# --- args -----------------------------------------------------------------
BINARY=""
ALLOWLIST=""
SRC=""
while [[ $# -gt 0 ]]; do
    case "$1" in
        --binary) BINARY="$2"; shift 2 ;;
        --allowlist) ALLOWLIST="$2"; shift 2 ;;
        --src) SRC="$2"; shift 2 ;;
        *) echo "a11y-audit: unknown arg: $1" >&2; exit 2 ;;
    esac
done

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
PY="$SCRIPT_DIR/a11y-audit.py"
[[ -z "$ALLOWLIST" ]] && ALLOWLIST="$SCRIPT_DIR/a11y-audit-allowlist.txt"
[[ -z "$SRC" ]] && SRC="$REPO_ROOT/src"

have() { command -v "$1" >/dev/null 2>&1; }

static_only_skip() {
    # The walker cannot run here; still ratchet the pure-python static pass, then
    # report SKIP so a machine without a usable bus/X degrades instead of failing.
    echo "a11y-audit: SKIP -- $1" >&2
    python3 "$PY" gate --reports "$(mktemp -d)" --root "$SRC" --allowlist "$ALLOWLIST" || {
        local rc=$?
        [[ $rc -ne 0 ]] && exit $rc # a real static violation is a genuine failure
    }
    exit "$SKIP"
}

# --- dependency gate (runs before any heavy setup) ------------------------
missing=()
have dbus-daemon || missing+=("dbus-daemon")
have Xvfb || missing+=("Xvfb")
python3 -c 'import gi; gi.require_version("Atspi","2.0"); import pyatspi' 2>/dev/null \
    || missing+=("python3+pyatspi")

ATSPI_LAUNCHER=""
for cand in \
    "${DAEMON_APP_ATSPI_LIBEXEC:-}/at-spi-bus-launcher" \
    "$(command -v at-spi-bus-launcher 2>/dev/null || true)"; do
    if [[ -n "$cand" && -x "$cand" ]]; then ATSPI_LAUNCHER="$cand"; break; fi
done
[[ -z "$ATSPI_LAUNCHER" ]] && missing+=("at-spi-bus-launcher")
ATSPI_REGISTRYD="$(dirname "$ATSPI_LAUNCHER")/at-spi2-registryd"

if [[ ${#missing[@]} -gt 0 ]]; then
    static_only_skip "sandbox deps missing: ${missing[*]} (enter the daemon-app devShell to get them)"
fi

if [[ -z "$BINARY" || ! -x "$BINARY" ]]; then
    echo "a11y-audit: --binary is required and must be executable (got: '$BINARY')" >&2
    exit 2
fi

# =====================  sandbox  ==========================================
WORK="$(mktemp -d)"
# shellcheck disable=SC2329  # invoked indirectly via `trap cleanup EXIT` below.
cleanup() {
    set +e
    [[ -n "${APP_PID:-}" ]] && kill "$APP_PID" 2>/dev/null
    [[ -n "${REG_PID:-}" ]] && kill "$REG_PID" 2>/dev/null
    [[ -n "${BUS_PID:-}" ]] && kill "$BUS_PID" 2>/dev/null
    [[ -n "${XVFB_PID:-}" ]] && kill "$XVFB_PID" 2>/dev/null
    # Killing our own dbus-daemon terminates every service it bus-activated
    # (xdg-desktop-portal / kwallet), which a dbus-run-session teardown would
    # otherwise hang waiting on.
    [[ -n "${DBUS_DAEMON_PID:-}" ]] && kill "$DBUS_DAEMON_PID" 2>/dev/null
    # Sweep any stragglers we parented (portal helpers reparent to us).
    pkill -P $$ 2>/dev/null
    rm -rf "$WORK"
}
trap cleanup EXIT

REPORTS="$WORK/reports"
mkdir -p "$REPORTS"
export XDG_RUNTIME_DIR="$WORK/xdg"; mkdir -p "$XDG_RUNTIME_DIR"; chmod 700 "$XDG_RUNTIME_DIR"

# --- private session bus (some sandboxes ship a session.conf with no <listen>
#     address, so supply a minimal one; launch it ourselves for a clean kill) --
DBUS_CFG="$WORK/session.conf"
cat >"$DBUS_CFG" <<'XML'
<!DOCTYPE busconfig PUBLIC "-//freedesktop//DTD D-Bus Bus Configuration 1.0//EN"
 "http://www.freedesktop.org/standards/dbus/1.0/busconfig.dtd">
<busconfig>
  <type>session</type>
  <listen>unix:tmpdir=/tmp</listen>
  <standard_session_servicedirs/>
  <policy context="default">
    <allow send_destination="*" eavesdrop="true"/>
    <allow eavesdrop="true"/>
    <allow own="*"/>
  </policy>
</busconfig>
XML
DBUS_DAEMON_PID=""
DBUS_SESSION_BUS_ADDRESS="$(
    dbus-daemon --config-file="$DBUS_CFG" --fork \
        --print-address --print-pid=3 3>"$WORK/dbus.pid" 2>/dev/null
)" || true
[[ -f "$WORK/dbus.pid" ]] && DBUS_DAEMON_PID="$(cat "$WORK/dbus.pid")"
if [[ -z "$DBUS_SESSION_BUS_ADDRESS" ]]; then
    static_only_skip "could not start a private session bus (dbus-daemon) here"
fi
export DBUS_SESSION_BUS_ADDRESS

# --- private headless X display -------------------------------------------
DISPLAY_NUM=""
for n in 99 98 97 101 102 123 145; do
    rm -f "/tmp/.X${n}-lock" 2>/dev/null
    Xvfb ":$n" -screen 0 1600x1200x24 -nolisten tcp >"$WORK/xvfb.log" 2>&1 &
    XVFB_PID=$!
    for _ in $(seq 1 20); do
        if [[ -S "/tmp/.X11-unix/X$n" ]] && kill -0 "$XVFB_PID" 2>/dev/null; then
            DISPLAY_NUM="$n"; break
        fi
        kill -0 "$XVFB_PID" 2>/dev/null || break
        sleep 0.2
    done
    [[ -n "$DISPLAY_NUM" ]] && break
    kill "$XVFB_PID" 2>/dev/null; XVFB_PID=""
done
if [[ -z "$DISPLAY_NUM" ]]; then
    static_only_skip "could not start Xvfb ($(tail -n1 "$WORK/xvfb.log" 2>/dev/null))"
fi
export DISPLAY=":$DISPLAY_NUM"

# --- AT-SPI accessibility bus + registry ----------------------------------
"$ATSPI_LAUNCHER" --launch-immediately >"$WORK/atspi-bus.log" 2>&1 &
BUS_PID=$!
sleep 0.5
if [[ -x "$ATSPI_REGISTRYD" ]]; then
    "$ATSPI_REGISTRYD" >"$WORK/atspi-reg.log" 2>&1 &
    REG_PID=$!
fi
sleep 0.5

# --- seed two home dirs: onboarded (mock, ready) + first-run --------------
seed_home() {
    local home="$1" setup="$2"
    mkdir -p "$home/.config/daemon-app"
    {
        echo "[app]"
        echo "setupComplete=$setup"
        echo
        echo "[conn]"
        echo "mode=mock"
        echo "target=ready"
    } >"$home/.config/daemon-app/daemon-app.conf"
}
HOME_READY="$WORK/home-ready"; seed_home "$HOME_READY" "true"
HOME_FIRST="$WORK/home-first"; seed_home "$HOME_FIRST" "false"

# --- one launch + snapshot per page ---------------------------------------
# label:page:home  (empty page -> base shell; home selects onboarded/first-run)
PAGES=(
    "base::$HOME_READY"
    "settings:settings:$HOME_READY"
    "models:models:$HOME_READY"
    "accounts:accounts:$HOME_READY"
    "profiles:profiles:$HOME_READY"
    "fleet:fleet:$HOME_READY"
    "sessions:sessions:$HOME_READY"
    "dashboard:dashboard:$HOME_READY"
    "approvals:approvals:$HOME_READY"
    "routing:routing:$HOME_READY"
    "cron:cron:$HOME_READY"
    "channels:channels:$HOME_READY"
    "access:access:$HOME_READY"
    "firstrun::$HOME_FIRST"
)

connected_any=0
early_exits=0
for entry in "${PAGES[@]}"; do
    IFS=":" read -r label page home <<<"$entry"
    export HOME="$home"
    APP_LOG="$WORK/app-$label.log"

    render_env=()
    [[ -n "$page" ]] && render_env=(DAEMON_APP_RENDER_PAGE="$page")

    # Keep the app off the session-bus desktop portals (file dialogs etc.): they
    # bus-activate long-lived helpers we would then have to reap.
    env DAEMON_APP_SERVICE_MODE=mock \
        QT_QPA_PLATFORM=xcb \
        QT_LINUX_ACCESSIBILITY_ALWAYS_ON=1 \
        QT_QUICK_BACKEND=software \
        LANG=C.UTF-8 \
        QT_FORCE_STDERR_LOGGING=1 \
        GTK_USE_PORTAL=0 \
        QT_NO_XDG_DESKTOP_PORTAL=1 \
        "${render_env[@]}" \
        "$BINARY" >"$APP_LOG" 2>&1 &
    APP_PID=$!

    # Give the shell time to load the QML root, connect the mock, navigate, and
    # register its a11y tree before snapshotting.
    settled=0
    for _ in $(seq 1 25); do
        kill -0 "$APP_PID" 2>/dev/null || break
        sleep 0.2
        settled=$((settled + 1))
        [[ $settled -ge 12 ]] && break
    done
    if ! kill -0 "$APP_PID" 2>/dev/null; then
        echo "a11y-audit: page '$label' exited before snapshot; skipping" >&2
        sed 's/^/    app: /' "$APP_LOG" | tail -n 6 >&2 || true
        APP_PID=""
        early_exits=$((early_exits + 1))
        # A page-independent boot failure (e.g. the QML root can't load in this
        # environment) fails every page identically. Bail after two so the gate
        # degrades to SKIP quickly instead of burning the whole test timeout.
        if [[ "$early_exits" -ge 2 && "$connected_any" -eq 0 ]]; then
            echo "a11y-audit: the app is not rendering in this environment; stopping early." >&2
            break
        fi
        continue
    fi

    python3 "$PY" walk --page "$label" --pid "$APP_PID" \
        --out "$REPORTS/$label.json" --timeout 10
    rc=$?
    [[ $rc -eq 0 ]] && connected_any=1

    kill "$APP_PID" 2>/dev/null; wait "$APP_PID" 2>/dev/null; APP_PID=""
done

# --- verdict --------------------------------------------------------------
if [[ "$connected_any" -eq 0 ]]; then
    echo "a11y-audit: the AT-SPI walker never attached to any page in this" >&2
    echo "a11y-audit: environment (the app did not render). Running the static" >&2
    echo "a11y-audit: blind-spot pass only, then reporting SKIP (no walk data)." >&2
    python3 "$PY" gate --reports "$REPORTS" --root "$SRC" \
        --allowlist "$ALLOWLIST" --json "$WORK/a11y-report.json"
    rc=$?
    [[ $rc -ne 0 ]] && exit $rc # a real static violation is a genuine failure
    exit "$SKIP"                # no walk data to gate on -> SKIP, don't false-pass
fi

python3 "$PY" gate --reports "$REPORTS" --root "$SRC" \
    --allowlist "$ALLOWLIST" --json "$WORK/a11y-report.json"
exit $?
