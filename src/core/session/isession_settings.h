// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

namespace session {

// Per-session override seam backing the composer's session-settings popover. It
// represents the active session's effective model/effort/profile overrides;
// the mock holds them in memory. A real implementation binds these to the live
// ComposerSessionController for the focused tab.
class ISessionSettings : public QObject {
    Q_OBJECT
    // The session these overrides apply to. Setting it switches the active
    // per-session state (so the composer popover reflects the focused chat,
    // not a single global), emitting changed() so bound views refresh.
    Q_PROPERTY(QString sessionId READ sessionId WRITE setSessionId NOTIFY sessionIdChanged)
    Q_PROPERTY(QString profile READ profile WRITE setProfile NOTIFY changed)
    Q_PROPERTY(QString effort READ effort WRITE setEffort NOTIFY changed)
    Q_PROPERTY(bool fast READ fast WRITE setFast NOTIFY changed)
    Q_PROPERTY(bool verbose READ verbose WRITE setVerbose NOTIFY changed)
    // The per-session approval mode (CHA-4): "ask" | "accept_edits" | "auto_allow" | "deny".
    // Default "ask" (interactive). In daemon mode the setter sends SetSessionMode so the node's
    // SessionOverlay.approval_mode tracks it; the mock holds it in memory.
    Q_PROPERTY(QString approvalMode READ approvalMode WRITE setApprovalMode NOTIFY changed)
    // [wave2:app-approvals-safety] D4: the active session's remembered exec-approval fingerprints
    // (per-session allow-list, wire v29). A VariantListModel of rows
    // { fingerprint, shortFingerprint, label } — null when unbacked (mock / non-daemon); QML
    // guards. The pointer is per-instance-constant (daemon impl builds it once); row changes drive
    // the view.
    Q_PROPERTY(QObject* rememberedFingerprints READ rememberedFingerprints CONSTANT)

public:
    using QObject::QObject;
    ~ISessionSettings() override = default;

    [[nodiscard]] virtual QString sessionId() const = 0;
    virtual void setSessionId(const QString& id) = 0;

    [[nodiscard]] virtual QString profile() const = 0;
    virtual void setProfile(const QString& profile) = 0;
    // The profile override for a specific session id (not the active one). The turn wiring reads
    // this so each orchestrator binds its OWN tab's profile without mutating the shared active
    // sessionId. Default: the active profile iff `id` is the active session, else empty (= node's
    // active profile); the mock overrides it with a per-session lookup.
    [[nodiscard]] virtual QString profileFor(const QString& id) const {
        return id == sessionId() ? profile() : QString();
    }
    [[nodiscard]] virtual QString effort() const = 0;
    virtual void setEffort(const QString& effort) = 0;
    [[nodiscard]] virtual bool fast() const = 0;
    virtual void setFast(bool on) = 0;
    [[nodiscard]] virtual bool verbose() const = 0;
    virtual void setVerbose(bool on) = 0;
    [[nodiscard]] virtual QString approvalMode() const = 0;
    virtual void setApprovalMode(const QString& mode) = 0;
    // The approval mode for a specific session id (not the active one) — the per-pane policy chip
    // (E1/TOOL-7) reads this so every visible session renders its own mode without mutating the
    // shared active sessionId. NOTE: this reflects the client's last-SET value (default "ask");
    // v28 wires no per-session mode getter, so it is not a node-authoritative read. Default: the
    // active mode iff `id` is the active session; the mock overrides with a per-session lookup.
    [[nodiscard]] Q_INVOKABLE virtual QString approvalModeFor(const QString& id) const {
        return id == sessionId() ? approvalMode() : QStringLiteral("ask");
    }

    [[nodiscard]] Q_INVOKABLE virtual QStringList effortOptions() const = 0;
    // The selectable approval modes for the popover (CHA-4).
    [[nodiscard]] Q_INVOKABLE virtual QStringList approvalModeOptions() const = 0;

    // [wave2:app-approvals-safety] D4: remembered-fingerprint management for the active session.
    // Default: unbacked (nullptr / no-op) so the mock and non-daemon builds are unchanged; the
    // DaemonSessionSettings override backs these with a FingerprintRepository.
    [[nodiscard]] virtual QObject* rememberedFingerprints() const { return nullptr; }
    // Re-query the active session's allow-list (FingerprintList). Call when the popover opens.
    Q_INVOKABLE virtual void refreshFingerprints() {}
    // Revoke one remembered fingerprint (FingerprintRevoke); on Ok the list re-syncs from the node.
    Q_INVOKABLE virtual void revokeFingerprint(const QString& /*fingerprint*/) {}

signals:
    void changed();
    void sessionIdChanged();
};

} // namespace session
