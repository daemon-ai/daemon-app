#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
# SPDX-FileCopyrightText: 2026 Jarrad Hope
"""Validate the shipped Qt translation catalogs (daemon-app_<code>.ts).

This is the i18n completeness + parity gate (ctest `i18n_translations`). It runs
on the source `.ts` files directly -- no build artifact -- and mirrors the QA
role the retired pseudolocale used to serve. Each translator subagent also runs
it standalone on its own catalog before reporting done.

Two classes of check:

  * Cross-locale parity (only when >=2 catalogs are passed): every catalog must
    carry an identical set of (context, source) messages. This catches a string
    that landed in some catalogs but is missing from others -- e.g. a partial
    `update_translations` run.

  * Per message (every catalog): the translation is non-empty (every
    <numerusform> non-empty for plurals); no leftover `type="unfinished"` (or
    other non-final translation state); the multiset of Qt placeholders
    (`%1`, `%2`, `%L1`, `%n`, `%%`) in the translation equals the source's; and
    the leading/trailing whitespace of the source is preserved verbatim (the TUI
    aligns on it).

Usage:
    check_translations.py <daemon-app_xx.ts> [<daemon-app_yy.ts> ...]

Exits non-zero and prints each problem as `path [context/source]: <reason>` when
any catalog is incomplete, inconsistent, or structurally wrong.
"""

from __future__ import annotations

import re
import sys
import xml.etree.ElementTree as ET
from collections import Counter
from pathlib import Path

# Qt placeholders that must survive translation unchanged: %1, %2, ... (and the
# locale-formatted %L1, %L2, ...), the plural %n, and the literal %% escape.
_PLACEHOLDER = re.compile(r"%(?:L?\d+|n|%)")

_LEADING_WS = re.compile(r"^\s*")
_TRAILING_WS = re.compile(r"\s*$")


def _content(el: ET.Element | None) -> str | None:
    """Full text content of an element, including any child markup in order.

    Qt encodes non-XML control bytes (e.g. the ANSI ESC 0x1b) as <byte value=".."/>
    child elements, so a `<source>`/`<translation>` can be mixed content whose
    leading run is a child rather than text. Reconstruct text + serialized
    children (+ tails) so such messages are compared on their real content
    instead of `Element.text`, which would be None for a child-leading value.
    Returns None only when the element itself is absent.
    """
    if el is None:
        return None
    parts = [el.text or ""]
    for child in el:
        parts.append(ET.tostring(child, encoding="unicode"))
        parts.append(child.tail or "")
    return "".join(parts)


def _placeholders(text: str) -> Counter[str]:
    return Counter(_PLACEHOLDER.findall(text or ""))


def _leading_ws(text: str) -> str:
    return _LEADING_WS.match(text or "").group(0)


def _trailing_ws(text: str) -> str:
    return _TRAILING_WS.search(text or "").group(0)


class Problem:
    __slots__ = ("path", "key", "reason")

    def __init__(self, path: str, key: str, reason: str) -> None:
        self.path = path
        self.key = key
        self.reason = reason

    def __str__(self) -> str:
        return f"{self.path} [{self.key}]: {self.reason}"


def _iter_messages(root: ET.Element):
    """Yield (context_name, message_element) for every <message>."""
    for ctx in root.findall("context"):
        name = ctx.findtext("name") or "<no-context>"
        for message in ctx.findall("message"):
            yield name, message


def check_catalog(path: Path) -> tuple[set[tuple[str, str]], list[Problem]]:
    """Validate one catalog. Returns its (context, source) key set + problems."""
    problems: list[Problem] = []
    try:
        root = ET.parse(path).getroot()
    except ET.ParseError as exc:
        return set(), [Problem(str(path), "<xml>", f"not well-formed XML: {exc}")]

    keys: set[tuple[str, str]] = set()
    for context, message in _iter_messages(root):
        source_el = message.find("source")
        source = _content(source_el)
        if source is None:
            problems.append(Problem(str(path), context, "message has no <source>"))
            continue

        key = (context, source)
        short = source if len(source) <= 60 else source[:57] + "..."
        label = f"{context}/{short}"
        if key in keys:
            problems.append(Problem(str(path), label, "duplicate (context, source)"))
        keys.add(key)

        translation = message.find("translation")
        if translation is None:
            problems.append(Problem(str(path), label, "message has no <translation>"))
            continue

        state = translation.get("type")
        if state is not None:
            problems.append(
                Problem(str(path), label, f'translation still marked type="{state}"')
            )

        src_ph = _placeholders(source)
        src_lead = _leading_ws(source)
        src_trail = _trailing_ws(source)

        numerus = translation.findall("numerusform")
        forms = [(f"numerusform[{i}]", _content(form)) for i, form in enumerate(numerus)]
        if not numerus:
            forms = [("translation", _content(translation))]

        for form_label, text in forms:
            if text is None or text == "":
                problems.append(Problem(str(path), label, f"{form_label} is empty"))
                continue
            if _placeholders(text) != src_ph:
                problems.append(
                    Problem(
                        str(path),
                        label,
                        f"{form_label} placeholder mismatch: "
                        f"source={sorted(src_ph.elements())} "
                        f"translation={sorted(_placeholders(text).elements())}",
                    )
                )
            if _leading_ws(text) != src_lead:
                problems.append(
                    Problem(str(path), label, f"{form_label} leading whitespace not preserved")
                )
            if _trailing_ws(text) != src_trail:
                problems.append(
                    Problem(str(path), label, f"{form_label} trailing whitespace not preserved")
                )

    return keys, problems


def main(argv: list[str]) -> int:
    paths = [Path(a) for a in argv]
    if not paths:
        print(__doc__)
        return 2

    missing = [p for p in paths if not p.is_file()]
    if missing:
        for p in missing:
            print(f"error: no such file: {p}", file=sys.stderr)
        return 2

    all_problems: list[Problem] = []
    key_sets: dict[Path, set[tuple[str, str]]] = {}
    for path in paths:
        keys, problems = check_catalog(path)
        key_sets[path] = keys
        all_problems.extend(problems)

    # Cross-locale parity: only meaningful with more than one catalog. Compare
    # every catalog against the union so a message missing anywhere is reported.
    if len(paths) > 1:
        union: set[tuple[str, str]] = set()
        for keys in key_sets.values():
            union |= keys
        for path, keys in key_sets.items():
            for context, source in sorted(union - keys):
                short = source if len(source) <= 60 else source[:57] + "..."
                all_problems.append(
                    Problem(str(path), f"{context}/{short}", "message missing from this catalog")
                )

    if all_problems:
        for problem in all_problems:
            print(problem)
        print(
            f"\nFAIL: {len(all_problems)} problem(s) across {len(paths)} catalog(s).",
            file=sys.stderr,
        )
        return 1

    total = sum(len(k) for k in key_sets.values())
    print(f"OK: {len(paths)} catalog(s), {total} message(s) checked, all complete and consistent.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
