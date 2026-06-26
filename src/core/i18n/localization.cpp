#include "localization.h"

#include <QCoreApplication>
#include <QLibraryInfo>
#include <QLocale>
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
    return {
        {QStringLiteral("system"), QStringLiteral("System default")},
        {QStringLiteral("en"), QStringLiteral("English")},
        {QStringLiteral("ar"),
         QStringLiteral("\u0627\u0644\u0639\u0631\u0628\u064a\u0629 (Arabic)")},
        {QStringLiteral("pseudo"), QStringLiteral("Pseudolocale (i18n test)")},
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

    // App catalog (embedded under ":/i18n"). The synthetic "pseudo" locale and
    // the explicit "ar" seed load by exact file; a system/explicit real locale
    // uses QLocale fallback (e.g. ar_EG -> ar).
    auto* appTranslator = new QTranslator;
    bool appLoaded = false;
    if (code == QLatin1String("pseudo") || code == QLatin1String("ar")) {
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

    if (code == QLatin1String("ar")) {
        return Qt::RightToLeft;
    }
    if (code.isEmpty() || code == QLatin1String("system")) {
        return QLocale(name).textDirection();
    }
    return Qt::LeftToRight;
}

} // namespace i18n
