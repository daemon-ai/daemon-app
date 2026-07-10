// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#include "platform/autostart/autostart_backend.h"
#include "platform/autostart/autostart_command.h"
#include "platform/autostart/autostart_controller.h"
#include "platform/autostart/autostart_entry.h"
#include "platform/autostart/autostart_windows_logic.h"
#include "platform/single_instance.h"
#include "settings/qt_settings_store.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest/QtTest>

using namespace autostart;
using settings::QtSettingsStore;

// Guards the launch-at-login seam (src/core/platform/autostart): the pure
// command/desktop-entry/StartupApproved logic on every platform, plus - on
// Linux, where the compiled backend is the XDG one - the backend end-to-end
// (apply/query/repair under QStandardPaths test mode) and the controller's
// state matrix / one-time first-run default. The single-instance guard's
// claim + raise protocol is covered at the end (platform/single_instance).
class TestAutostart : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        // Redirect QStandardPaths + QSettings to throwaway locations so the
        // suite never touches the developer's real config (or real autostart
        // entries!).
        QStandardPaths::setTestModeEnabled(true);
        QCoreApplication::setApplicationVersion(QStringLiteral("1.2.3"));
    }

    void init() {
        QFile::remove(entryPath());
        QtSettingsStore store;
        store.setValue(QStringLiteral("app/autostartConfigured"), {});
        store.setValue(QStringLiteral("app/autostartRegisteredVersion"), {});
    }

    // --- pure: Exec quoting + desktop-entry render/parse ---------------------

    void execQuoting() {
        QCOMPARE(quoteExecArg(QStringLiteral("/opt/daemon-app")),
                 QStringLiteral("\"/opt/daemon-app\""));
        QCOMPARE(quoteExecArg(QStringLiteral("/opt/da emon/app")),
                 QStringLiteral("\"/opt/da emon/app\""));
        // The spec's reserved characters are backslash-escaped inside quotes.
        QCOMPARE(quoteExecArg(QStringLiteral("a\"b`c$d\\e")),
                 QStringLiteral("\"a\\\"b\\`c\\$d\\\\e\""));
    }

    void entryRenderParseRoundTrip() {
        const QString program = QStringLiteral("/opt/da emon/daemon-app");
        const QString content = renderAutostartEntry(program, {hiddenArg()});

        const EntryFacts facts = parseAutostartEntry(content);
        QCOMPARE(facts.execProgram, program);
        QCOMPARE(facts.execArgs, QStringList{hiddenArg()});
        QCOMPARE(facts.tryExec, program);
        QVERIFY(!facts.hidden);
        QVERIFY(facts.gnomeEnabled);
        QVERIFY(entryEnablesAutostart(facts));
        QVERIFY(!entryNeedsRewrite(facts, program, hiddenArg()));

        // Spec-shaped essentials the DEs rely on.
        QVERIFY(content.contains(QStringLiteral("[Desktop Entry]")));
        QVERIFY(content.contains(QStringLiteral("Type=Application")));
        QVERIFY(content.contains(QStringLiteral("Terminal=false")));
        QVERIFY(content.contains(QStringLiteral("TryExec=") + program));
    }

    void entryExternalDisableReadsAsDisabled() {
        const QString program = QStringLiteral("/usr/bin/daemon-app");
        // A DE's own autostart UI disables via Hidden=true or
        // X-GNOME-Autostart-enabled=false; both must read back as "off".
        EntryFacts hidden = parseAutostartEntry(renderAutostartEntry(program, {hiddenArg()},
                                                                     /*hidden=*/true));
        QVERIFY(hidden.hidden);
        QVERIFY(!entryEnablesAutostart(hidden));

        EntryFacts gnomeOff =
            parseAutostartEntry(renderAutostartEntry(program, {hiddenArg()}, /*hidden=*/false,
                                                     /*gnomeEnabled=*/false));
        QVERIFY(!gnomeOff.gnomeEnabled);
        QVERIFY(!entryEnablesAutostart(gnomeOff));
    }

    void entryNeedsRewriteMatrix() {
        const QString oldProgram = QStringLiteral("/nix/store/aaa/bin/daemon-app");
        const QString newProgram = QStringLiteral("/nix/store/bbb/bin/daemon-app");
        const EntryFacts facts =
            parseAutostartEntry(renderAutostartEntry(oldProgram, {hiddenArg()}));
        QVERIFY(!entryNeedsRewrite(facts, oldProgram, hiddenArg()));
        // Program moved (AppImage self-update / nix store churn) -> rewrite.
        QVERIFY(entryNeedsRewrite(facts, newProgram, hiddenArg()));
        // An entry missing the --hidden argument (user-edited Exec) -> rewrite.
        const EntryFacts noHidden = parseAutostartEntry(renderAutostartEntry(oldProgram, {}));
        QVERIFY(entryNeedsRewrite(noHidden, oldProgram, hiddenArg()));
    }

    // --- pure: launch-command resolution --------------------------------------

    void programResolutionPrecedence() {
        // $APPIMAGE wins: the FUSE-mount applicationFilePath is gone next boot.
        QCOMPARE(resolveProgramFromParts(QStringLiteral("/home/u/Daemon.AppImage"),
                                         QStringLiteral("/tmp/.mount_x/usr/bin/daemon-app")),
                 QStringLiteral("/home/u/Daemon.AppImage"));
        QCOMPARE(resolveProgramFromParts(QString(), QStringLiteral("/usr/bin/daemon-app")),
                 QStringLiteral("/usr/bin/daemon-app"));
    }

    void siblingGuiResolution() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        // Absent sibling -> empty (the TUI hides the row).
        QCOMPARE(resolveSiblingGuiProgramInDir(dir.path()), QString());

#ifndef Q_OS_WIN
        const QString sibling = QDir(dir.path()).filePath(QStringLiteral("daemon-app"));
        {
            QFile f(sibling);
            QVERIFY(f.open(QIODevice::WriteOnly));
            f.write("#!/bin/sh\n");
        }
        QFile::setPermissions(sibling, QFile::permissions(sibling) | QFileDevice::ExeOwner);
        QCOMPARE(resolveSiblingGuiProgramInDir(dir.path()), sibling);
#endif
    }

    // --- pure: Windows Run value + StartupApproved ---------------------------

    void windowsRunValue() {
        const QString value = composeRunValue(
            QStringLiteral("C:\\Users\\u\\AppData\\Local\\Programs\\Daemon\\daemon-app.exe"),
            hiddenArg());
        QCOMPARE(value, QStringLiteral("\"C:\\Users\\u\\AppData\\Local\\Programs\\Daemon\\"
                                       "daemon-app.exe\" --hidden"));
        QCOMPARE(runValueProgram(value),
                 QStringLiteral("C:\\Users\\u\\AppData\\Local\\Programs\\Daemon\\"
                                "daemon-app.exe"));
        // Bare (unquoted) legacy values parse to the first token.
        QCOMPARE(runValueProgram(QStringLiteral("C:\\daemon.exe --hidden")),
                 QStringLiteral("C:\\daemon.exe"));
        QCOMPARE(runValueProgram(QString()), QString());
    }

    void startupApprovedDecoding() {
        // No blob at all: the user never touched Task Manager -> runs.
        QCOMPARE(decodeStartupApproved({}), ApprovedState::NoData);
        // First byte even -> enabled (observed 0x02 / 0x06 variants).
        QCOMPARE(decodeStartupApproved(QByteArray::fromHex("020000000000000000000000")),
                 ApprovedState::Enabled);
        QCOMPARE(decodeStartupApproved(QByteArray::fromHex("06")), ApprovedState::Enabled);
        // First byte odd -> the user disabled it (observed 0x03 / 0x07).
        QCOMPARE(decodeStartupApproved(QByteArray::fromHex("030000000000000000000000")),
                 ApprovedState::Disabled);
        QCOMPARE(decodeStartupApproved(QByteArray::fromHex("07")), ApprovedState::Disabled);
    }

// The compiled backend is the XDG one exactly where the app would use it; the
// end-to-end below exercises the same code path a real Linux run takes.
#if defined(Q_OS_LINUX)

    // --- backend end-to-end (XDG, under QStandardPaths test mode) ------------

    void xdgBackendEndToEnd() {
        const QString program = fakeProgram();

        // Fresh state: no entry -> Disabled.
        QCOMPARE(backend::query(program).state, State::Disabled);

        // Enable writes an active entry.
        QCOMPARE(backend::apply(program, true).state, State::Enabled);
        QVERIFY(QFile::exists(entryPath()));
        QCOMPARE(backend::query(program).state, State::Enabled);

        // Disable removes the file entirely.
        QCOMPARE(backend::apply(program, false).state, State::Disabled);
        QVERIFY(!QFile::exists(entryPath()));
    }

    void xdgBackendHonorsExternalDisable() {
        const QString program = fakeProgram();
        QCOMPARE(backend::apply(program, true).state, State::Enabled);

        // A DE flipping Hidden=true must read back as Disabled (OS state is
        // the source of truth)...
        writeEntry(renderAutostartEntry(program, {hiddenArg()}, /*hidden=*/true));
        QCOMPARE(backend::query(program).state, State::Disabled);

        // ...and a repair (stale-path rewrite) must NOT resurrect it.
        writeEntry(renderAutostartEntry(QStringLiteral("/stale/daemon-app"), {hiddenArg()},
                                        /*hidden=*/true));
        QVERIFY(backend::repair(program));
        const EntryFacts facts = parseAutostartEntry(readEntry());
        QCOMPARE(facts.execProgram, program);
        QVERIFY(facts.hidden);
        QCOMPARE(backend::query(program).state, State::Disabled);
    }

    void xdgBackendRepairRewritesStaleProgram() {
        const QString program = fakeProgram();
        writeEntry(
            renderAutostartEntry(QStringLiteral("/nix/store/old/daemon-app"), {hiddenArg()}));
        QVERIFY(backend::repair(program));
        const EntryFacts facts = parseAutostartEntry(readEntry());
        QCOMPARE(facts.execProgram, program);
        QCOMPARE(facts.tryExec, program);
        // An up-to-date entry is left alone.
        QVERIFY(!backend::repair(program));
        // repair never creates an entry from nothing.
        QFile::remove(entryPath());
        QVERIFY(!backend::repair(program));
        QVERIFY(!QFile::exists(entryPath()));
    }

    // --- controller state matrix over the real backend -----------------------

    void controllerStateAndToggle() {
        QtSettingsStore store;
        AutostartController controller(fakeProgram(), &store);
        QVERIFY(controller.supported());
        QVERIFY(controller.available());
        QVERIFY(!controller.enabled());
        QCOMPARE(controller.reason(), QString());

        QSignalSpy spy(&controller, &AutostartController::statusChanged);
        controller.setEnabled(true);
        QVERIFY(controller.enabled());
        QCOMPARE(spy.count(), 1);
        // Any explicit toggle sets the one-time marker.
        QVERIFY(store.value(QStringLiteral("app/autostartConfigured")).toBool());

        controller.setEnabled(false);
        QVERIFY(!controller.enabled());

        // refresh() re-reads external changes.
        writeEntry(renderAutostartEntry(fakeProgram(), {hiddenArg()}));
        controller.refresh();
        QVERIFY(controller.enabled());
    }

    void controllerUnsupportedWithoutProgram() {
        QtSettingsStore store;
        // The TUI with no GUI sibling: empty program -> Unsupported, row hidden.
        AutostartController controller(QString(), &store);
        QVERIFY(!controller.supported());
        QVERIFY(!controller.available());
        QVERIFY(!controller.reason().isEmpty());
        // setEnabled is a no-op there.
        controller.setEnabled(true);
        QVERIFY(!controller.enabled());
        QVERIFY(!QFile::exists(entryPath()));
    }

    void firstRunDefaultAppliesExactlyOnce() {
        QtSettingsStore store;
        AutostartController controller(fakeProgram(), &store);

        // First completion: default ON + marker set.
        controller.applyFirstRunDefault();
        QVERIFY(controller.enabled());
        QVERIFY(store.value(QStringLiteral("app/autostartConfigured")).toBool());

        // The user disables (anywhere); a later first-run re-run (FirstRun
        // restart / marker present) must never force it back on.
        controller.setEnabled(false);
        controller.applyFirstRunDefault();
        QVERIFY(!controller.enabled());
        QVERIFY(!QFile::exists(entryPath()));
    }

    void repairOnBootRewritesAndRefreshes() {
        QtSettingsStore store;
        // Entry written by an older install at a stale path, still enabled.
        writeEntry(
            renderAutostartEntry(QStringLiteral("/nix/store/old/daemon-app"), {hiddenArg()}));
        AutostartController controller(fakeProgram(), &store);
        QVERIFY(controller.enabled());
        controller.repairOnBoot();
        const EntryFacts facts = parseAutostartEntry(readEntry());
        QCOMPARE(facts.execProgram, fakeProgram());
        QVERIFY(controller.enabled());
    }

#endif // Q_OS_LINUX

    // --- single-instance guard (platform/single_instance) ---------------------

    void singleInstanceClaimAndRaise() {
        const QString key = platform::SingleInstanceGuard::defaultKey() +
                            QStringLiteral("-tst-%1").arg(QCoreApplication::applicationPid());

        platform::SingleInstanceGuard primary(key);
        QVERIFY(primary.tryClaim());
        QSignalSpy raised(&primary, &platform::SingleInstanceGuard::activateRequested);

        // A second guard on the same key must not claim...
        platform::SingleInstanceGuard secondary(key);
        QVERIFY(!secondary.tryClaim());

        // ...a manual relaunch raises the primary's window...
        QVERIFY(secondary.notifyPrimary(/*raise=*/true));
        QTRY_COMPARE(raised.count(), 1);

        // ...and a --hidden autostart duplicate only pings (no raise).
        QVERIFY(secondary.notifyPrimary(/*raise=*/false));
        QTest::qWait(100);
        QCOMPARE(raised.count(), 1);
    }

    void singleInstanceStaleSocketIsReclaimed() {
        const QString key = platform::SingleInstanceGuard::defaultKey() +
                            QStringLiteral("-stale-%1").arg(QCoreApplication::applicationPid());
        {
            // A primary that never gets to clean up (crash analog): destroying
            // the guard closes the server, but on unix a stale socket file can
            // outlive a SIGKILL; removeServer handles both shapes.
            platform::SingleInstanceGuard crashed(key);
            QVERIFY(crashed.tryClaim());
        }
        platform::SingleInstanceGuard next(key);
        QVERIFY(next.tryClaim());
    }

private:
    static QString entryPath() {
        return QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) +
               QStringLiteral("/autostart/daemon-app.desktop");
    }

    static void writeEntry(const QString& content) {
        QDir().mkpath(QFileInfo(entryPath()).absolutePath());
        QFile f(entryPath());
        QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
        f.write(content.toUtf8());
    }

    static QString readEntry() {
        QFile f(entryPath());
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return {};
        }
        return QString::fromUtf8(f.readAll());
    }

    // A stable fake program path for entries written by the tests. The XDG
    // backend never stats it (TryExec is the DE's own check), so a fixed path
    // keeps the assertions readable.
    static QString fakeProgram() { return QStringLiteral("/opt/daemon/bin/daemon-app"); }
};

QTEST_GUILESS_MAIN(TestAutostart)
#include "tst_autostart.moc"
