// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

// macOS 13+ (the bundle's LSMinimumSystemVersion floor): SMAppService over the
// agent plist shipped inside the bundle at
// Contents/Library/LaunchAgents/io.daemon.app.autostart.plist (see
// packaging/macos/, staged by src/DaemonApp/App/CMakeLists.txt). The plist is
// inert until registered here at runtime; the entry appears under the app's
// own name in System Settings > General > Login Items. The environment guards
// reuse the tested predicates from update/self_apply_macos.h: a translocated /
// DMG-mounted / non-bundle process must never register, because the login item
// would bind to an ephemeral path.

#include "platform/autostart/autostart_backend.h"

#include "update/self_apply_macos.h"

#import <Foundation/Foundation.h>
#import <ServiceManagement/ServiceManagement.h>

namespace autostart::backend {

namespace {

NSString* plistName() {
    return @"io.daemon.app.autostart.plist";
}

// Environment gate shared by every operation; returns true (with *status
// filled) when registration must not proceed from this process.
bool blockedHere(const QString& program, Status* status) {
    if (program.isEmpty()) {
        *status = {State::Unsupported, Detail::NoProgram, {}};
        return true;
    }
    const QString bundle = update::macos::runningBundlePath();
    if (bundle.isEmpty()) {
        *status = {State::Blocked, Detail::NeedsAppBundle, {}};
        return true;
    }
    if (update::macos::pathIsTranslocated(bundle)) {
        *status = {State::Blocked, Detail::Translocated, {}};
        return true;
    }
    if (update::macos::pathUnderVolumes(bundle)) {
        *status = {State::Blocked, Detail::RunningFromDmg, {}};
        return true;
    }
    return false;
}

Status mapServiceStatus(SMAppServiceStatus status) {
    switch (status) {
    case SMAppServiceStatusEnabled:
        return {State::Enabled, Detail::None, {}};
    case SMAppServiceStatusRequiresApproval:
        return {State::RequiresApproval, Detail::None, {}};
    case SMAppServiceStatusNotRegistered:
        return {State::Disabled, Detail::None, {}};
    case SMAppServiceStatusNotFound:
    default:
        // The agent plist is missing from the bundle (a repackaged/dev build).
        return {State::Blocked, Detail::RegistrationFailed,
                QStringLiteral("login item definition not found in the app bundle")};
    }
}

} // namespace

Status query(const QString& program) {
    Status gate;
    if (blockedHere(program, &gate)) {
        return gate;
    }
    if (@available(macOS 13.0, *)) {
        @autoreleasepool {
            SMAppService* service = [SMAppService agentServiceWithPlistName:plistName()];
            return mapServiceStatus(service.status);
        }
    }
    return {State::Unsupported, Detail::None, {}};
}

Status apply(const QString& program, bool enable) {
    Status gate;
    if (blockedHere(program, &gate)) {
        return gate;
    }
    if (@available(macOS 13.0, *)) {
        @autoreleasepool {
            SMAppService* service = [SMAppService agentServiceWithPlistName:plistName()];
            NSError* error = nil;
            BOOL ok = enable ? [service registerAndReturnError:&error]
                             : [service unregisterAndReturnError:&error];
            // Registration pending user approval reports an error but lands in
            // RequiresApproval; re-reading the live status keeps that case a
            // state, not a failure.
            const Status after = mapServiceStatus(service.status);
            if (!ok && after.state == (enable ? State::Disabled : State::Enabled)) {
                return {State::Blocked, Detail::RegistrationFailed,
                        error != nil ? QString::fromNSString(error.localizedDescription)
                                     : QString()};
            }
            return after;
        }
    }
    return {State::Unsupported, Detail::None, {}};
}

bool repair(const QString& /*program*/) {
    // Nothing path-based to repair: BundleProgram is bundle-relative, so the
    // entry survives the user moving the app. Post-update refresh is
    // reregister()'s job.
    return false;
}

void reregister(const QString& program) {
    Status gate;
    if (blockedHere(program, &gate)) {
        return;
    }
    if (@available(macOS 13.0, *)) {
        @autoreleasepool {
            SMAppService* service = [SMAppService agentServiceWithPlistName:plistName()];
            NSError* error = nil;
            [service unregisterAndReturnError:&error];
            error = nil;
            [service registerAndReturnError:&error];
        }
    }
}

void openApprovalSettings() {
    if (@available(macOS 13.0, *)) {
        @autoreleasepool {
            [SMAppService openSystemSettingsLoginItems];
        }
    }
}

} // namespace autostart::backend
