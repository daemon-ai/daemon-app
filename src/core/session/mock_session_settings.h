#pragma once

#include "session/isession_settings.h"

#include <QHash>

namespace session {

class MockSessionSettings : public ISessionSettings {
    Q_OBJECT

public:
    explicit MockSessionSettings(QObject* parent = nullptr);

    [[nodiscard]] int sessionId() const override { return m_sessionId; }
    void setSessionId(int id) override;

    [[nodiscard]] QString profile() const override { return entry().profile; }
    void setProfile(const QString& profile) override;
    [[nodiscard]] QString effort() const override { return entry().effort; }
    void setEffort(const QString& effort) override;
    [[nodiscard]] bool fast() const override { return entry().fast; }
    void setFast(bool on) override;
    [[nodiscard]] bool verbose() const override { return entry().verbose; }
    void setVerbose(bool on) override;

    [[nodiscard]] QStringList effortOptions() const override;

private:
    // The per-session override set. Defaults use the unified off/low/medium/
    // high effort vocabulary (matches the composer's default reasoning effort).
    struct Settings {
        QString profile = QStringLiteral("General Assistant");
        QString effort = QStringLiteral("medium");
        bool fast = false;
        bool verbose = false;
    };

    // The active session's entry (const + mutable accessors), created lazily.
    [[nodiscard]] const Settings& entry() const;
    [[nodiscard]] Settings& entry();

    int m_sessionId = -1;
    mutable QHash<int, Settings> m_bySession;
};

} // namespace session
