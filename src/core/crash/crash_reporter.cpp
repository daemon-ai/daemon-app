// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "crash/crash_reporter.h"

#include <cstdlib>
#include <QCoreApplication>
#include <QDir>
#include <QSettings>
#include <QStandardPaths>
#include <stdexcept>

namespace {
// The QSettings key the last-known crash consent is cached under (shared with the GUI/TUI mains).
constexpr auto kConsentGroupKey = "crash/consent";
QSettings crashSettings() {
    return QSettings(QStringLiteral("daemon-app"), QStringLiteral("daemon-app"));
}
} // namespace

// Crash reporting is only compiled when the sentry-native target is linked (desktop, non-wasm — see
// cmake/Dependencies.cmake, which defines DAEMON_APP_HAVE_SENTRY on this target). Everywhere else
// the whole module degrades to no-ops and never references <sentry.h>.
#if defined(DAEMON_APP_HAVE_SENTRY) && !defined(Q_OS_WASM)
#define DAEMON_APP_CRASH_ENABLED 1
#include <sentry.h>
#else
#define DAEMON_APP_CRASH_ENABLED 0
#endif

// The product default DSN, compiled in from the CMake cache var. Empty ⇒ disabled. Kept as a macro
// so an unset build (fork / no -DDAEMON_APP_SENTRY_DSN) compiles crash reporting out by default.
#ifndef DAEMON_APP_SENTRY_DSN
#define DAEMON_APP_SENTRY_DSN ""
#endif

namespace crash {

namespace {

#if DAEMON_APP_CRASH_ENABLED
const char* levelString(Level level) {
    switch (level) {
    case Level::Debug:
        return "debug";
    case Level::Info:
        return "info";
    case Level::Warning:
        return "warning";
    case Level::Error:
        return "error";
    case Level::Fatal:
        return "fatal";
    }
    return "info";
}
#endif

} // namespace

QString compiledDefaultDsn() {
    return QString::fromUtf8(DAEMON_APP_SENTRY_DSN);
}

QString effectiveDsn() {
    // The env override wins (lets a build with a compiled default still be redirected / disabled at
    // runtime); otherwise the compiled default.
    return qEnvironmentVariable("DAEMON_APP_SENTRY_DSN", compiledDefaultDsn());
}

QString defaultDatabasePath() {
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    // A dedicated subdir so it never collides with the SQLite cache or settings that also live
    // under AppDataLocation.
    return QDir(base).filePath(QStringLiteral("crash-db"));
}

QString defaultHandlerPath() {
    // Co-located with the app binary, exactly like the node binary (see local_daemon_launcher.cpp):
    // every desktop package ships crashpad_handler next to the executable.
#ifdef Q_OS_WIN
    const QString exe = QStringLiteral("crashpad_handler.exe");
#else
    const QString exe = QStringLiteral("crashpad_handler");
#endif
    return QDir(QCoreApplication::applicationDirPath()).filePath(exe);
}

bool consentCached() {
    return crashSettings().value(QLatin1String(kConsentGroupKey), false).toBool();
}

void triggerTestCrash(const QString& kind) {
    if (kind == QLatin1String("segv")) {
        // Deliberate null dereference (SIGSEGV) — the canonical native-crash corpus case.
        volatile int* p = nullptr;
        *p = 42; // NOLINT(clang-analyzer-core.NullDereference): intentional crash-test trigger
    } else if (kind == QLatin1String("abort")) {
        std::abort();
    } else if (kind == QLatin1String("throw")) {
        throw std::runtime_error("DAEMON_APP_CRASH_TEST=throw");
    }
    // Unknown/empty: no-op (never crash a normal run).
}

void persistConsent(bool enabled) {
    // Live gate first (so a crash between this and the next line still honors the new decision),
    // then the durable cache the next startup reads. giveConsent/revokeConsent are no-ops when
    // crash reporting is compiled out / not yet armed; the QSettings write always happens.
    if (enabled) {
        giveConsent();
    } else {
        revokeConsent();
    }
    crashSettings().setValue(QLatin1String(kConsentGroupKey), enabled);
}

#if DAEMON_APP_CRASH_ENABLED

Guard::~Guard() {
    if (m_armed) {
        // Flushes any pending envelope and shuts the transport down.
        sentry_close();
    }
}

Guard& Guard::operator=(Guard&& other) noexcept {
    if (this != &other) {
        if (m_armed) {
            sentry_close();
        }
        m_armed = other.m_armed;
        other.m_armed = false;
    }
    return *this;
}

Guard init(const QString& component, const QString& release, const QString& databasePath,
           const QString& handlerPath, const QString& dsn) {
    if (dsn.isEmpty()) {
        return Guard{}; // disabled — no DSN configured.
    }

    // Ensure the database dir exists (sentry resolves it to an absolute path inside sentry_init).
    QDir().mkpath(databasePath);

    sentry_options_t* options = sentry_options_new();
    sentry_options_set_dsn(options, dsn.toUtf8().constData());
    sentry_options_set_release(options, release.toUtf8().constData());
    sentry_options_set_database_path(options,
                                     QDir::toNativeSeparators(databasePath).toUtf8().constData());
    sentry_options_set_handler_path(options,
                                    QDir::toNativeSeparators(handlerPath).toUtf8().constData());
    // Arm capture but hold every upload until the user grants consent (the dedicated crash consent,
    // separate from telemetry). The consent seam flips this via giveConsent()/revokeConsent().
    sentry_options_set_require_user_consent(options, 1);
    // A minidump is process memory; consent gates its upload. Never opt into extra PII.
    sentry_options_set_symbolize_stacktraces(options, 1);

    if (sentry_init(options) != 0) {
        return Guard{};
    }

    // Tag the process role so a GUI crash and a TUI crash are distinguishable in one project (the
    // node processes tag `component` the same way).
    sentry_set_tag("component", component.toUtf8().constData());
    return Guard{true};
}

void giveConsent() {
    sentry_user_consent_give();
}

void revokeConsent() {
    sentry_user_consent_revoke();
}

void addBreadcrumb(Level level, const QString& category, const QString& message) {
    sentry_value_t crumb = sentry_value_new_breadcrumb("default", message.toUtf8().constData());
    sentry_value_set_by_key(crumb, "level", sentry_value_new_string(levelString(level)));
    if (!category.isEmpty()) {
        sentry_value_set_by_key(crumb, "category",
                                sentry_value_new_string(category.toUtf8().constData()));
    }
    sentry_add_breadcrumb(crumb);
}

#else // !DAEMON_APP_CRASH_ENABLED — no-op stubs (wasm / no sentry target).

Guard::~Guard() = default;

Guard& Guard::operator=(Guard&& other) noexcept {
    if (this != &other) {
        m_armed = other.m_armed;
        other.m_armed = false;
    }
    return *this;
}

Guard init(const QString& /*component*/, const QString& /*release*/,
           const QString& /*databasePath*/, const QString& /*handlerPath*/,
           const QString& /*dsn*/) {
    return Guard{};
}

void giveConsent() {}
void revokeConsent() {}
void addBreadcrumb(Level /*level*/, const QString& /*category*/, const QString& /*message*/) {}

#endif // DAEMON_APP_CRASH_ENABLED

} // namespace crash
