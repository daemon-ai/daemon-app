// SPDX-FileCopyrightText: 2026 Jarrad Hope
// SPDX-License-Identifier: MPL-2.0
//
// daemon-updater apply engine. Plain C++17, no Qt. POSIX and Win32 paths sit
// behind #ifdef. Design provenance (Sparkle 2.x study): verify before mutate;
// stage on the target filesystem; atomic swap primitive with a two-move +
// rollback fallback; apply only after the target process exited; keep the
// displaced old version until the new one is confirmed; never trust
// world-writable paths; the app decides downgrades, the helper only re-verifies
// bytes.

#include "updater.h"

#include "digest.h"

#include <array>
#include <cctype>
#include <charconv>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#else
#include <cerrno>
#include <csignal>
#include <ctime>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#ifdef __linux__
#include <sys/syscall.h>
#endif
#endif

namespace fs = std::filesystem;

namespace {

// --- Contract exit codes ---------------------------------------------------
constexpr int kExitApplied = 0;           // applied (+relaunched if requested)
constexpr int kExitVerifyFailed = 2;      // staged re-verification failed; nothing mutated
constexpr int kExitApplyRolledBack = 3;   // apply failed, previous state restored
constexpr int kExitApplyInconsistent = 4; // apply failed AND rollback failed (loud)
constexpr int kExitBadArgs = 5;           // bad arguments or wait-pid timeout; nothing mutated

constexpr int kPollIntervalMs = 100;
constexpr int kDefaultTimeoutSecs = 60;
constexpr int kMaxAsideAttempts = 4096;

enum class Mode : std::uint8_t { RenameSwap, TwoMove, NsisSilent };

struct Spec {
    long waitPid = 0;
    Mode mode = Mode::RenameSwap;
    bool modeSet = false;
    std::string staged;
    std::string target;
    std::string sha256; // lowercase hex; empty => none
    std::string oldAside;
    std::string logPath;
    int timeoutSecs = kDefaultTimeoutSecs;
    std::vector<std::string> relaunch;
    bool relocated = false; // internal: this is the relocated Windows copy
};

// --- Logging ---------------------------------------------------------------
// Deterministic, greppable, one line per step. Best-effort: a logging failure
// never aborts the apply. Every line is also mirrored to stderr for forensics.
void logMsg(const Spec& spec, std::string_view line) {
    std::array<char, 32> stamp{};
    const std::time_t now = std::time(nullptr);
    std::tm tmv{};
#ifdef _WIN32
    gmtime_s(&tmv, &now);
#else
    gmtime_r(&now, &tmv);
#endif
    if (std::strftime(stamp.data(), stamp.size(), "%Y-%m-%dT%H:%M:%SZ", &tmv) == 0) {
        stamp[0] = '\0';
    }
    std::string composed =
        std::string(stamp.data()) + " daemon-updater: " + std::string(line) + "\n";
    std::fputs(composed.c_str(), stderr);
    if (!spec.logPath.empty()) {
        std::ofstream out(spec.logPath, std::ios::app | std::ios::binary);
        if (out) {
            out << composed;
        }
    }
}

std::string toLower(std::string s) {
    for (char& c : s) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return s;
}

// --- Filesystem helpers ----------------------------------------------------
std::string parentDir(const std::string& path) {
    fs::path parent = fs::path(path).parent_path();
    if (parent.empty()) {
        return ".";
    }
    return parent.string();
}

// Same-filesystem check: staging next to the target is the whole point (an
// atomic rename cannot cross a mount). POSIX compares st_dev; Windows compares
// the volume mount point.
bool sameFilesystem(const std::string& a, const std::string& b) {
#ifdef _WIN32
    std::wstring wa(a.begin(), a.end());
    std::wstring wb(b.begin(), b.end());
    std::array<wchar_t, MAX_PATH> volA{};
    std::array<wchar_t, MAX_PATH> volB{};
    if (GetVolumePathNameW(wa.c_str(), volA.data(), static_cast<DWORD>(volA.size())) == 0) {
        return false;
    }
    if (GetVolumePathNameW(wb.c_str(), volB.data(), static_cast<DWORD>(volB.size())) == 0) {
        return false;
    }
    return lstrcmpiW(volA.data(), volB.data()) == 0;
#else
    struct stat sa{};
    struct stat sb{};
    if (::stat(a.c_str(), &sa) != 0 || ::stat(b.c_str(), &sb) != 0) {
        return false;
    }
    return sa.st_dev == sb.st_dev;
#endif
}

// Unique, not-yet-existing path in the target's own directory (same filesystem
// by construction) to park the displaced old version when --old-aside is empty.
std::string makeTempAsidePath(const Spec& spec) {
    const fs::path parent = parentDir(spec.target);
#ifdef _WIN32
    const auto pid = static_cast<long>(GetCurrentProcessId());
#else
    const auto pid = static_cast<long>(::getpid());
#endif
    for (int i = 0; i < kMaxAsideAttempts; ++i) {
        fs::path cand =
            parent / (".daemon-updater-old-" + std::to_string(pid) + "-" + std::to_string(i));
        std::error_code ec;
        if (!fs::exists(cand, ec)) {
            return cand.string();
        }
    }
    return {};
}

void makeExecutable(const Spec& spec, const std::string& path) {
#ifdef _WIN32
    (void)spec;
    (void)path;
#else
    struct stat st{};
    if (::stat(path.c_str(), &st) == 0) {
        const mode_t next = st.st_mode | S_IXUSR | S_IXGRP | S_IXOTH;
        if (::chmod(path.c_str(), next) == 0) {
            logMsg(spec, "chmod +x ok");
            return;
        }
    }
    logMsg(spec, "chmod +x failed (best-effort) errno=" + std::to_string(errno));
#endif
}

// --- wait-pid --------------------------------------------------------------
// Poll for the target process to exit. PID reuse between spawn and exit is an
// accepted risk at this scale (documented): a reused PID at worst makes the
// helper wait a little longer for an unrelated process, never mutates early.
bool waitForPid(const Spec& spec) {
    if (spec.waitPid <= 0) {
        logMsg(spec, "wait-pid skipped (pid=0)");
        return true;
    }
    logMsg(spec, "wait-pid waiting for pid=" + std::to_string(spec.waitPid) +
                     " timeout=" + std::to_string(spec.timeoutSecs) + "s");
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(spec.timeoutSecs);
#ifdef _WIN32
    HANDLE proc = OpenProcess(SYNCHRONIZE, FALSE, static_cast<DWORD>(spec.waitPid));
    if (proc == nullptr) {
        logMsg(spec, "wait-pid target already gone");
        return true;
    }
    const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
                               deadline - std::chrono::steady_clock::now())
                               .count();
    const DWORD ms = remaining <= 0 ? 0 : static_cast<DWORD>(remaining);
    const DWORD rc = WaitForSingleObject(proc, ms);
    CloseHandle(proc);
    if (rc == WAIT_OBJECT_0) {
        logMsg(spec, "wait-pid target exited");
        return true;
    }
    logMsg(spec, "wait-pid TIMEOUT");
    return false;
#else
    for (;;) {
        if (::kill(static_cast<pid_t>(spec.waitPid), 0) != 0 && errno == ESRCH) {
            logMsg(spec, "wait-pid target exited");
            return true;
        }
        if (std::chrono::steady_clock::now() >= deadline) {
            logMsg(spec, "wait-pid TIMEOUT");
            return false;
        }
        const timespec req{0, static_cast<long>(kPollIntervalMs) * 1000L * 1000L};
        ::nanosleep(&req, nullptr);
    }
#endif
}

// --- relaunch --------------------------------------------------------------
void relaunch(const Spec& spec) {
    if (spec.relaunch.empty()) {
        return;
    }
    logMsg(spec, "relaunch spawning detached: " + spec.relaunch.front());
#ifdef _WIN32
    std::wstring cmd;
    for (const std::string& part : spec.relaunch) {
        std::wstring wpart(part.begin(), part.end());
        if (!cmd.empty()) {
            cmd += L' ';
        }
        cmd += L'"';
        cmd += wpart;
        cmd += L'"';
    }
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    std::vector<wchar_t> mutableCmd(cmd.begin(), cmd.end());
    mutableCmd.push_back(L'\0');
    if (CreateProcessW(nullptr, mutableCmd.data(), nullptr, nullptr, FALSE,
                       DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP, nullptr, nullptr, &si,
                       &pi) != 0) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        logMsg(spec, "relaunch ok");
    } else {
        logMsg(spec, "relaunch FAILED (best-effort)");
    }
#else
    const pid_t child = ::fork();
    if (child < 0) {
        logMsg(spec, "relaunch fork FAILED (best-effort)");
        return;
    }
    if (child == 0) {
        ::setsid();
        const int devnull = ::open("/dev/null", O_RDWR);
        if (devnull >= 0) {
            ::dup2(devnull, STDIN_FILENO);
            ::dup2(devnull, STDOUT_FILENO);
            ::dup2(devnull, STDERR_FILENO);
            if (devnull > STDERR_FILENO) {
                ::close(devnull);
            }
        }
        std::vector<char*> argv;
        argv.reserve(spec.relaunch.size() + 1);
        for (const std::string& part : spec.relaunch) {
            argv.push_back(const_cast<char*>(part.c_str()));
        }
        argv.push_back(nullptr);
        ::execv(argv[0], argv.data());
        ::_exit(127); // exec failed in the child
    }
    logMsg(spec, "relaunch ok (pid=" + std::to_string(static_cast<long>(child)) + ")");
#endif
}

// --- rename-swap (AppImage, POSIX) -----------------------------------------
int applyRenameSwap(const Spec& spec) {
#ifdef _WIN32
    logMsg(spec, "error rename-swap is a POSIX-only mode");
    return kExitBadArgs;
#else
    if (!sameFilesystem(spec.staged, spec.target)) {
        logMsg(spec, "same-fs FAIL staged and target are on different filesystems");
        return kExitVerifyFailed;
    }
    logMsg(spec, "same-fs ok");

    bool exchanged = false;
#if defined(__linux__) && defined(SYS_renameat2)
#ifndef RENAME_EXCHANGE
#define RENAME_EXCHANGE (1U << 1U)
#endif
    if (::syscall(SYS_renameat2, AT_FDCWD, spec.staged.c_str(), AT_FDCWD, spec.target.c_str(),
                  static_cast<unsigned int>(RENAME_EXCHANGE)) == 0) {
        exchanged = true;
        logMsg(spec, "exchange ok (RENAME_EXCHANGE)");
    } else {
        logMsg(spec, "exchange unavailable errno=" + std::to_string(errno) +
                         "; falling back to plain rename");
    }
#endif

    if (!exchanged) {
        // Plain rename(2) over the target. POSIX keeps the running mount's inode
        // alive, so this is safe even before the old process exits (we still
        // honored --wait-pid). Atomic: on failure nothing was mutated.
        if (::rename(spec.staged.c_str(), spec.target.c_str()) != 0) {
            logMsg(spec, "rename FAIL errno=" + std::to_string(errno));
            return kExitApplyRolledBack;
        }
        logMsg(spec, "rename ok (plain rename over target)");
    }

    makeExecutable(spec, spec.target);

    // After an exchange the displaced old file now sits at the staged path.
    if (exchanged) {
        std::error_code ec;
        if (!spec.oldAside.empty()) {
            if (::rename(spec.staged.c_str(), spec.oldAside.c_str()) == 0) {
                logMsg(spec, "old-aside parked displaced old at " + spec.oldAside);
            } else {
                fs::remove(spec.staged, ec);
                logMsg(spec, "old-aside move failed; deleted displaced old (best-effort)");
            }
        } else {
            fs::remove(spec.staged, ec);
            logMsg(spec, "displaced old version deleted");
        }
    }
    return kExitApplied;
#endif
}

// --- two-move (macOS .app directory, generic dir/file fallback) ------------
int applyTwoMove(const Spec& spec) {
    if (!sameFilesystem(spec.staged, spec.target)) {
        logMsg(spec, "same-fs FAIL staged and target are on different filesystems");
        return kExitVerifyFailed;
    }

    std::string aside;
    bool asideIsTemp = false;
    if (!spec.oldAside.empty()) {
        if (!sameFilesystem(parentDir(spec.oldAside), spec.target)) {
            logMsg(spec, "same-fs FAIL --old-aside is on a different filesystem than the target");
            return kExitVerifyFailed;
        }
        aside = spec.oldAside;
    } else {
        aside = makeTempAsidePath(spec);
        asideIsTemp = true;
        if (aside.empty()) {
            logMsg(spec, "error could not allocate a temp aside path next to the target");
            return kExitApplyRolledBack;
        }
    }
    logMsg(spec, "same-fs ok; aside=" + aside);

    std::error_code ec;

    // Step 1: move the target out of the way. On failure nothing was mutated.
    fs::rename(spec.target, aside, ec);
    if (ec) {
        logMsg(spec, "move-target FAIL: " + ec.message());
        return kExitApplyRolledBack;
    }
    logMsg(spec, "move-target ok (target -> aside)");

    // Step 2: move the staged artifact into place.
    fs::rename(spec.staged, spec.target, ec);
    if (ec) {
        logMsg(spec, "move-staged FAIL: " + ec.message() + " -> rolling back");
        std::error_code rec;
        fs::rename(aside, spec.target, rec);
        if (rec) {
            logMsg(spec, "ROLLBACK FAILED: " + rec.message() +
                             " -- state INCONSISTENT; old version is at " + aside);
            return kExitApplyInconsistent;
        }
        logMsg(spec, "rollback ok (aside -> target restored)");
        return kExitApplyRolledBack;
    }
    logMsg(spec, "move-staged ok (staged -> target)");

    if (fs::is_regular_file(spec.target, ec)) {
        makeExecutable(spec, spec.target);
    }

    if (asideIsTemp) {
        fs::remove_all(aside, ec);
        logMsg(spec, "displaced old version deleted");
    } else {
        logMsg(spec, "displaced old version parked at " + aside + " (app cleans up on next start)");
    }
    return kExitApplied;
}

// --- nsis-silent (Windows) -------------------------------------------------
#ifdef _WIN32
std::string ownExePath() {
    std::array<wchar_t, MAX_PATH> buf{};
    const DWORD n = GetModuleFileNameW(nullptr, buf.data(), static_cast<DWORD>(buf.size()));
    if (n == 0) {
        return {};
    }
    std::wstring w(buf.data(), n);
    return std::string(w.begin(), w.end());
}

// An exe cannot overwrite itself while running, and a per-user/per-machine NSIS
// installer rewrites its own install tree. If we are still running from inside
// that tree we relocate to %TEMP% and re-spawn an identical invocation carrying
// --relocated so the copy does the real work. We relocate unconditionally
// (unless already relocated): it is the safe superset of "under the install
// tree" and costs one extra short-lived process.
int relocateAndRespawn(const Spec& spec, int argc, char** argv) {
    const std::string self = ownExePath();
    std::array<wchar_t, MAX_PATH> tmpDir{};
    if (GetTempPathW(static_cast<DWORD>(tmpDir.size()), tmpDir.data()) == 0) {
        logMsg(spec, "relocate FAIL: no %TEMP%");
        return kExitApplyRolledBack;
    }
    std::array<wchar_t, MAX_PATH> tmpFile{};
    if (GetTempFileNameW(tmpDir.data(), L"dup", 0, tmpFile.data()) == 0) {
        logMsg(spec, "relocate FAIL: GetTempFileNameW");
        return kExitApplyRolledBack;
    }
    std::wstring dst(tmpFile.data());
    dst += L".exe";
    std::wstring wself(self.begin(), self.end());
    if (CopyFileW(wself.c_str(), dst.c_str(), FALSE) == 0) {
        logMsg(spec, "relocate FAIL: CopyFileW");
        return kExitApplyRolledBack;
    }

    // Rebuild the original command line, appending --relocated.
    std::wstring cmd = L'"' + dst + L'"';
    for (int i = 1; i < argc; ++i) {
        std::string a(argv[i]);
        std::wstring wa(a.begin(), a.end());
        cmd += L" \"" + wa + L'"';
    }
    cmd += L" --relocated";

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    std::vector<wchar_t> mutableCmd(cmd.begin(), cmd.end());
    mutableCmd.push_back(L'\0');
    if (CreateProcessW(nullptr, mutableCmd.data(), nullptr, nullptr, FALSE,
                       DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP, nullptr, nullptr, &si,
                       &pi) == 0) {
        logMsg(spec, "relocate FAIL: CreateProcessW");
        return kExitApplyRolledBack;
    }
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    logMsg(spec, "relocated to temp and re-spawned; exiting parent");
    return kExitApplied;
}

void selfDeleteBestEffort(const Spec& spec) {
    const std::string self = ownExePath();
    if (self.empty()) {
        return;
    }
    std::wstring wself(self.begin(), self.end());
    // Schedule deletion on reboot (a running exe cannot delete itself now).
    MoveFileExW(wself.c_str(), nullptr, MOVEFILE_DELAY_UNTIL_REBOOT);
    logMsg(spec, "self-delete scheduled (best-effort)");
}

int runInstallerSilently(const Spec& spec) {
    std::wstring winstaller(spec.staged.begin(), spec.staged.end());
    std::wstring cmd = L'"' + winstaller + L"\" /S";
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    std::vector<wchar_t> mutableCmd(cmd.begin(), cmd.end());
    mutableCmd.push_back(L'\0');
    if (CreateProcessW(nullptr, mutableCmd.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr,
                       &si, &pi) == 0) {
        logMsg(spec, "nsis CreateProcess FAIL");
        return kExitApplyRolledBack;
    }
    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD code = 1;
    GetExitCodeProcess(pi.hProcess, &code);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    if (code != 0) {
        logMsg(spec, "nsis installer exited non-zero (" + std::to_string(code) + ")");
        return kExitApplyRolledBack;
    }
    logMsg(spec, "nsis installer /S exit 0");
    return kExitApplied;
}
#endif // _WIN32

int applyNsisSilent(const Spec& spec) {
#ifdef _WIN32
    return runInstallerSilently(spec);
#else
    logMsg(spec, "error nsis-silent is a Windows-only mode");
    return kExitBadArgs;
#endif
}

// --- argument parsing ------------------------------------------------------
bool parseArgs(int argc, char** argv, Spec& spec, std::string& err) {
    auto value = [&](int& i) -> const char* {
        if (i + 1 >= argc) {
            return nullptr;
        }
        return argv[++i];
    };

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--wait-pid") {
            const char* v = value(i);
            if (v == nullptr) {
                err = "missing value for --wait-pid";
                return false;
            }
            const char* last = v + std::strlen(v);
            if (std::from_chars(v, last, spec.waitPid).ptr != last) {
                err = "invalid --wait-pid value";
                return false;
            }
        } else if (arg == "--mode") {
            const char* v = value(i);
            if (v == nullptr) {
                err = "missing value for --mode";
                return false;
            }
            const std::string m = v;
            if (m == "rename-swap") {
                spec.mode = Mode::RenameSwap;
            } else if (m == "two-move") {
                spec.mode = Mode::TwoMove;
            } else if (m == "nsis-silent") {
                spec.mode = Mode::NsisSilent;
            } else {
                err = "unknown --mode: " + m;
                return false;
            }
            spec.modeSet = true;
        } else if (arg == "--staged") {
            const char* v = value(i);
            if (v == nullptr) {
                err = "missing value for --staged";
                return false;
            }
            spec.staged = v;
        } else if (arg == "--target") {
            const char* v = value(i);
            if (v == nullptr) {
                err = "missing value for --target";
                return false;
            }
            spec.target = v;
        } else if (arg == "--sha256") {
            const char* v = value(i);
            if (v == nullptr) {
                err = "missing value for --sha256";
                return false;
            }
            spec.sha256 = toLower(v);
        } else if (arg == "--old-aside") {
            const char* v = value(i);
            if (v == nullptr) {
                err = "missing value for --old-aside";
                return false;
            }
            spec.oldAside = v;
        } else if (arg == "--log") {
            const char* v = value(i);
            if (v == nullptr) {
                err = "missing value for --log";
                return false;
            }
            spec.logPath = v;
        } else if (arg == "--timeout") {
            const char* v = value(i);
            if (v == nullptr) {
                err = "missing value for --timeout";
                return false;
            }
            const char* last = v + std::strlen(v);
            if (std::from_chars(v, last, spec.timeoutSecs).ptr != last) {
                err = "invalid --timeout value";
                return false;
            }
        } else if (arg == "--relocated") {
            spec.relocated = true;
        } else if (arg == "--relaunch") {
            if (i + 1 >= argc || std::string(argv[i + 1]) != "--") {
                err = "--relaunch must be followed by -- <argv...>";
                return false;
            }
            i += 2;
            for (; i < argc; ++i) {
                spec.relaunch.emplace_back(argv[i]);
            }
            break;
        } else {
            err = "unknown argument: " + arg;
            return false;
        }
    }
    return true;
}

bool staticValidate(const Spec& spec, std::string& err) {
    if (!spec.modeSet) {
        err = "--mode is required";
        return false;
    }
    if (spec.staged.empty()) {
        err = "--staged is required";
        return false;
    }
    if (spec.timeoutSecs < 0) {
        err = "--timeout must be >= 0";
        return false;
    }
    if (spec.mode != Mode::NsisSilent && spec.target.empty()) {
        err = "--target is required for rename-swap and two-move";
        return false;
    }
    if (spec.mode == Mode::NsisSilent && !spec.oldAside.empty()) {
        err = "--old-aside is not valid for nsis-silent";
        return false;
    }
    return true;
}

// Validate the staged artifact's shape against the --sha256 rule before waiting.
// Regular file => digest required. Directory (.app bundle) => digest must be
// absent (staging dir 0700 owns its integrity). nsis-silent stages a file.
bool fsShapeValidate(const Spec& spec, std::string& err, int& badExit) {
    std::error_code ec;
    if (!fs::exists(spec.staged, ec)) {
        err = "staged path does not exist: " + spec.staged;
        badExit = kExitVerifyFailed;
        return false;
    }
    const bool isDir = fs::is_directory(spec.staged, ec);
    if (isDir) {
        if (!spec.sha256.empty()) {
            err = "--sha256 must be empty when --staged is a directory";
            badExit = kExitBadArgs;
            return false;
        }
        if (spec.mode == Mode::NsisSilent) {
            err = "nsis-silent requires a staged installer file, not a directory";
            badExit = kExitBadArgs;
            return false;
        }
    } else {
        if (spec.sha256.empty()) {
            err = "--sha256 is required when --staged is a regular file";
            badExit = kExitBadArgs;
            return false;
        }
    }
    return true;
}

// Re-verify the staged digest immediately before mutating (closes TOCTOU).
bool verifyStaged(const Spec& spec) {
    if (spec.sha256.empty()) {
        return true; // directory bundle: nothing to hash
    }
    std::string actual;
    if (!daemon_updater::sha256File(spec.staged, actual)) {
        logMsg(spec, "verify FAIL could not read staged file");
        return false;
    }
    if (actual != spec.sha256) {
        logMsg(spec, "verify MISMATCH expected=" + spec.sha256 + " actual=" + actual);
        return false;
    }
    logMsg(spec, "verify ok sha256=" + actual);
    return true;
}

int dispatchApply(const Spec& spec) {
    switch (spec.mode) {
    case Mode::RenameSwap:
        return applyRenameSwap(spec);
    case Mode::TwoMove:
        return applyTwoMove(spec);
    case Mode::NsisSilent:
        return applyNsisSilent(spec);
    }
    return kExitBadArgs;
}

int runImpl(int argc, char** argv) {
    Spec spec;
    std::string err;
    if (!parseArgs(argc, argv, spec, err)) {
        logMsg(spec, "args ERROR " + err);
        return kExitBadArgs;
    }
    if (!staticValidate(spec, err)) {
        logMsg(spec, "args ERROR " + err);
        return kExitBadArgs;
    }

#ifdef _WIN32
    if (spec.mode == Mode::NsisSilent && !spec.relocated) {
        return relocateAndRespawn(spec, argc, argv);
    }
#endif

    int badExit = kExitBadArgs;
    if (!fsShapeValidate(spec, err, badExit)) {
        logMsg(spec, "args ERROR " + err);
        return badExit;
    }

    if (!waitForPid(spec)) {
        return kExitBadArgs; // wait-pid timeout; nothing mutated
    }

    if (!verifyStaged(spec)) {
        return kExitVerifyFailed;
    }

    const int applyRc = dispatchApply(spec);
    if (applyRc == kExitApplied) {
        relaunch(spec);
#ifdef _WIN32
        if (spec.relocated) {
            selfDeleteBestEffort(spec);
        }
#endif
        logMsg(spec, "done applied");
    } else {
        logMsg(spec, "done apply-failed rc=" + std::to_string(applyRc));
    }
    return applyRc;
}

} // namespace

namespace daemon_updater {

int run(int argc, char** argv) noexcept {
    try {
        return runImpl(argc, argv);
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "daemon-updater: fatal exception: %s\n", ex.what());
        return kExitApplyInconsistent;
    } catch (...) {
        std::fprintf(stderr, "daemon-updater: fatal unknown exception\n");
        return kExitApplyInconsistent;
    }
}

} // namespace daemon_updater
