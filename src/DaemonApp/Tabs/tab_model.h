#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QList>
#include <QString>
#include <QtQml/qqmlregistration.h>

// A pane-level tab strip model shared by the QML GUI and the Tui Widgets TUI.
// It is the single source of truth for the ordered list of open tabs and which
// one is active; the heavy per-tab objects (session controllers, turn
// orchestrators, documents) are owned by each frontend and keyed by the stable
// `tabId` this model hands out.
//
// Tabs are either Transcript tabs (one per open session, identified by
// `sessionId`) or singleton page tabs (Settings now, extensible). The model
// enforces find-or-create semantics: opening a session/page that is already
// open re-activates its existing tab instead of duplicating it. This maps cleanly
// onto the daemon's SessionId session tree (a transcript tab == an open session).
class TabModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    // Tab kinds. Transcript tabs carry a sessionId; page kinds are
    // singletons (at most one open at a time).
    enum Kind {
        Transcript = 0,
        Settings = 1,
        // App-level manager pages (singletons). The GUI renders these as Nav
        // overlay pages; the TUI hosts them as page tabs with a markdown
        // projection of the same shared seams.
        Models = 2,
        Accounts = 3,
        Profiles = 4,
        Fleet = 5,
        Sessions = 6,
        Dashboard = 7,
        Approvals = 8,
        Routing = 9,
        Cron = 10,
        // Open file tabs: multi-instance (unlike the singleton manager pages),
        // keyed by (rootId, path) rather than a sessionId.
        File = 11,
        // Per-agent surfaces (multi-instance, keyed by an agent ref == ProfileRef):
        // Memory visualization (Mnemosyne, per-profile bank) and the agent's
        // Profile (ProfileSpec) editor. Opening the same agent re-activates its tab.
        Memory = 12,
        Profile = 13,
        // Channels / Events-IO account manager (story 04): transport adapters,
        // configured accounts with status dots, and live per-account rooms.
        Channels = 14,
    };
    Q_ENUM(Kind)

    enum Role {
        TabIdRole = Qt::UserRole + 1, // stable, monotonically-assigned id
        KindRole,                     // Kind
        TitleRole,                    // display label
        SessionIdRole,                // transcript tabs only; "" for pages (string SessionId)
        ClosableRole,                 // false pins the tab open
        CurrentRole,                  // true for the active row
        PreviewRole,                  // true for the transient "preview" tab
        FilePathRole,                 // File tabs: root-relative path
        FileRootRole,                 // File tabs: owning root id
        DirtyRole,                    // File tabs: unsaved-changes flag
        ProfileRole,                  // Memory/Profile tabs: agent ref (ProfileRef)
    };

    explicit TabModel(QObject* parent = nullptr);

    [[nodiscard]] int currentIndex() const { return m_currentIndex; }
    void setCurrentIndex(int index);
    [[nodiscard]] int count() const { return static_cast<int>(m_tabs.size()); }

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Find-or-create a PINNED (permanent) transcript tab for `sessionId`,
    // activate it, and return its stable tab id. An existing tab is reused (title
    // refreshed) and pinned. This is the deliberate "open" path (list double-click
    // / new session).
    Q_INVOKABLE int openTranscript(const QString& sessionId, const QString& title);
    // Alias kept for clarity at call sites; identical to openTranscript.
    Q_INVOKABLE int openTranscriptPinned(const QString& sessionId, const QString& title);

    // VSCode-style transient open: if `sessionId` is already open in any tab,
    // activate it; otherwise reuse the single preview tab (reassigning its
    // session in place and emitting tabSessionChanged) or, if none
    // exists, append a new preview tab. Returns the active tab's id. The preview
    // tab is replaced by the next preview and becomes permanent via pinTab*.
    Q_INVOKABLE int previewTranscript(const QString& sessionId, const QString& title);

    // Pin (make permanent) the tab at `index` / with `tabId` / the active tab:
    // clears its preview flag so the next preview opens a fresh slot instead of
    // replacing it. No-op when already pinned or out of range.
    Q_INVOKABLE void pinTab(int index);
    Q_INVOKABLE void pinTabById(int tabId);
    Q_INVOKABLE void pinCurrent();

    // Find-or-create the singleton page of `kind` (e.g. Settings), activate it,
    // and return its tab id.
    Q_INVOKABLE int openPage(int kind, const QString& title);

    // Per-agent tabs (multi-instance, keyed by (kind, profile)). Used by the
    // Memory and Profile kinds so each agent (ProfileRef) gets its own tab;
    // opening the same agent re-activates the existing tab. Returns the tab id.
    Q_INVOKABLE int openAgentTab(int kind, const QString& profile, const QString& title);
    [[nodiscard]] Q_INVOKABLE QString agentRefAt(int index) const;

    // File tabs (multi-instance, keyed by (rootId, path)). previewFile reuses the
    // single preview slot (VSCode-style transient open); openFilePinned makes a
    // permanent tab. An already-open file is re-activated.
    Q_INVOKABLE int previewFile(const QString& rootId, const QString& path, const QString& title);
    Q_INVOKABLE int openFilePinned(const QString& rootId, const QString& path,
                                   const QString& title);
    // Set the unsaved-changes flag for a File tab (drives the tab dirty dot).
    Q_INVOKABLE void setDirtyById(int tabId, bool dirty);
    [[nodiscard]] Q_INVOKABLE QString filePathAt(int index) const;
    [[nodiscard]] Q_INVOKABLE QString fileRootAt(int index) const;
    // Close the tab at `index` (no-op when out of range or not closable). The
    // active tab moves to a neighbour; emits tabClosed(id) for frontend teardown.
    Q_INVOKABLE void closeTab(int index);
    Q_INVOKABLE void closeTabById(int tabId);
    // Activate the tab at `index` (clamped no-op when out of range).
    Q_INVOKABLE void activate(int index);
    // Cycle the active tab by `delta`, wrapping around. No-op when empty.
    Q_INVOKABLE void cycle(int delta);
    // Reorder: move the tab at `from` to `to` (no-op for out-of-range/equal).
    Q_INVOKABLE void moveTab(int from, int to);
    // Rename the tab at `index`.
    Q_INVOKABLE void setTitle(int index, const QString& title);

    // Frontend accessors keyed by row or id.
    [[nodiscard]] Q_INVOKABLE int tabIdAt(int index) const;
    [[nodiscard]] Q_INVOKABLE int kindAt(int index) const;
    [[nodiscard]] Q_INVOKABLE QString sessionIdAt(int index) const;
    [[nodiscard]] Q_INVOKABLE QString titleAt(int index) const;
    [[nodiscard]] Q_INVOKABLE bool isPreviewAt(int index) const;
    [[nodiscard]] Q_INVOKABLE int indexOfTabId(int tabId) const;

signals:
    void currentIndexChanged();
    void countChanged();
    // The active tab actually changed; carries the new tab id (-1 when empty).
    void currentTabChanged(int tabId);
    // A tab was removed; carries its (now-defunct) tab id so the frontend can
    // dispose of the matching per-tab session.
    void tabClosed(int tabId);
    // A preview tab was reassigned to a different session in place (its tab id
    // is stable). The frontend rebinds the matching per-tab page/session.
    void tabSessionChanged(int tabId, const QString& sessionId);
    // A preview tab changed kind/path in place (e.g. Transcript <-> File). The
    // frontend must tear down the old per-tab controller/session and rebind.
    void tabKindChanged(int tabId);
    // A File preview tab was reassigned to a different root/path in place while
    // keeping the same tab id. Frontends must reload the file controller.
    void tabFileChanged(int tabId, const QString& rootId, const QString& path);

private:
    struct Tab {
        int id = 0;
        int kind = Transcript;
        QString title;
        QString sessionId; // string SessionId for transcript tabs; "" for pages
        bool closable = true;
        bool preview = false; // transient (VSCode-style) tab; at most one exists
        QString rootId;       // File tabs: owning root id
        QString path;         // File tabs: root-relative path
        bool dirty = false;   // File tabs: unsaved changes
        QString profile;      // Memory/Profile tabs: agent ref (ProfileRef)
    };

    // Move the active row to `index` (already validated/clamped by the caller),
    // emitting currentIndexChanged + currentTabChanged + the CurrentRole repaint.
    void setCurrentInternal(int index);
    void emitCurrentChanged(); // dataChanged(CurrentRole) across all rows
    [[nodiscard]] int findTranscriptRow(const QString& sessionId) const;
    [[nodiscard]] int findPageRow(int kind) const;
    [[nodiscard]] int findPreviewRow() const;
    [[nodiscard]] int findFileRow(const QString& rootId, const QString& path) const;
    [[nodiscard]] int findAgentRow(int kind, const QString& profile) const;

    QList<Tab> m_tabs;
    int m_currentIndex = -1;
    int m_nextId = 1;
};
