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
    // "System default" localizes to the current UI language; every concrete
    // language keeps its own-name (endonym) so users can find their language
    // regardless of the active UI language. en_US is the source language (shown
    // as "English"). The shipped catalogs follow, in the same order listed in
    // DAEMON_APP_LOCALES (cmake/Translations.cmake) - keep the two in sync.
    return {
        {QStringLiteral("system"), QObject::tr("System default", "language picker")},
        {QStringLiteral("en"), QStringLiteral("English")},
        {QStringLiteral("pt_BR"), QStringLiteral("Português (Brasil)")},
        {QStringLiteral("id"), QStringLiteral("Bahasa Indonesia")},
        {QStringLiteral("tr"), QStringLiteral("Türkçe")},
        {QStringLiteral("hi"), QStringLiteral("हिन्दी")},
        {QStringLiteral("zh_CN"), QStringLiteral("简体中文")},
        {QStringLiteral("de"), QStringLiteral("Deutsch")},
        {QStringLiteral("fil"), QStringLiteral("Filipino")},
        {QStringLiteral("zh_TW"), QStringLiteral("繁體中文")},
        {QStringLiteral("ru"), QStringLiteral("Русский")},
        {QStringLiteral("es"), QStringLiteral("Español")},
        {QStringLiteral("ja"), QStringLiteral("日本語")},
        {QStringLiteral("bn"), QStringLiteral("বাংলা")},
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

    // App catalog (embedded under ":/i18n"). A system/explicit real locale uses
    // QLocale fallback (e.g. fr_CA -> fr, zh_CN -> zh_CN then zh). The source
    // language installs no catalog (the in-source English text shows).
    auto* appTranslator = new QTranslator;
    bool appLoaded = false;
    if (!isSourceLanguage(code, lang)) {
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
    // skipped for the source language (no Qt catalog needed).
    if (!isSourceLanguage(code, lang)) {
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
    // he, fa, ...) mirrors the UI automatically. The source language is always
    // left-to-right.
    if (isSourceLanguage(code, lang)) {
        return Qt::LeftToRight;
    }
    // System or an explicit real locale: use that locale's own direction.
    return QLocale(name).textDirection();
}

} // namespace i18n
