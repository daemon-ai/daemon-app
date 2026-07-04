// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "localization.h"

#include <QCoreApplication>
#include <QLibraryInfo>
#include <QLocale>
#include <QObject>
#include <QTranslator>

namespace i18n {
namespace {

// The currently installed translators, owned here so a language switch can
// remove and replace them. nullptr when the source language (English) is active.
QTranslator* g_appTranslator = nullptr;
QTranslator* g_qtTranslator = nullptr;

void clear(QTranslator*& t) {
    if (t != nullptr) {
        QCoreApplication::removeTranslator(t);
        delete t;
        t = nullptr;
    }
}

bool isSourceLanguage(const QString& code, const QString& lang) {
    return code == QLatin1String("en") || lang == QLatin1String("en");
}

} // namespace

QList<LocaleOption> availableLocales() {
    // Meta options (system default / pseudolocale) localize to the current UI
    // language; a concrete language would keep its own-name (endonym) so users can
    // find their language regardless of the active UI language. en_US is the
    // source language (shown as "English"); no real translations ship yet, so the
    // list is source + the pseudolocale QA catalog until one is added.
    return {
        {QStringLiteral("system"), QObject::tr("System default", "language picker")},
        {QStringLiteral("en"), QStringLiteral("English")},
        {QStringLiteral("pseudo"), QObject::tr("Pseudolocale (i18n test)", "language picker")},
    };
}

QString resolveLocaleName(const QString& code) {
    if (code.isEmpty() || code == QLatin1String("system")) {
        return QLocale::system().name();
    }
    return code;
}

Qt::LayoutDirection applyLocale(const QString& code) {
    clear(g_appTranslator);
    clear(g_qtTranslator);

    const QString name = resolveLocaleName(code);
    const QString lang = name.left(2);

    // App catalog (embedded under ":/i18n"). The synthetic "pseudo" locale loads
    // by exact file; a system/explicit real locale uses QLocale fallback (e.g.
    // fr_CA -> fr). The source language installs no catalog (source text shows).
    auto* appTranslator = new QTranslator;
    bool appLoaded = false;
    if (code == QLatin1String("pseudo")) {
        appLoaded =
            appTranslator->load(QStringLiteral("daemon-app_") + code, QStringLiteral(":/i18n"));
    } else if (!isSourceLanguage(code, lang)) {
        appLoaded = appTranslator->load(QLocale(name), QStringLiteral("daemon-app"),
                                        QStringLiteral("_"), QStringLiteral(":/i18n"));
    }
    if (appLoaded) {
        QCoreApplication::installTranslator(appTranslator);
        g_appTranslator = appTranslator;
    } else {
        delete appTranslator;
    }

    // Qt's own standard strings (e.g. built-in dialog buttons). Best-effort and
    // skipped for the source language and the pseudolocale (no Qt catalog).
    if (!isSourceLanguage(code, lang) && code != QLatin1String("pseudo")) {
        auto* qtTranslator = new QTranslator;
        if (qtTranslator->load(QLocale(name), QStringLiteral("qtbase"), QStringLiteral("_"),
                               QLibraryInfo::path(QLibraryInfo::TranslationsPath))) {
            QCoreApplication::installTranslator(qtTranslator);
            g_qtTranslator = qtTranslator;
        } else {
            delete qtTranslator;
        }
    }

    // Layout direction follows the locale itself, so a future RTL language (ar,
    // he, fa, ...) mirrors the UI automatically. The source language and the
    // (Latin) pseudolocale are always left-to-right.
    if (isSourceLanguage(code, lang) || code == QLatin1String("pseudo")) {
        return Qt::LeftToRight;
    }
    // System or an explicit real locale: use that locale's own direction.
    return QLocale(name).textDirection();
}

} // namespace i18n
