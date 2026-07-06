// SPDX-License-Identifier: MPL-2.0
// SPDX-FileCopyrightText: 2026 Jarrad Hope

#pragma once

#include "session/isession_settings.h"

#include <QHash>

namespace session {

class MockSessionSettings : public ISessionSettings {
    Q_OBJECT

public:
    explicit MockSessionSettings(QObject* parent = nullptr);

    [[nodiscard]] QString sessionId() const override { return m_sessionId; }
    void setSessionId(const QString& id) override;

    [[nodiscard]] QString profile() const override { return entry().profile; }
    void setProfile(const QString& profile) override;
    // Per-session lookup (no side effects): read the override for `id` directly from the map
    // without switching the active session, so a background tab's turn wiring never clobbers the
    // focused session's overrides. Empty when `id` has no stored override (= node's active
    // profile).
    [[nodiscard]] QString profileFor(const QString& id) const override {
        return m_bySession.value(id).profile;
    }
    [[nodiscard]] QString effort() const override { return entry().effort; }
    void setEffort(const QString& effort) override;
    [[nodiscard]] bool fast() const override { return entry().fast; }
    void setFast(bool on) override;
    [[nodiscard]] bool verbose() const override { return entry().verbose; }
    void setVerbose(bool on) override;
    [[nodiscard]] QString approvalMode() const override { return entry().approvalMode; }
    void setApprovalMode(const QString& mode) override;
    // Per-session lookup (no side effects), like profileFor: default "ask" for ids with no stored
    // override.
    [[nodiscard]] QString approvalModeFor(const QString& id) const override {
        return m_bySession.contains(id) ? m_bySession.value(id).approvalMode
                                        : QStringLiteral("ask");
    }

    [[nodiscard]] QStringList effortOptions() const override;
    [[nodiscard]] QStringList approvalModeOptions() const override;

protected:
    // The per-session override set. Defaults use the unified off/low/medium/
    // high effort vocabulary (matches the composer's default reasoning effort).
    struct Settings {
        // Empty = no per-session override, i.e. the node's active profile drives the turn. A
        // profile is bound only once the user explicitly picks one in the session-settings
        // popover/overlay.
        QString profile;
        QString effort = QStringLiteral("medium");
        bool fast = false;
        bool verbose = false;
        QString approvalMode = QStringLiteral("ask");
    };

    // The active session's entry (const + mutable accessors), created lazily.
    [[nodiscard]] const Settings& entry() const;
    [[nodiscard]] Settings& entry();

    QString m_sessionId;
    mutable QHash<QString, Settings> m_bySession;
};

} // namespace session
