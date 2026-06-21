#pragma once

#include <QObject>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QtQml/qqmlregistration.h>

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

    // Editor text style: a category (Sans/Serif/Mono) plus the chosen font index
    // within that category. editorFontFamily is the resolved family name the
    // transcript applies.
    Q_PROPERTY(QString fontCategory READ fontCategory WRITE setFontCategory NOTIFY
                   fontStyleChanged)
    Q_PROPERTY(int sansIndex READ sansIndex NOTIFY fontStyleChanged)
    Q_PROPERTY(int serifIndex READ serifIndex NOTIFY fontStyleChanged)
    Q_PROPERTY(int monoIndex READ monoIndex NOTIFY fontStyleChanged)
    Q_PROPERTY(QStringList sansFonts READ sansFonts CONSTANT)
    Q_PROPERTY(QStringList serifFonts READ serifFonts CONSTANT)
    Q_PROPERTY(QStringList monoFonts READ monoFonts CONSTANT)
    Q_PROPERTY(QString editorFontFamily READ editorFontFamily NOTIFY fontStyleChanged)

    Q_PROPERTY(int editorFontSize READ editorFontSize WRITE setEditorFontSize NOTIFY
                   editorFontSizeChanged)
    Q_PROPERTY(int minFontSize READ minFontSize CONSTANT)
    Q_PROPERTY(int maxFontSize READ maxFontSize CONSTANT)

    Q_PROPERTY(bool centerText READ centerText WRITE setCenterText NOTIFY centerTextChanged)
    Q_PROPERTY(bool distractionFree READ distractionFree WRITE setDistractionFree NOTIFY
                   distractionFreeChanged)

    Q_PROPERTY(
        bool showNotesList READ showNotesList WRITE setShowNotesList NOTIFY showNotesListChanged)
    Q_PROPERTY(bool showFoldersTree READ showFoldersTree WRITE setShowFoldersTree NOTIFY
                   showFoldersTreeChanged)
    Q_PROPERTY(
        bool showPlainText READ showPlainText WRITE setShowPlainText NOTIFY showPlainTextChanged)
    Q_PROPERTY(
        bool showUserRail READ showUserRail WRITE setShowUserRail NOTIFY showUserRailChanged)

public:
    explicit UiSettings(QObject* parent = nullptr);

    [[nodiscard]] QString theme() const { return m_theme; }
    void setTheme(const QString& theme);

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

    [[nodiscard]] bool showNotesList() const { return m_showNotesList; }
    void setShowNotesList(bool on);
    [[nodiscard]] bool showFoldersTree() const { return m_showFoldersTree; }
    void setShowFoldersTree(bool on);
    [[nodiscard]] bool showPlainText() const { return m_showPlainText; }
    void setShowPlainText(bool on);
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

    // Restore every preference to its default (does not touch conversation data).
    Q_INVOKABLE void resetAll();

signals:
    void themeChanged();
    void fontStyleChanged();
    void editorFontSizeChanged();
    void centerTextChanged();
    void distractionFreeChanged();
    void showNotesListChanged();
    void showFoldersTreeChanged();
    void showPlainTextChanged();
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
    QString m_fontCategory = QStringLiteral("Sans");
    int m_sansIndex = 0;
    int m_serifIndex = 0;
    int m_monoIndex = 0;
    int m_editorFontSize = kDefaultFontSize;
    bool m_centerText = false;
    bool m_distractionFree = false;
    bool m_showNotesList = true;
    bool m_showFoldersTree = true;
    bool m_showPlainText = false;
    bool m_showUserRail = true;
};
