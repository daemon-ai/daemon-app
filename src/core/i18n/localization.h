// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QList>
#include <QString>
#include <Qt>

// Runtime translation loading, shared by the GUI (src/DaemonApp/App/main.cpp +
// Application) and the TUI (src/tui/main.cpp). Toolkit-agnostic: only QtCore
// (QTranslator / QLocale), so the TUI can reuse it without pulling in QtGui.
namespace i18n {

// A selectable UI language. `code` is the persisted key ("system"/"en"/a
// shipped locale code like "de" or "pt_BR"); `label` is the menu text shown to
// the user.
struct LocaleOption {
    QString code;
    QString label;
};

// Languages the app ships, in menu order. "system" follows the OS locale and
// "en" is the source language (no catalog, shows in-source strings).
[[nodiscard]] QList<LocaleOption> availableLocales();

// Resolve a stored code to a concrete locale name ("system" ->
// QLocale::system().name(), e.g. "ar_EG"; otherwise the code itself).
[[nodiscard]] QString resolveLocaleName(const QString& code);

// Install the embedded app catalog (":/i18n/daemon-app_<x>.qm") plus the
// matching Qt base catalog for `code`, removing any previously installed ones.
// Idempotent and safe to call again for live language switching. Returns the
// layout direction the caller should apply to the application.
Qt::LayoutDirection applyLocale(const QString& code);

} // namespace i18n
