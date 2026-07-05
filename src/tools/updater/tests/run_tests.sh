#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 Jarrad Hope
# SPDX-License-Identifier: MPL-2.0
#
# Native integration tests for the daemon-updater helper. Drives the built
# binary through every mode and every contract exit code against real files on
# disk. Requires: bash, coreutils (sha256sum/stat/chmod), a POSIX host.
#
# Usage: run_tests.sh <path-to-daemon-updater>
set -uo pipefail

BIN="${1:?usage: run_tests.sh <path-to-daemon-updater>}"
[ -x "$BIN" ] || { echo "FAIL: helper binary not executable: $BIN"; exit 1; }

WORK="$(mktemp -d)"
trap 'chmod -R u+rwx "$WORK" 2>/dev/null; rm -rf "$WORK"' EXIT

fails=0
pass() { echo "ok  : $*"; }
fail() { echo "FAIL: $*"; fails=$((fails + 1)); }

sha() { sha256sum "$1" | cut -d' ' -f1; }
dev() { stat -c '%d' "$1"; }

# --- 1. rename-swap happy (with --old-aside) --------------------------------
t1() {
	local d; d="$(mktemp -d "$WORK/t1.XXXXXX")"
	printf 'OLD-APP' >"$d/target"; chmod 0755 "$d/target"
	printf 'NEW-APP' >"$d/staged"
	local h; h="$(sha "$d/staged")"
	"$BIN" --mode rename-swap --wait-pid 0 --staged "$d/staged" --target "$d/target" \
		--sha256 "$h" --old-aside "$d/aside" --log "$d/log" >/dev/null 2>&1
	local rc=$?
	[ "$rc" -eq 0 ] || { fail "t1 rename-swap exit=$rc (want 0)"; return; }
	[ "$(cat "$d/target")" = "NEW-APP" ] || { fail "t1 target not swapped"; return; }
	[ -x "$d/target" ] || { fail "t1 target lost +x"; return; }
	[ -f "$d/aside" ] && [ "$(cat "$d/aside")" = "OLD-APP" ] ||
		{ fail "t1 old version not parked at --old-aside"; return; }
	[ ! -e "$d/staged" ] || { fail "t1 staged path should be consumed"; return; }
	pass "rename-swap happy path (swap + +x + old parked aside)"
}

# --- 2. rename-swap happy (empty --old-aside => delete old) -----------------
t2() {
	local d; d="$(mktemp -d "$WORK/t2.XXXXXX")"
	printf 'OLD' >"$d/target"; chmod 0755 "$d/target"
	printf 'NEWBYTES' >"$d/staged"
	local h; h="$(sha "$d/staged")"
	"$BIN" --mode rename-swap --wait-pid 0 --staged "$d/staged" --target "$d/target" \
		--sha256 "$h" >/dev/null 2>&1
	local rc=$?
	[ "$rc" -eq 0 ] || { fail "t2 exit=$rc (want 0)"; return; }
	[ "$(cat "$d/target")" = "NEWBYTES" ] || { fail "t2 target not swapped"; return; }
	pass "rename-swap happy path (empty --old-aside)"
}

# --- 3. rename-swap cross-filesystem => exit 2, nothing mutated -------------
t3() {
	[ -d /dev/shm ] && [ -w /dev/shm ] || { pass "rename-swap cross-fs SKIPPED (no writable /dev/shm)"; return; }
	local d; d="$(mktemp -d "$WORK/t3.XXXXXX")"          # on $TMPDIR
	local s; s="$(mktemp -d /dev/shm/upd.XXXXXX)"        # on tmpfs
	printf 'OLD' >"$d/target"; chmod 0755 "$d/target"
	printf 'NEW' >"$s/staged"
	if [ "$(dev "$d")" = "$(dev "$s")" ]; then
		rm -rf "$s"; pass "rename-swap cross-fs SKIPPED (same device)"; return
	fi
	local h; h="$(sha "$s/staged")"
	"$BIN" --mode rename-swap --wait-pid 0 --staged "$s/staged" --target "$d/target" \
		--sha256 "$h" --log "$d/log" >/dev/null 2>&1
	local rc=$?
	rm -rf "$s"
	[ "$rc" -eq 2 ] || { fail "t3 cross-fs exit=$rc (want 2)"; return; }
	[ "$(cat "$d/target")" = "OLD" ] || { fail "t3 target mutated on a cross-fs refusal"; return; }
	pass "rename-swap cross-filesystem refused (exit 2, nothing mutated)"
}

# --- 4. two-move happy (directories) ----------------------------------------
t4() {
	local d; d="$(mktemp -d "$WORK/t4.XXXXXX")"
	mkdir -p "$d/target.app"; printf 'v1' >"$d/target.app/Info"
	mkdir -p "$d/staged.app"; printf 'v2' >"$d/staged.app/Info"
	"$BIN" --mode two-move --wait-pid 0 --staged "$d/staged.app" --target "$d/target.app" \
		--old-aside "$d/old.app" --log "$d/log" >/dev/null 2>&1
	local rc=$?
	[ "$rc" -eq 0 ] || { fail "t4 two-move exit=$rc (want 0)"; return; }
	[ "$(cat "$d/target.app/Info")" = "v2" ] || { fail "t4 target not swapped"; return; }
	[ -d "$d/old.app" ] && [ "$(cat "$d/old.app/Info")" = "v1" ] ||
		{ fail "t4 old bundle not parked aside"; return; }
	pass "two-move happy path (directory swap + old parked aside)"
}

# --- 5. two-move induced second-move failure => rollback, exit 3 ------------
t5() {
	if [ "$(id -u)" -eq 0 ]; then pass "two-move rollback SKIPPED (running as root; perms ignored)"; return; fi
	local d; d="$(mktemp -d "$WORK/t5.XXXXXX")"
	mkdir -p "$d/target.app"; printf 'orig' >"$d/target.app/Info"
	# Put the staged bundle in a read-only parent so step 2 (rename staged ->
	# target) cannot remove it from its directory and fails, after step 1
	# (target -> aside) has already succeeded => rollback path.
	mkdir -p "$d/ro"; mkdir -p "$d/ro/staged.app"; printf 'new' >"$d/ro/staged.app/Info"
	chmod 0500 "$d/ro"
	"$BIN" --mode two-move --wait-pid 0 --staged "$d/ro/staged.app" --target "$d/target.app" \
		--old-aside "$d/old.app" --log "$d/log" >/dev/null 2>&1
	local rc=$?
	chmod 0700 "$d/ro"
	[ "$rc" -eq 3 ] || { fail "t5 induced-failure exit=$rc (want 3)"; return; }
	[ -d "$d/target.app" ] && [ "$(cat "$d/target.app/Info")" = "orig" ] ||
		{ fail "t5 rollback did not restore the original target"; return; }
	pass "two-move induced failure rolled back (exit 3, original restored)"
}

# --- 6. sha256 mismatch => exit 2, nothing mutated --------------------------
t6() {
	local d; d="$(mktemp -d "$WORK/t6.XXXXXX")"
	printf 'OLD' >"$d/target"; chmod 0755 "$d/target"
	printf 'NEW' >"$d/staged"
	"$BIN" --mode rename-swap --wait-pid 0 --staged "$d/staged" --target "$d/target" \
		--sha256 "0000000000000000000000000000000000000000000000000000000000000000" \
		--log "$d/log" >/dev/null 2>&1
	local rc=$?
	[ "$rc" -eq 2 ] || { fail "t6 sha mismatch exit=$rc (want 2)"; return; }
	[ "$(cat "$d/target")" = "OLD" ] || { fail "t6 target mutated on a digest mismatch"; return; }
	pass "sha256 mismatch refused (exit 2, nothing mutated)"
}

# --- 7. wait-pid: applies only after the target exits -----------------------
t7() {
	local d; d="$(mktemp -d "$WORK/t7.XXXXXX")"
	printf 'OLD' >"$d/target"; chmod 0755 "$d/target"
	printf 'NEW' >"$d/staged"
	local h; h="$(sha "$d/staged")"
	sleep 2 & local pid=$!
	local start; start="$(date +%s)"
	"$BIN" --mode rename-swap --wait-pid "$pid" --timeout 10 \
		--staged "$d/staged" --target "$d/target" --sha256 "$h" --log "$d/log" >/dev/null 2>&1
	local rc=$?
	local elapsed=$(( $(date +%s) - start ))
	[ "$rc" -eq 0 ] || { fail "t7 wait-pid exit=$rc (want 0)"; return; }
	[ "$(cat "$d/target")" = "NEW" ] || { fail "t7 target not swapped after wait"; return; }
	[ "$elapsed" -ge 1 ] || { fail "t7 helper did not wait for the pid (elapsed=${elapsed}s)"; return; }
	pass "wait-pid waited for target exit then applied (elapsed=${elapsed}s)"
}

# --- 8. wait-pid timeout => exit 5, nothing mutated -------------------------
t8() {
	local d; d="$(mktemp -d "$WORK/t8.XXXXXX")"
	printf 'OLD' >"$d/target"; chmod 0755 "$d/target"
	printf 'NEW' >"$d/staged"
	local h; h="$(sha "$d/staged")"
	sleep 30 & local pid=$!
	"$BIN" --mode rename-swap --wait-pid "$pid" --timeout 1 \
		--staged "$d/staged" --target "$d/target" --sha256 "$h" --log "$d/log" >/dev/null 2>&1
	local rc=$?
	kill "$pid" 2>/dev/null; wait "$pid" 2>/dev/null
	[ "$rc" -eq 5 ] || { fail "t8 timeout exit=$rc (want 5)"; return; }
	[ "$(cat "$d/target")" = "OLD" ] || { fail "t8 target mutated after a wait-pid timeout"; return; }
	pass "wait-pid timeout refused (exit 5, nothing mutated)"
}

# --- 9. relaunch: applied binary is spawned detached ------------------------
t9() {
	local d; d="$(mktemp -d "$WORK/t9.XXXXXX")"
	printf 'OLD' >"$d/target"; chmod 0755 "$d/target"
	# The staged "new app" is a script that drops a sentinel when relaunched.
	cat >"$d/staged" <<EOF
#!/usr/bin/env bash
printf relaunched >"$d/sentinel"
EOF
	chmod 0755 "$d/staged"
	local h; h="$(sha "$d/staged")"
	"$BIN" --mode rename-swap --wait-pid 0 --staged "$d/staged" --target "$d/target" \
		--sha256 "$h" --log "$d/log" --relaunch -- "$d/target" >/dev/null 2>&1
	local rc=$?
	[ "$rc" -eq 0 ] || { fail "t9 relaunch exit=$rc (want 0)"; return; }
	local i=0
	while [ ! -f "$d/sentinel" ] && [ "$i" -lt 50 ]; do sleep 0.1; i=$((i + 1)); done
	[ -f "$d/sentinel" ] && [ "$(cat "$d/sentinel")" = "relaunched" ] ||
		{ fail "t9 relaunched binary did not write its sentinel"; return; }
	pass "relaunch spawned the applied binary detached (sentinel written)"
}

# --- 10. bad args => exit 5 -------------------------------------------------
t10() {
	"$BIN" --mode bogus --staged /x --target /y >/dev/null 2>&1
	[ $? -eq 5 ] || { fail "t10 unknown --mode did not exit 5"; return; }
	"$BIN" --mode rename-swap --wait-pid 0 --staged /x >/dev/null 2>&1
	[ $? -eq 5 ] || { fail "t10 missing --target did not exit 5"; return; }
	pass "bad arguments refused (exit 5)"
}

t1; t2; t3; t4; t5; t6; t7; t8; t9; t10

if [ "$fails" -ne 0 ]; then
	echo "== daemon-updater tests: $fails failure(s) =="
	exit 1
fi
echo "== daemon-updater tests: all passed =="
