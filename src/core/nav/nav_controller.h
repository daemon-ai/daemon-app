#pragma once

#include <QObject>
#include <QString>

namespace nav {

// The single app-level navigation seam shared by both front ends. App-level
// settings/manager surfaces (Settings, Models, Accounts, Profiles, Fleet,
// Sessions, Dashboard, Approvals, Routing, Cron) are full-window *pages* (not
// transcript popups); this controller holds which one is open and an optional
// deep-link section. The GUI mounts the active page as an overlay over the shell;
// the TUI raises a matching full-screen window. Reachable from the cog menu, the
// command palette, and slash commands in both front ends.
class NavController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString page READ page NOTIFY changed)
    Q_PROPERTY(QString section READ section NOTIFY changed)
    Q_PROPERTY(bool active READ active NOTIFY changed)

public:
    using QObject::QObject;

    [[nodiscard]] QString page() const { return m_page; }
    [[nodiscard]] QString section() const { return m_section; }
    [[nodiscard]] bool active() const { return !m_page.isEmpty(); }

    // Open `page` (e.g. "settings", "models", "accounts", ...), optionally
    // deep-linked to `section` (a settings tab id, a models subroute, ...).
    Q_INVOKABLE void open(const QString& page, const QString& section = QString());
    // Switch the section of the already-open page (no-op if a different page).
    Q_INVOKABLE void setSection(const QString& section);
    Q_INVOKABLE void close();

    // Open a per-agent surface keyed by `profileRef` (agent identity). `kind` is
    // "memory" or "profile"; `title` is the tab label. Multi-instance (one tab per
    // agent), unlike the singleton page open() above.
    Q_INVOKABLE void openAgent(const QString& kind, const QString& profileRef,
                               const QString& title);

signals:
    void changed();
    // Fired on every open() so the TUI can raise/replace its page window even
    // when the page id is unchanged (e.g. re-deep-linking a section).
    void openRequested(const QString& page, const QString& section);
    // Fired on openAgent(): open/raise a per-agent (ProfileRef-keyed) Memory or
    // Profile tab. Both front ends route this to TabModel::openAgentTab.
    void openAgentRequested(const QString& kind, const QString& profileRef,
                            const QString& title);
    void closeRequested();

private:
    QString m_page;
    QString m_section;
};

} // namespace nav
