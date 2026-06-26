#pragma once

#include <QObject>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QtQml/qqmlregistration.h>
#include <QVariantList>

// Single source of truth for user-facing UI preferences, persisted via QSettings
// so they survive restarts. Exposed to QML as a
// singleton (DaemonApp.Settings.UiSettings); the settings menu mutates it and the
// rest of the UI binds to it (theme, editor font/size, center text, distraction
// free, and the three-pane visibility options).
//
// Theme stays mirrored here for persistence, but DaemonApp.Theme remains the live
// token switch: the menu calls Theme.setTheme() and App writes the result back.
class UiSettings : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QString theme READ theme WRITE setTheme NOTIFY themeChanged)

    // UI language. Persisted as a code ("system"/"en"/"ar"/"pseudo"); the App
    // (and TUI at startup) load the matching translations. availableLanguages is
    // the {code,label} list the settings picker renders.
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)
    Q_PROPERTY(QVariantList availableLanguages READ availableLanguages CONSTANT)

    // Editor text style: a category (Sans/Serif/Mono) plus the chosen font index
    // within that category. editorFontFamily is the resolved family name the
    // transcript applies.
    Q_PROPERTY(QString fontCategory READ fontCategory WRITE setFontCategory NOTIFY fontStyleChanged)
    Q_PROPERTY(int sansIndex READ sansIndex NOTIFY fontStyleChanged)
    Q_PROPERTY(int serifIndex READ serifIndex NOTIFY fontStyleChanged)
    Q_PROPERTY(int monoIndex READ monoIndex NOTIFY fontStyleChanged)
    Q_PROPERTY(QStringList sansFonts READ sansFonts CONSTANT)
    Q_PROPERTY(QStringList serifFonts READ serifFonts CONSTANT)
    Q_PROPERTY(QStringList monoFonts READ monoFonts CONSTANT)
    Q_PROPERTY(QString editorFontFamily READ editorFontFamily NOTIFY fontStyleChanged)

    Q_PROPERTY(
        int editorFontSize READ editorFontSize WRITE setEditorFontSize NOTIFY editorFontSizeChanged)
    Q_PROPERTY(int minFontSize READ minFontSize CONSTANT)
    Q_PROPERTY(int maxFontSize READ maxFontSize CONSTANT)

    Q_PROPERTY(bool centerText READ centerText WRITE setCenterText NOTIFY centerTextChanged)
    Q_PROPERTY(bool distractionFree READ distractionFree WRITE setDistractionFree NOTIFY
                   distractionFreeChanged)

    Q_PROPERTY(bool showSessionsList READ showSessionsList WRITE setShowSessionsList NOTIFY
                   showSessionsListChanged)
    Q_PROPERTY(
        bool showFleetTree READ showFleetTree WRITE setShowFleetTree NOTIFY showFleetTreeChanged)
    Q_PROPERTY(bool showFileExplorer READ showFileExplorer WRITE setShowFileExplorer NOTIFY
                   showFileExplorerChanged)
    Q_PROPERTY(bool showTerminal READ showTerminal WRITE setShowTerminal NOTIFY showTerminalChanged)
    // Last working directory of the embedded terminal, persisted so a reopened
    // session resumes where it left off (empty => fall back to $HOME).
    Q_PROPERTY(QString terminalWorkingDirectory READ terminalWorkingDirectory WRITE
                   setTerminalWorkingDirectory NOTIFY terminalWorkingDirectoryChanged)
    Q_PROPERTY(bool showRawMarkdown READ showRawMarkdown WRITE setShowRawMarkdown NOTIFY
                   showRawMarkdownChanged)
    Q_PROPERTY(bool showUserRail READ showUserRail WRITE setShowUserRail NOTIFY showUserRailChanged)

public:
    explicit UiSettings(QObject* parent = nullptr);

    [[nodiscard]] QString theme() const { return m_theme; }
    void setTheme(const QString& theme);

    [[nodiscard]] QString language() const { return m_language; }
    void setLanguage(const QString& language);
    [[nodiscard]] QVariantList availableLanguages() const;

    [[nodiscard]] QString fontCategory() const { return m_fontCategory; }
    void setFontCategory(const QString& category);

    [[nodiscard]] int sansIndex() const { return m_sansIndex; }
    [[nodiscard]] int serifIndex() const { return m_serifIndex; }
    [[nodiscard]] int monoIndex() const { return m_monoIndex; }

    [[nodiscard]] QStringList sansFonts() const;
    [[nodiscard]] QStringList serifFonts() const;
    [[nodiscard]] QStringList monoFonts() const;

    [[nodiscard]] QString editorFontFamily() const;

    [[nodiscard]] int editorFontSize() const { return m_editorFontSize; }
    void setEditorFontSize(int size);
    [[nodiscard]] int minFontSize() const { return kMinFontSize; }
    [[nodiscard]] int maxFontSize() const { return kMaxFontSize; }

    [[nodiscard]] bool centerText() const { return m_centerText; }
    void setCenterText(bool on);
    [[nodiscard]] bool distractionFree() const { return m_distractionFree; }
    void setDistractionFree(bool on);

    [[nodiscard]] bool showSessionsList() const { return m_showSessionsList; }
    void setShowSessionsList(bool on);
    [[nodiscard]] bool showFleetTree() const { return m_showFleetTree; }
    void setShowFleetTree(bool on);
    [[nodiscard]] bool showFileExplorer() const { return m_showFileExplorer; }
    void setShowFileExplorer(bool on);
    [[nodiscard]] bool showTerminal() const { return m_showTerminal; }
    void setShowTerminal(bool on);
    [[nodiscard]] QString terminalWorkingDirectory() const { return m_terminalWorkingDirectory; }
    void setTerminalWorkingDirectory(const QString& dir);
    [[nodiscard]] bool showRawMarkdown() const { return m_showRawMarkdown; }
    void setShowRawMarkdown(bool on);
    [[nodiscard]] bool showUserRail() const { return m_showUserRail; }
    void setShowUserRail(bool on);

    // Resolved family name for a category's font index (label != family for the
    // iA Writer faces). Used by the Style cards to render each "Ag" preview.
    Q_INVOKABLE QString familyAt(const QString& category, int index) const;

    // Pick a specific font within a category (from the dropdown). Selecting a font
    // also makes that category active.
    Q_INVOKABLE void selectFont(const QString& category, int index);
    // Cycle to the next font within a category and make it active (clicking the
    // big "Ag" sample, index == -1 behavior).
    Q_INVOKABLE void cycleFont(const QString& category);

    Q_INVOKABLE void increaseFontSize() { setEditorFontSize(m_editorFontSize + 1); }
    Q_INVOKABLE void decreaseFontSize() { setEditorFontSize(m_editorFontSize - 1); }

    // Restore every preference to its default (does not touch session data).
    Q_INVOKABLE void resetAll();

signals:
    void themeChanged();
    void languageChanged();
    void fontStyleChanged();
    void editorFontSizeChanged();
    void centerTextChanged();
    void distractionFreeChanged();
    void showSessionsListChanged();
    void showFleetTreeChanged();
    void showFileExplorerChanged();
    void showTerminalChanged();
    void terminalWorkingDirectoryChanged();
    void showRawMarkdownChanged();
    void showUserRailChanged();

private:
    static constexpr int kMinFontSize = 9;
    static constexpr int kMaxFontSize = 28;
    static constexpr int kDefaultFontSize = 15;

    void load();
    void store(const QString& key, const QVariant& value);
    [[nodiscard]] int* indexForCategory(const QString& category);

    QSettings m_settings;

    QString m_theme = QStringLiteral("Light");
    QString m_language = QStringLiteral("system");
    QString m_fontCategory = QStringLiteral("Sans");
    int m_sansIndex = 0;
    int m_serifIndex = 0;
    int m_monoIndex = 0;
    int m_editorFontSize = kDefaultFontSize;
    bool m_centerText = false;
    bool m_distractionFree = false;
    bool m_showSessionsList = true;
    bool m_showFleetTree = true;
    bool m_showFileExplorer = false;
    bool m_showTerminal = false;
    QString m_terminalWorkingDirectory;
    bool m_showRawMarkdown = false;
    bool m_showUserRail = true;
};
