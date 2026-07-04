#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
# SPDX-FileCopyrightText: 2026 Jarrad Hope
"""Guard against hardcoded, untranslated user-facing strings in QML.

The app internationalizes every user-visible string through Qt Linguist:
`qsTr()` in QML and `tr()` / `QObject::tr()` in C++/TUI. This scanner catches
the common QML regression -- a user-facing property assigned a raw string
literal that never flows through `qsTr()` -- and fails the build so new strings
stay translatable.

It is deliberately conservative (property-anchored, single-line) to avoid false
positives; C++ coverage is left to review plus the pseudolocale QA pass. Known,
legitimate exceptions live in scripts/i18n-audit-allowlist.txt.

Usage:
    i18n-audit.py [--root <dir>] [--allowlist <file>]

Exits non-zero (and prints each offender as `path:line: <text>`) when an
unmarked user-facing string is found.
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

# Properties whose value is rendered to the user. A raw string literal assigned
# to one of these should be wrapped in qsTr().
USER_FACING_PROPS = (
    "text",
    "title",
    "placeholderText",
    "tooltipText",
    "toolTipText",
    "description",
    "label",
    "header",
    "heading",
    "hint",
    "caption",
    "nameFilters",
    "buttonText",
)

PROP_RE = re.compile(r"^\s*(?:readonly\s+property\s+\w+\s+)?(" + "|".join(USER_FACING_PROPS) + r")\s*:\s*(.+?)\s*$")
STRING_LITERAL_RE = re.compile(r'"(?:[^"\\]|\\.)*"')
# Escape sequences inside a literal (\uXXXX glyphs, \xXX, \n, ...). Stripped
# before the word test so a glyph like "\u25cf " is not misread as the word "cf".
ESCAPE_RE = re.compile(r"\\u[0-9a-fA-F]{4}|\\x[0-9a-fA-F]{2}|\\.")
# A literal counts as human copy only if it has two consecutive ASCII letters.
HAS_WORD_RE = re.compile(r"[A-Za-z]{2,}")
# Identifier / key shaped tokens (no spaces): property keys, enum/family codes,
# setting paths, camelCase argument names. These appear as function arguments or
# comparison operands, not display copy, so they are not translation defects.
IDENTIFIER_RE = re.compile(r"[\w./-]+")


def is_key_like(inner: str) -> bool:
    """True when a literal looks like a key/identifier rather than display copy.

    Copy tends to contain spaces or be a Title/ALLCAPS word; keys are
    lowercase/camelCase words, dotted enum codes, or slash-separated setting
    paths (e.g. "systemPrompt", "memory.tier", "chat/systemPrompt").
    """
    if not IDENTIFIER_RE.fullmatch(inner):
        return False  # contains spaces/punctuation -> treat as copy
    if any(ch in inner for ch in "./_-"):
        return True
    return inner[:1].islower()

# Values that are locale-aware or non-copy by construction; skip the whole RHS.
SKIP_VALUE_SUBSTRINGS = (
    "qsTr(",
    "qsTranslate(",
    "formatDateTime",
    "Qt.formatDate",
    "Qt.formatTime",
    "FontIcons.",
    "Theme.",
)

EXCLUDE_DIR_RE = re.compile(r"/(vendor|generated)/|/codec/vendor/")


def load_allowlist(path: Path) -> set[str]:
    """Allowlist entries are `<relative-path>` (skip file) or `<relative-path>|<literal>`.

    Blank lines and `#` comments are ignored.
    """
    entries: set[str] = set()
    if not path.is_file():
        return entries
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        entries.add(line)
    return entries


def is_allowlisted(rel_path: str, literal: str, allow: set[str]) -> bool:
    return rel_path in allow or f"{rel_path}|{literal}" in allow


def scan_file(path: Path, rel_path: str, allow: set[str]) -> list[tuple[int, str]]:
    offenders: list[tuple[int, str]] = []
    for lineno, line in enumerate(path.read_text(encoding="utf-8").splitlines(), start=1):
        stripped = line.lstrip()
        if stripped.startswith("//"):
            continue
        m = PROP_RE.match(line)
        if not m:
            continue
        value = m.group(2)
        if any(sub in value for sub in SKIP_VALUE_SUBSTRINGS):
            continue
        for literal in STRING_LITERAL_RE.findall(value):
            inner = literal[1:-1]
            if not HAS_WORD_RE.search(ESCAPE_RE.sub(" ", inner)):
                continue
            if is_key_like(inner):
                continue
            if is_allowlisted(rel_path, literal, allow):
                continue
            offenders.append((lineno, literal))
    return offenders


def main() -> int:
    repo_root = Path(__file__).resolve().parent.parent
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=repo_root / "src")
    parser.add_argument(
        "--allowlist", type=Path, default=repo_root / "scripts" / "i18n-audit-allowlist.txt"
    )
    args = parser.parse_args()

    allow = load_allowlist(args.allowlist)
    root: Path = args.root
    if not root.is_dir():
        print(f"i18n-audit: root not found: {root}", file=sys.stderr)
        return 2

    total = 0
    for qml in sorted(root.rglob("*.qml")):
        posix = qml.as_posix()
        if EXCLUDE_DIR_RE.search(posix):
            continue
        try:
            rel_path = qml.relative_to(repo_root).as_posix()
        except ValueError:
            rel_path = qml.as_posix()
        for lineno, literal in scan_file(qml, rel_path, allow):
            print(f"{rel_path}:{lineno}: unmarked user-facing string {literal}")
            total += 1

    if total:
        print(
            f"\ni18n-audit: {total} unmarked user-facing string(s) found. "
            "Wrap them in qsTr(), or add a justified entry to "
            "scripts/i18n-audit-allowlist.txt.",
            file=sys.stderr,
        )
        return 1
    print("i18n-audit: no unmarked user-facing QML strings found.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
