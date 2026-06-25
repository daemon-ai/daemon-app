#!/usr/bin/env python3
"""Fill a Qt .ts file with pseudolocalized translations.

Pseudolocalization keeps the app fully usable in English while making missing
or hard-coded (untranslated) strings, truncation, and bidi issues obvious:

  * every translatable character is replaced with an accented look-alike, so any
    on-screen text still rendered in plain ASCII has escaped translation;
  * the string is padded ~40% longer to surface layout truncation;
  * it is wrapped in guillemets so the start/end of each string is visible;
  * Qt placeholders (%1, %n, %L1) and the leading/trailing whitespace that the
    TUI relies on for alignment are preserved verbatim.

The pseudolocale carries no real translations, so `lupdate` skips it. Instead
derive it from a catalog that `update_translations` *does* refresh (the `ar`
seed), so new strings always reach the pseudolocale:

    python3 i18n/pseudolocalize.py i18n/daemon-app_ar.ts i18n/daemon-app_pseudo.ts

A single argument pseudolocalizes a file in place.
"""

from __future__ import annotations

import re
import sys
import xml.etree.ElementTree as ET

# Latin-1/extended look-alikes for ASCII letters.
_MAP = str.maketrans(
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ",
    "\u00e0\u0180\u00e7\u00f0\u00e9\u0192\u011d\u0125\u00ec\u0135"
    "\u0137\u013c\u1e3f\u00f1\u00f3\u00fe\u0071\u0155\u015b\u0167"
    "\u00fa\u1e7d\u1e85\u0078\u00fd\u017e"
    "\u00c0\u0181\u00c7\u00d0\u00c9\u0191\u011c\u0124\u00cc\u0134"
    "\u0136\u013b\u1e3e\u00d1\u00d3\u00de\u0051\u0154\u015a\u0166"
    "\u00da\u1e7c\u1e84\u0058\u00dd\u017d",
)

# Qt placeholders that must survive untouched.
_PLACEHOLDER = re.compile(r"%(?:L?\d+|n|%)")


def _accent(text: str) -> str:
    return text.translate(_MAP)


def pseudo(source: str) -> str:
    """Pseudolocalize while preserving placeholders and edge whitespace."""
    lead = source[: len(source) - len(source.lstrip())]
    trail = source[len(source.rstrip()):]
    core = source[len(lead): len(source) - len(trail)]
    if not core:
        return source

    out: list[str] = []
    pos = 0
    for m in _PLACEHOLDER.finditer(core):
        out.append(_accent(core[pos:m.start()]))
        out.append(m.group(0))  # keep %1 / %n verbatim
        pos = m.end()
    out.append(_accent(core[pos:]))
    body = "".join(out)

    # ~40% padding to expose truncation, using a benign accented filler.
    pad = "\u2009\u00fe\u00e9\u00f1" * max(1, (len(core) * 4) // 30)
    return f"{lead}\u00ab{body} {pad}\u00bb{trail}"


def main(src: str, dst: str) -> int:
    tree = ET.parse(src)
    root = tree.getroot()
    # Drop the target language so `lupdate` skips this file on the next
    # `update_translations` run (which would otherwise clobber the generated
    # pseudo translations). lrelease still compiles it to a .qm.
    root.attrib.pop("language", None)
    filled = 0
    for message in root.iter("message"):
        source = message.find("source")
        translation = message.find("translation")
        if source is None or translation is None or source.text is None:
            continue
        body = pseudo(source.text)
        numerus = list(translation.findall("numerusform"))
        if numerus:
            # Plural form: one <numerusform> per plural category. Pseudo all of
            # them with the (singular) source; we only need them non-empty.
            for form in numerus:
                form.text = body
        else:
            translation.text = body
        # Mark finished so lrelease compiles it into the .qm.
        if "type" in translation.attrib:
            del translation.attrib["type"]
        filled += 1

    tree.write(dst, encoding="utf-8", xml_declaration=True)
    print(f"pseudolocalized {filled} message(s): {src} -> {dst}")
    return 0


if __name__ == "__main__":
    if len(sys.argv) == 2:
        raise SystemExit(main(sys.argv[1], sys.argv[1]))
    if len(sys.argv) == 3:
        raise SystemExit(main(sys.argv[1], sys.argv[2]))
    print(__doc__)
    raise SystemExit(2)
