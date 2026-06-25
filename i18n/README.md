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

- `en` - source language (English); no `.ts` file.
- `pseudo` - generated pseudolocale used to validate i18n coverage, layout, and
  truncation. Every string is accented, padded, and wrapped in guillemets, so
  any plain-ASCII text on screen has escaped translation. Backs `tst_localization`.
- `ar` - Arabic, a seed right-to-left locale for translators to fill (RTL
  mirroring is driven by the locale, independent of how complete it is).

Add a locale by listing its code in `DAEMON_APP_LOCALES`
([`cmake/Translations.cmake`](../cmake/Translations.cmake)) and in
`i18n::availableLocales()`
([`src/core/i18n/localization.cpp`](../src/core/i18n/localization.cpp)), then
running the update step below.

## Workflow

Refresh the `.ts` files after changing or adding strings (run in `nix develop`):

```
cmake --build <build-dir> --target update_translations
```

This re-extracts from the whole `src` tree. Translate the new entries in Qt
Linguist (or any editor), then rebuild - `lrelease` runs automatically and the
updated `.qm` files are re-embedded.

Regenerate the pseudolocale after an update (it is mechanical, not hand-edited):

```
python3 i18n/pseudolocalize.py i18n/daemon-app_pseudo.ts
```
