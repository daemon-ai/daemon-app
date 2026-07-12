#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
# SPDX-FileCopyrightText: 2026 Jarrad Hope
"""Accessibility (AT-SPI) audit gate for the daemon-app GUI.

Two complementary passes, both driven by scripts/a11y-audit.sh and gated by the
same allowlist ratchet (scripts/a11y-audit-allowlist.txt):

1. WALK  -- with the app running under a private AT-SPI bus, walk the exported
   accessibility tree of one rendered page and flag every INTERACTIVE element
   (button, checkbox, tab, entry, slider, menu item, actionable list/tree item,
   ...) that is SHOWING yet has no accessible name, no labelled-by relation and
   no description. A screen reader would announce such an element as an
   anonymous "button"/"entry", so it is a real defect. One JSON report per page.

2. STATIC -- a blind-spot companion for the walker: bespoke `Item`+`MouseArea`
   (or `TapHandler`) click targets that declare no `Accessible.` attached
   property never enter the AT-SPI tree at all, so pass 1 cannot see them. Scan
   src/**/*.qml for that shape and flag it.

`gate` aggregates the per-page WALK reports plus the STATIC pass, subtracts the
allowlist, prints a human summary + a combined JSON artifact, and exits non-zero
when any non-allowlisted violation remains.

The walker half requires pyatspi + a live AT-SPI bus; the launcher exits 77
(ctest SKIP) before ever invoking `walk` when those are absent. `static` and the
allowlist logic are pure-python and always run.

Usage:
    a11y-audit.py walk  --page <id> --pid <pid> --out <file.json> [--timeout S]
    a11y-audit.py static [--root <src>] [--allowlist <file>]
    a11y-audit.py gate  --reports <dir> [--root <src>] [--allowlist <file>]
                        [--json <combined.json>]
"""

from __future__ import annotations

import argparse
import json
import re
import sys
import time
from pathlib import Path

# ---------------------------------------------------------------------------
# Allowlist (shared by both passes) -- ratchet of justified exemptions.
# ---------------------------------------------------------------------------
# Entry forms (one per line; blank + `#` comment lines ignored):
#   <relative-qml-path>                    STATIC: exempt the whole file
#   static|<relative-qml-path>             STATIC: same, explicit form
#   atspi|<page>                           WALK:   exempt every finding on <page>
#   atspi|<page>|<role>                    WALK:   exempt a role on <page>
#   atspi|<page>|<role>|<path>             WALK:   exempt one exact element


def load_allowlist(path: Path) -> set[str]:
    entries: set[str] = set()
    if not path.is_file():
        return entries
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        entries.add(line)
    return entries


# ---------------------------------------------------------------------------
# STATIC pass: MouseArea / TapHandler click targets with no Accessible markup.
# ---------------------------------------------------------------------------
EXCLUDE_DIR_RE = re.compile(r"/(vendor|generated)/|/codec/vendor/")
CLICK_TARGET_RE = re.compile(r"\b(MouseArea|TapHandler)\b")
ACCESSIBLE_RE = re.compile(r"\bAccessible\.")
# A file is exempt from the blind-spot rule when it clearly is not a control:
# no interactive intent (no clicked/tapped/pressed handler wired to the area).
INTERACTIVE_HINT_RE = re.compile(
    r"\bon(Clicked|Tapped|Pressed|DoubleClicked|PressAndHold|Released)\b|"
    r"\b(clicked|tapped|singleTapped|doubleTapped)\s*\("
)


def scan_static(root: Path, repo_root: Path, allow: set[str]) -> list[dict]:
    """Flag QML files that host a click target but declare no Accessible.* markup."""
    findings: list[dict] = []
    for qml in sorted(root.rglob("*.qml")):
        posix = qml.as_posix()
        if EXCLUDE_DIR_RE.search(posix):
            continue
        text = qml.read_text(encoding="utf-8")
        if not CLICK_TARGET_RE.search(text):
            continue
        if ACCESSIBLE_RE.search(text):
            continue  # file participates in the a11y tree -> walker covers it
        if not INTERACTIVE_HINT_RE.search(text):
            continue  # a MouseArea with no click handler is hover/drag chrome
        try:
            rel = qml.relative_to(repo_root).as_posix()
        except ValueError:
            rel = posix
        if rel in allow or f"static|{rel}" in allow:
            continue
        # Report the first click-target line for a stable, greppable anchor.
        lineno = next(
            (i for i, ln in enumerate(text.splitlines(), 1) if CLICK_TARGET_RE.search(ln)),
            1,
        )
        findings.append(
            {
                "pass": "static",
                "path": rel,
                "line": lineno,
                "detail": "click target (MouseArea/TapHandler) with no Accessible.* markup",
            }
        )
    return findings


# ---------------------------------------------------------------------------
# WALK pass: AT-SPI tree walk of one rendered page. Imports pyatspi lazily so
# `static`/`gate` work without the a11y stack installed.
# ---------------------------------------------------------------------------
def _interactive_roles(pyatspi):
    """Roles whose instances a screen reader operates and must be able to name."""
    r = pyatspi
    roles = {
        r.ROLE_PUSH_BUTTON,
        r.ROLE_TOGGLE_BUTTON,
        r.ROLE_CHECK_BOX,
        r.ROLE_RADIO_BUTTON,
        r.ROLE_COMBO_BOX,
        r.ROLE_MENU_ITEM,
        r.ROLE_CHECK_MENU_ITEM,
        r.ROLE_RADIO_MENU_ITEM,
        r.ROLE_PAGE_TAB,
        r.ROLE_SLIDER,
        r.ROLE_SPIN_BUTTON,
        r.ROLE_ENTRY,
        r.ROLE_PASSWORD_TEXT,
        r.ROLE_LINK,
    }
    # Some role enums are version-dependent; add when present.
    for opt in ("ROLE_TOGGLE_BUTTON", "ROLE_LIST_ITEM", "ROLE_TREE_ITEM", "ROLE_TABLE_CELL"):
        roles.add(getattr(r, opt, None))
    roles.discard(None)
    return roles


def _safe(fn, default=None):
    try:
        return fn()
    except Exception:
        return default


def _has_labelled_by(pyatspi, acc) -> bool:
    try:
        rs = acc.getRelationSet()
    except Exception:
        return False
    for rel in rs or []:
        try:
            if rel.getRelationType() == pyatspi.RELATION_LABELLED_BY:
                if rel.getNTargets() > 0:
                    return True
        except Exception:
            continue
    return False


def _is_editable(pyatspi, states) -> bool:
    try:
        return states.contains(pyatspi.STATE_EDITABLE)
    except Exception:
        return False


def _actionable(acc) -> bool:
    try:
        action = acc.queryAction()
        return action.get_nActions() > 0
    except Exception:
        return False


def _nearest_named_ancestor(acc) -> str:
    parent = _safe(acc.get_parent) or _safe(lambda: acc.parent)
    depth = 0
    while parent is not None and depth < 40:
        name = _safe(parent.get_name, "") or ""
        if name.strip():
            role = _safe(parent.get_role_name, "") or ""
            return f"{role}:{name}"
        parent = _safe(parent.get_parent) or _safe(lambda: parent.parent)
        depth += 1
    return ""


def walk(page: str, pid: int, out: Path, timeout: float) -> int:
    try:
        import gi

        gi.require_version("Atspi", "2.0")
        import pyatspi
    except Exception as exc:  # pragma: no cover - environment gate
        print(f"a11y-audit walk: pyatspi unavailable: {exc}", file=sys.stderr)
        return 3

    interactive = _interactive_roles(pyatspi)
    text_roles = {pyatspi.ROLE_ENTRY, pyatspi.ROLE_PASSWORD_TEXT}
    text_roles.add(getattr(pyatspi, "ROLE_TEXT", None))
    text_roles.discard(None)
    item_roles = {
        getattr(pyatspi, n, None) for n in ("ROLE_LIST_ITEM", "ROLE_TREE_ITEM", "ROLE_TABLE_CELL")
    }
    item_roles.discard(None)

    # Locate the app root by pid, retrying while the a11y tree is registered.
    desktop = None
    app_root = None
    deadline = time.time() + timeout
    while time.time() < deadline and app_root is None:
        try:
            desktop = pyatspi.Registry.getDesktop(0)
        except Exception as exc:
            print(f"a11y-audit walk: no AT-SPI desktop: {exc}", file=sys.stderr)
            return 3
        for i in range(_safe(desktop.get_child_count, 0) or 0):
            app = _safe(lambda i=i: desktop.getChildAtIndex(i))
            if app is None:
                continue
            app_pid = _safe(app.get_process_id, -1)
            if pid and app_pid == pid:
                app_root = app
                break
            name = _safe(app.get_name, "") or ""
            if not pid and "daemon-app" in name.lower():
                app_root = app
                break
        if app_root is None:
            time.sleep(0.3)

    if app_root is None:
        print(
            f"a11y-audit walk: app pid={pid} never appeared in the AT-SPI tree",
            file=sys.stderr,
        )
        return 3

    violations: list[dict] = []
    visited = 0

    def visit(acc, path):
        nonlocal visited
        if acc is None or visited > 20000:
            return
        visited += 1
        role = _safe(acc.get_role)
        states = _safe(acc.getState)
        showing = False
        if states is not None:
            showing = _safe(lambda: states.contains(pyatspi.STATE_SHOWING), False)
        name = (_safe(acc.get_name, "") or "").strip()
        desc = (_safe(acc.get_description, "") or "").strip()
        role_name = _safe(acc.get_role_name, "") or str(role)

        flag = role in interactive
        if role in text_roles and not _is_editable(pyatspi, states):
            flag = False  # a non-editable text node is a label, not a control
        if role in item_roles and not _actionable(acc):
            flag = False  # only actionable list/tree/table items are operable
        if flag and showing and not name and not desc and not _has_labelled_by(pyatspi, acc):
            extents = _safe(
                lambda: acc.queryComponent().getExtents(pyatspi.DESKTOP_COORDS)
            )
            geometry = (
                {"x": extents.x, "y": extents.y, "w": extents.width, "h": extents.height}
                if extents is not None
                else None
            )
            violations.append(
                {
                    "pass": "walk",
                    "page": page,
                    "role": role_name,
                    "path": path,
                    "geometry": geometry,
                    "nearest_named_ancestor": _nearest_named_ancestor(acc),
                }
            )

        for i in range(_safe(acc.get_child_count, 0) or 0):
            child = _safe(lambda i=i: acc.getChildAtIndex(i))
            child_role = _safe(child.get_role_name, "?") if child is not None else "?"
            visit(child, f"{path}/{child_role}[{i}]")

    root_role = _safe(app_root.get_role_name, "application") or "application"
    visit(app_root, root_role)

    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(
        json.dumps({"page": page, "pid": pid, "visited": visited, "violations": violations}, indent=2),
        encoding="utf-8",
    )
    print(
        f"a11y-audit walk[{page}]: visited {visited} nodes, "
        f"{len(violations)} raw interactive violation(s) -> {out}"
    )
    return 0


# ---------------------------------------------------------------------------
# GATE: aggregate WALK reports + STATIC pass, subtract the allowlist, verdict.
# ---------------------------------------------------------------------------
def _walk_allowlisted(v: dict, allow: set[str]) -> bool:
    page, role, path = v.get("page", ""), v.get("role", ""), v.get("path", "")
    candidates = {
        f"atspi|{page}",
        f"atspi|{page}|{role}",
        f"atspi|{page}|{role}|{path}",
    }
    return bool(candidates & allow)


def gate(reports_dir: Path, root: Path, repo_root: Path, allow: set[str], json_out: Path | None) -> int:
    walk_raw: list[dict] = []
    if reports_dir.is_dir():
        for rep in sorted(reports_dir.glob("*.json")):
            try:
                data = json.loads(rep.read_text(encoding="utf-8"))
            except Exception as exc:
                print(f"a11y-audit gate: unreadable report {rep}: {exc}", file=sys.stderr)
                continue
            walk_raw.extend(data.get("violations", []))

    walk_viol = [v for v in walk_raw if not _walk_allowlisted(v, allow)]
    walk_exempt = len(walk_raw) - len(walk_viol)

    static_viol = scan_static(root, repo_root, allow)

    combined = {
        "walk": {"total": len(walk_raw), "allowlisted": walk_exempt, "violations": walk_viol},
        "static": {"violations": static_viol},
    }
    if json_out is not None:
        json_out.write_text(json.dumps(combined, indent=2), encoding="utf-8")

    print("=== a11y-audit: AT-SPI walk pass ===")
    if not reports_dir.is_dir() or not any(reports_dir.glob("*.json")):
        print("  (no page reports found; the walker was skipped for this run)")
    for v in walk_viol:
        geo = v.get("geometry")
        geo_s = f" @({geo['x']},{geo['y']} {geo['w']}x{geo['h']})" if geo else ""
        print(
            f"  {v.get('page')}: unnamed {v.get('role')}{geo_s}\n"
            f"      path: {v.get('path')}\n"
            f"      near: {v.get('nearest_named_ancestor') or '(none)'}"
        )
    print(f"  walk violations: {len(walk_viol)} ({walk_exempt} allowlisted)")

    print("=== a11y-audit: static blind-spot pass ===")
    for v in static_viol:
        print(f"  {v['path']}:{v['line']}: {v['detail']}")
    print(f"  static violations: {len(static_viol)}")

    total = len(walk_viol) + len(static_viol)
    if total:
        print(
            f"\na11y-audit: {total} non-allowlisted accessibility violation(s). "
            "Add an Accessible.name / labelled-by / accessibleName, or a justified "
            "entry to scripts/a11y-audit-allowlist.txt.",
            file=sys.stderr,
        )
        return 1
    print("\na11y-audit: no non-allowlisted accessibility violations.")
    return 0


def main() -> int:
    repo_root = Path(__file__).resolve().parent.parent
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="mode", required=True)

    pw = sub.add_parser("walk", help="walk one rendered page's AT-SPI tree")
    pw.add_argument("--page", required=True)
    pw.add_argument("--pid", type=int, required=True)
    pw.add_argument("--out", type=Path, required=True)
    pw.add_argument("--timeout", type=float, default=15.0)

    ps = sub.add_parser("static", help="static blind-spot scan of QML")
    ps.add_argument("--root", type=Path, default=repo_root / "src")
    ps.add_argument("--allowlist", type=Path, default=repo_root / "scripts" / "a11y-audit-allowlist.txt")

    pg = sub.add_parser("gate", help="aggregate reports + static pass, verdict")
    pg.add_argument("--reports", type=Path, required=True)
    pg.add_argument("--root", type=Path, default=repo_root / "src")
    pg.add_argument("--allowlist", type=Path, default=repo_root / "scripts" / "a11y-audit-allowlist.txt")
    pg.add_argument("--json", type=Path, default=None)

    args = parser.parse_args()

    if args.mode == "walk":
        return walk(args.page, args.pid, args.out, args.timeout)
    if args.mode == "static":
        allow = load_allowlist(args.allowlist)
        findings = scan_static(args.root, repo_root, allow)
        for v in findings:
            print(f"{v['path']}:{v['line']}: {v['detail']}")
        print(f"a11y-audit static: {len(findings)} blind-spot violation(s).")
        return 1 if findings else 0
    if args.mode == "gate":
        allow = load_allowlist(args.allowlist)
        return gate(args.reports, args.root, repo_root, allow, args.json)
    return 2


if __name__ == "__main__":
    sys.exit(main())
