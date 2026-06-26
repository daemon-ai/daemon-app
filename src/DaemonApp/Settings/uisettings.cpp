#include "uisettings.h"

#include "i18n/localization.h"
#include "theme/theme_palette.h"

#include <algorithm>
#include <QVariant>
#include <QVariantMap>

namespace {

// text-style categories. The dropdown shows the friendly label; the
// transcript applies the resolved family (the iA Writer faces embed a trailing
// " S" in their family name, so label != family there).
struct FontEntry {
    const char* label;
    const char* family;
};

const QList<FontEntry>& sansTable() {
    static const QList<FontEntry> table = {{"Inter", "Inter"}};
    return table;
}

const QList<FontEntry>& serifTable() {
    static const QList<FontEntry> table = {
        {"Trykker", "Trykker"},
        {"Ibarra Real Nova", "Ibarra Real Nova"},
    };
    return table;
}

const QList<FontEntry>& monoTable() {
    static const QList<FontEntry> table = {
        {"iA Writer Mono", "iA Writer Mono S"},
        {"iA Writer Duo", "iA Writer Duo S"},
        {"iA Writer Quattro", "iA Writer Quattro S"},
    };
    return table;
}

const QList<FontEntry>& tableForCategory(const QString& category) {
    if (category == QStringLiteral("Serif")) {
        return serifTable();
    }
    if (category == QStringLiteral("Mono")) {
        return monoTable();
    }
    return sansTable();
}

QStringList labelsOf(const QList<FontEntry>& table) {
    QStringList out;
    out.reserve(table.size());
    for (const FontEntry& e : table) {
        out << QString::fromUtf8(e.label);
    }
    return out;
}

bool isKnownCategory(const QString& category) {
    return category == QStringLiteral("Sans") || category == QStringLiteral("Serif") ||
           category == QStringLiteral("Mono");
}

bool isKnownTheme(const QString& theme) {
    // Single-sourced with the GUI palette + the TUI: the set of valid theme names
    // lives in the shared theme lib.
    return theme::ThemePalette::isKnown(theme);
}

bool isKnownLanguage(const QString& code) {
    return std::ranges::any_of(i18n::availableLocales(),
                               [&](const i18n::LocaleOption& opt) { return opt.code == code; });
}

} // namespace

UiSettings::UiSettings(QObject* parent) : QObject(parent) {
    load();
}

void UiSettings::load() {
    m_settings.beginGroup(QStringLiteral("ui"));
    const QString theme = m_settings.value(QStringLiteral("theme"), m_theme).toString();
    if (isKnownTheme(theme)) {
        m_theme = theme;
    }
    const QString language = m_settings.value(QStringLiteral("language"), m_language).toString();
    if (isKnownLanguage(language)) {
        m_language = language;
    }
    const QString category =
        m_settings.value(QStringLiteral("fontCategory"), m_fontCategory).toString();
    if (isKnownCategory(category)) {
        m_fontCategory = category;
    }
    m_sansIndex = m_settings.value(QStringLiteral("sansIndex"), m_sansIndex).toInt();
    m_serifIndex = m_settings.value(QStringLiteral("serifIndex"), m_serifIndex).toInt();
    m_monoIndex = m_settings.value(QStringLiteral("monoIndex"), m_monoIndex).toInt();
    m_editorFontSize = m_settings.value(QStringLiteral("fontSize"), m_editorFontSize).toInt();
    m_centerText = m_settings.value(QStringLiteral("centerText"), m_centerText).toBool();
    m_distractionFree =
        m_settings.value(QStringLiteral("distractionFree"), m_distractionFree).toBool();
    m_showSessionsList =
        m_settings.value(QStringLiteral("showSessionsList"), m_showSessionsList).toBool();
    m_showFleetTree = m_settings.value(QStringLiteral("showFleetTree"), m_showFleetTree).toBool();
    m_showFileExplorer =
        m_settings.value(QStringLiteral("showFileExplorer"), m_showFileExplorer).toBool();
    m_showRawMarkdown =
        m_settings.value(QStringLiteral("showRawMarkdown"), m_showRawMarkdown).toBool();
    m_showUserRail = m_settings.value(QStringLiteral("showUserRail"), m_showUserRail).toBool();
    m_settings.endGroup();

    // Clamp persisted values defensively (font tables / limits can change between
    // versions).
    m_editorFontSize = std::clamp(m_editorFontSize, kMinFontSize, kMaxFontSize);
    m_sansIndex = std::clamp(m_sansIndex, 0, static_cast<int>(sansTable().size()) - 1);
    m_serifIndex = std::clamp(m_serifIndex, 0, static_cast<int>(serifTable().size()) - 1);
    m_monoIndex = std::clamp(m_monoIndex, 0, static_cast<int>(monoTable().size()) - 1);
}

void UiSettings::store(const QString& key, const QVariant& value) {
    m_settings.setValue(QStringLiteral("ui/") + key, value);
}

int* UiSettings::indexForCategory(const QString& category) {
    if (category == QStringLiteral("Serif")) {
        return &m_serifIndex;
    }
    if (category == QStringLiteral("Mono")) {
        return &m_monoIndex;
    }
    if (category == QStringLiteral("Sans")) {
        return &m_sansIndex;
    }
    return nullptr;
}

void UiSettings::setTheme(const QString& theme) {
    if (theme == m_theme || !isKnownTheme(theme)) {
        return;
    }
    m_theme = theme;
    store(QStringLiteral("theme"), m_theme);
    emit themeChanged();
}

void UiSettings::setLanguage(const QString& language) {
    if (language == m_language || !isKnownLanguage(language)) {
        return;
    }
    m_language = language;
    store(QStringLiteral("language"), m_language);
    emit languageChanged();
}

QVariantList UiSettings::availableLanguages() const {
    QVariantList out;
    for (const i18n::LocaleOption& opt : i18n::availableLocales()) {
        out.append(
            QVariantMap{{QStringLiteral("code"), opt.code}, {QStringLiteral("label"), opt.label}});
    }
    return out;
}

void UiSettings::setFontCategory(const QString& category) {
    if (category == m_fontCategory || !isKnownCategory(category)) {
        return;
    }
    m_fontCategory = category;
    store(QStringLiteral("fontCategory"), m_fontCategory);
    emit fontStyleChanged();
}

QStringList UiSettings::sansFonts() const {
    return labelsOf(sansTable());
}
QStringList UiSettings::serifFonts() const {
    return labelsOf(serifTable());
}
QStringList UiSettings::monoFonts() const {
    return labelsOf(monoTable());
}

QString UiSettings::editorFontFamily() const {
    const QList<FontEntry>& table = tableForCategory(m_fontCategory);
    int index = 0;
    if (m_fontCategory == QStringLiteral("Serif")) {
        index = m_serifIndex;
    } else if (m_fontCategory == QStringLiteral("Mono")) {
        index = m_monoIndex;
    } else {
        index = m_sansIndex;
    }
    index = std::clamp(index, 0, static_cast<int>(table.size()) - 1);
    return QString::fromUtf8(table.at(index).family);
}

QString UiSettings::familyAt(const QString& category, int index) const {
    const QList<FontEntry>& table = tableForCategory(category);
    const int clamped = std::clamp(index, 0, static_cast<int>(table.size()) - 1);
    return QString::fromUtf8(table.at(clamped).family);
}

void UiSettings::selectFont(const QString& category, int index) {
    int* slot = indexForCategory(category);
    if (slot == nullptr) {
        return;
    }
    const QList<FontEntry>& table = tableForCategory(category);
    const int clamped = std::clamp(index, 0, static_cast<int>(table.size()) - 1);

    bool changed = false;
    if (*slot != clamped) {
        *slot = clamped;
        const QString key = category.toLower() + QStringLiteral("Index");
        store(key, clamped);
        changed = true;
    }
    if (m_fontCategory != category) {
        m_fontCategory = category;
        store(QStringLiteral("fontCategory"), m_fontCategory);
        changed = true;
    }
    if (changed) {
        emit fontStyleChanged();
    }
}

void UiSettings::cycleFont(const QString& category) {
    int* slot = indexForCategory(category);
    if (slot == nullptr) {
        return;
    }
    const QList<FontEntry>& table = tableForCategory(category);
    // Activate the category first; only advance the font when it was already
    // active (so the first click just switches style).
    if (m_fontCategory != category) {
        setFontCategory(category);
        return;
    }
    if (table.size() <= 1) {
        return;
    }
    *slot = (*slot + 1) % static_cast<int>(table.size());
    store(category.toLower() + QStringLiteral("Index"), *slot);
    emit fontStyleChanged();
}

void UiSettings::setEditorFontSize(int size) {
    const int clamped = std::clamp(size, kMinFontSize, kMaxFontSize);
    if (clamped == m_editorFontSize) {
        return;
    }
    m_editorFontSize = clamped;
    store(QStringLiteral("fontSize"), m_editorFontSize);
    emit editorFontSizeChanged();
}

void UiSettings::setCenterText(bool on) {
    if (on == m_centerText) {
        return;
    }
    m_centerText = on;
    store(QStringLiteral("centerText"), m_centerText);
    emit centerTextChanged();
}

void UiSettings::setDistractionFree(bool on) {
    if (on == m_distractionFree) {
        return;
    }
    m_distractionFree = on;
    store(QStringLiteral("distractionFree"), m_distractionFree);
    emit distractionFreeChanged();
}

void UiSettings::setShowSessionsList(bool on) {
    if (on == m_showSessionsList) {
        return;
    }
    m_showSessionsList = on;
    store(QStringLiteral("showSessionsList"), m_showSessionsList);
    emit showSessionsListChanged();
}

void UiSettings::setShowFleetTree(bool on) {
    if (on == m_showFleetTree) {
        return;
    }
    m_showFleetTree = on;
    store(QStringLiteral("showFleetTree"), m_showFleetTree);
    emit showFleetTreeChanged();
}

void UiSettings::setShowFileExplorer(bool on) {
    if (on == m_showFileExplorer) {
        return;
    }
    m_showFileExplorer = on;
    store(QStringLiteral("showFileExplorer"), m_showFileExplorer);
    emit showFileExplorerChanged();
}

void UiSettings::setShowRawMarkdown(bool on) {
    if (on == m_showRawMarkdown) {
        return;
    }
    m_showRawMarkdown = on;
    store(QStringLiteral("showRawMarkdown"), m_showRawMarkdown);
    emit showRawMarkdownChanged();
}

void UiSettings::setShowUserRail(bool on) {
    if (on == m_showUserRail) {
        return;
    }
    m_showUserRail = on;
    store(QStringLiteral("showUserRail"), m_showUserRail);
    emit showUserRailChanged();
}

void UiSettings::resetAll() {
    setTheme(QStringLiteral("Light"));
    setLanguage(QStringLiteral("system"));
    setFontCategory(QStringLiteral("Sans"));
    selectFont(QStringLiteral("Sans"), 0);
    selectFont(QStringLiteral("Serif"), 0);
    selectFont(QStringLiteral("Mono"), 0);
    setFontCategory(QStringLiteral("Sans"));
    setEditorFontSize(kDefaultFontSize);
    setCenterText(false);
    setDistractionFree(false);
    setShowSessionsList(true);
    setShowFleetTree(true);
    setShowRawMarkdown(false);
    setShowUserRail(true);
}
