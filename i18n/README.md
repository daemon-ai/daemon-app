# i18n

Translation catalog for the GUI (`daemon-app`) and TUI (`daemon-tui`). One
catalog backs both front ends, so shared C++ view-model strings are translated
once.

## How it works

- User-facing strings are marked translatable with `qsTr(...)` (QML) and
  `tr(...)` / `QObject::tr(...)` (C++).
- [`cmake/Translations.cmake`](../cmake/Translations.cmake) calls
  `qt_add_translations`, which runs `lupdate` to extract strings into the
  `daemon-app_<locale>.ts` files here and `lrelease` to compile each to a `.qm`
  that is embedded into the binaries under the `:/i18n` resource prefix.
- At startup [`src/core/i18n/localization.cpp`](../src/core/i18n/localization.cpp)
  installs a `QTranslator` for the persisted UI language (`UiSettings.language`,
  default: follow the OS locale). The GUI also switches live: changing the
  language in Settings -> Appearance swaps the translator and calls
  `QQmlEngine::retranslate()`. The TUI applies the choice on next launch.

## Locales

- `en_US` - source language (American English). The strings live in the
  `qsTr()`/`tr()` literals, so there is intentionally **no** `daemon-app_en*.ts`;
  `lupdate -source-language en_US` records the base for translators. Write new UI
  copy in US English (e.g. "color", not "colour").
- `pseudo` - generated pseudolocale used to validate i18n coverage, layout, and
  truncation. Every string is accented, padded, and wrapped in guillemets, so
  any plain-ASCII text on screen has escaped translation. Backs `tst_localization`.
  This is the only committed catalog until a real language is added.

No real translations ship yet. Layout direction is derived from the chosen
locale, so adding an RTL language (ar, he, fa, ...) mirrors the UI automatically.

Add a language by dropping in `daemon-app_<code>.ts`, listing `<code>` in
`DAEMON_APP_LOCALES` ([`cmake/Translations.cmake`](../cmake/Translations.cmake))
and in `i18n::availableLocales()`
([`src/core/i18n/localization.cpp`](../src/core/i18n/localization.cpp)), then
running the update step below and translating the entries.

## Workflow

Refresh the `.ts` files after changing or adding strings (run in `nix develop`):

```
cmake --build <build-dir> --target update_translations
```

This re-extracts from the whole `src` tree into every listed catalog (currently
just the pseudolocale, plus any real `daemon-app_<code>.ts` you add). Translate
new entries for real languages in Qt Linguist (or any editor), then rebuild -
`lrelease` runs automatically and the updated `.qm` files are re-embedded.

Regenerate the pseudolocale after an update (it is mechanical, not hand-edited);
it refills every entry from its `<source>`, so the run is idempotent:

```
python3 i18n/pseudolocalize.py i18n/daemon-app_pseudo.ts
```
