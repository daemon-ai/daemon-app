// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#ifndef DAEMON_APP_CORE_CRASH_CRASH_REPORTER_H
#define DAEMON_APP_CORE_CRASH_CRASH_REPORTER_H

#include <QString>

// Consent-gated crash reporting for the daemon-app frontends (GUI + TUI), backed by the Sentry
// native SDK (sentry-native, crashpad/breakpad backend per platform — see cmake/Dependencies.cmake
// and docs/crash-reporting.md).
//
// Compiled-out cases (every function becomes a no-op, and <sentry.h> is never referenced):
//   - Q_OS_WASM builds (no out-of-process handler / native crash surface in the browser), and
//   - builds where the sentry-native target is absent (DAEMON_APP_HAVE_SENTRY undefined).
//
// Consent model (distinct from the telemetry consent): the SDK is initialized with
// `require_user_consent = 1`, so it ARMS capture immediately (a crash right after launch is caught)
// but HOLDS every upload until consent is granted. `giveConsent()` / `revokeConsent()` flip that
// gate at runtime; the consent seam also caches the decision in QSettings so `init()` +
// `giveConsent()` can arm correctly at the next launch before any node connection exists.
namespace crash {

// Breadcrumb severity (maps onto Sentry levels). Mirrors the Qt message types the log handlers
// forward, so the crash's Sentry event carries the qWarning/qCritical trail that led up to it.
enum class Level { Debug, Info, Warning, Error, Fatal };

// RAII holder for the process-lifetime Sentry client. Its destructor calls `sentry_close()` (which
// flushes any pending envelope), so bind it in `main()` and keep it for the whole run — it must
// outlive every early-return path. A default-constructed / disabled Guard is a no-op.
class Guard {
public:
    Guard() = default;
    explicit Guard(bool armed) : m_armed(armed) {}
    ~Guard();

    Guard(Guard&& other) noexcept : m_armed(other.m_armed) { other.m_armed = false; }
    Guard& operator=(Guard&& other) noexcept;
    Guard(const Guard&) = delete;
    Guard& operator=(const Guard&) = delete;

    [[nodiscard]] bool armed() const { return m_armed; }

private:
    bool m_armed = false;
};

// The DSN compiled in via -DDAEMON_APP_SENTRY_DSN (empty ⇒ crash reporting disabled/compiled out).
// PUBLIC value — safe to embed. Callers resolve the effective DSN with the env override below.
[[nodiscard]] QString compiledDefaultDsn();

// The effective DSN: the `DAEMON_APP_SENTRY_DSN` environment override if set, else the compiled
// default. Empty ⇒ crash reporting is disabled.
[[nodiscard]] QString effectiveDsn();

// The default sentry-native database directory (holds pending/captured crash state) under
// QStandardPaths::AppDataLocation. Safe to call once the application name is set; it does not touch
// a running QCoreApplication instance beyond the standard-paths lookup.
[[nodiscard]] QString defaultDatabasePath();

// The co-located crashpad_handler path: `<applicationDirPath>/crashpad_handler[.exe]`, the same
// co-location contract the local daemon launcher uses for the node binary. Only meaningful for the
// crashpad backend; ignored by the breakpad/inproc backends.
[[nodiscard]] QString defaultHandlerPath();

// Initialize the Sentry SDK for the `component` role (e.g. "gui", "tui"). `release` is the Sentry
// release string ("daemon-app@<version>"); `databasePath` / `handlerPath` as above; `dsn` the
// resolved DSN (empty ⇒ returns a disabled Guard, all no-ops). Also installs the SDK with
// `require_user_consent = 1` (armed, upload held) and — call this BEFORE the QML engine / TUI root
// so the earliest frames are covered (constructing QApplication first is acceptable; capture of the
// very first pre-init frames is the documented tradeoff we accept).
[[nodiscard]] Guard init(const QString& component, const QString& release,
                         const QString& databasePath, const QString& handlerPath,
                         const QString& dsn);

// Runtime consent gate (sentry_user_consent_give / _revoke). No-op when disabled.
void giveConsent();
void revokeConsent();

// The last-known crash consent, cached in QSettings (default false). `init()` callers read this to
// arm `giveConsent()` at startup before any node connection exists; the consent seam writes it via
// persistConsent() on every toggle. This is a pure QSettings read (works even when crash reporting
// is compiled out), so the cached decision survives across builds/platforms.
[[nodiscard]] bool consentCached();

// Apply + persist a consent decision: flip the live Sentry consent gate (give/revokeConsent) AND
// cache it in QSettings (consentCached). The single seam both the daemon and mock consent surfaces
// call so the local reporter and the next-startup arming stay in lockstep with the node's toggle.
void persistConsent(bool enabled);

// Add a breadcrumb to the current Sentry scope (the Qt log-handler trail). No-op when disabled.
void addBreadcrumb(Level level, const QString& category, const QString& message);

// Hidden crash-corpus trigger for manual/local verification (never in a normal run): deliberately
// crash the process by `kind` — "segv" (null deref), "abort" (std::abort), or "throw" (uncaught
// exception → terminate). Wired to the DAEMON_APP_CRASH_TEST env in the GUI/TUI mains. With consent
// OFF the resulting minidump is captured into the local database but NOT uploaded. An unknown/empty
// kind is a no-op. This function does not return for a recognized kind.
void triggerTestCrash(const QString& kind);

} // namespace crash

#endif // DAEMON_APP_CORE_CRASH_CRASH_REPORTER_H
