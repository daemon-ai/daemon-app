# i18n

Translation catalogs for the GUI (`daemon-app`) and TUI (`daemon-tui`). One
catalog per language backs both front ends, so shared C++ view-model strings are
translated once.

## How it works

- User-facing strings are marked translatable with `qsTr(...)` (QML) and
  `tr(...)` / `QObject::tr(...)` (C++).
- [`cmake/Translations.cmake`](../cmake/Translations.cmake) calls
  `qt_add_translations`, which runs `lupdate` to extract strings into the
  `daemon-app_<code>.ts` files here and `lrelease` to compile each to a `.qm`
  that is embedded into the binaries under the `:/i18n` resource prefix.
- At startup [`src/core/i18n/localization.cpp`](../src/core/i18n/localization.cpp)
  installs a `QTranslator` for the persisted UI language (`UiSettings.language`,
  default: follow the OS locale). The GUI also switches live: changing the
  language in Settings -> Appearance swaps the translator and calls
  `QQmlEngine::retranslate()`. The TUI applies the choice on next launch.

## Locales

`en_US` is the source language (American English): the strings live in the
`qsTr()`/`tr()` literals, so there is intentionally **no** `daemon-app_en*.ts`.
`lupdate -source-language en_US` records the base for translators. Write new UI
copy in US English (e.g. "color", not "colour").

Twelve non-English catalogs ship, listed in `DAEMON_APP_LOCALES`
([`cmake/Translations.cmake`](../cmake/Translations.cmake)) and offered in the
Settings -> Appearance picker by their endonym via `i18n::availableLocales()`:

| Code | Language |
|---------|-----------------------|
| `pt_BR` | Português (Brasil) |
| `id` | Bahasa Indonesia |
| `tr` | Türkçe |
| `hi` | हिन्दी |
| `zh_CN` | 简体中文 |
| `de` | Deutsch |
| `fil` | Filipino |
| `zh_TW` | 繁體中文 |
| `ru` | Русский |
| `es` | Español |
| `ja` | 日本語 |
| `bn` | বাংলা |

Layout direction is derived from the chosen locale, so adding an RTL language
(ar, he, fa, ...) mirrors the UI automatically.

Add a language by generating `daemon-app_<code>.ts` (add `<code>` to
`DAEMON_APP_LOCALES`, then run the refresh step below), listing `<code>` with its
endonym in `i18n::availableLocales()`
([`src/core/i18n/localization.cpp`](../src/core/i18n/localization.cpp)), and
translating the new entries.

## Workflow

Refresh the catalogs after adding or changing `qsTr()`/`tr()` strings (run in
`nix develop`):

```
cmake --build <build-dir> --target update_translations
```

This re-extracts from the whole `src` tree into every listed catalog, adding new
entries as `<translation type="unfinished">`. Translate the new entries (Qt
Linguist or any editor), then rebuild - `lrelease` runs automatically and the
updated `.qm` files are re-embedded.

Preserve Qt placeholders (`%1`, `%2`, `%L1`, `%n`, `%%`) and the leading/trailing
whitespace of each source (the TUI aligns on it). Keep the per-language
`<numerusform>` count `lupdate` generates for plural (`numerus="yes"`) messages,
filling each with the grammatically correct form.

## Validation gates

Two gates guard two distinct desync risks (both are run in CI / `just lint`, and
the parity gate is a `ctest`):

- **`i18n/check_translations.py`** (ctest `i18n_translations`) - completeness +
  cross-locale parity. It fails if any catalog has an incomplete/empty
  translation, a leftover `type="unfinished"`, a placeholder or edge-whitespace
  mismatch against the source, or a `(context, source)` message set that differs
  from the other catalogs. Pure Python, no build; run it on one or many catalogs:

  ```
  python3 i18n/check_translations.py i18n/daemon-app_*.ts
  ```

- **`scripts/i18n-drift.sh`** - source-vs-catalog drift. It re-runs
  `update_translations` and fails if any `daemon-app_*.ts` changed, which means a
  string was added/reworded/removed in the source without refreshing the
  catalogs (a desync the parity checker cannot see, since the catalogs can be
  stale in lockstep). Run it in the devShell:

  ```
  bash scripts/i18n-drift.sh
  ```

The closed loop: add a string -> `i18n-drift.sh` forces `update_translations` ->
the new entry lands `unfinished` in all catalogs -> `check_translations.py` fails
until it is translated -> both gates green. See the i18n policy in
[`AGENTS.md`](../AGENTS.md) for the "re-evaluate all 12 catalogs on any string
change" rule that covers the semantic drift the mechanical gates cannot detect.
