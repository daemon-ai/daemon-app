#include "uisettings.h"

#include "theme/theme_palette.h"

#include <QVariant>

#include <algorithm>

namespace {

// text-style categories. The dropdown shows the friendly label; the
// transcript applies the resolved family (the iA Writer faces embed a trailing
// " S" in their family name, so label != family there).
struct FontEntry {
    const char* label;
    const char* family;
};

const QList<FontEntry>& sansTable()
{
    static const QList<FontEntry> table = { { "Inter", "Inter" } };
    return table;
}

const QList<FontEntry>& serifTable()
{
    static const QList<FontEntry> table = {
        { "Trykker", "Trykker" },
        { "Ibarra Real Nova", "Ibarra Real Nova" },
    };
    return table;
}

const QList<FontEntry>& monoTable()
{
    static const QList<FontEntry> table = {
        { "iA Writer Mono", "iA Writer Mono S" },
        { "iA Writer Duo", "iA Writer Duo S" },
        { "iA Writer Quattro", "iA Writer Quattro S" },
    };
    return table;
}

const QList<FontEntry>& tableForCategory(const QString& category)
{
    if (category == QStringLiteral("Serif")) {
        return serifTable();
    }
    if (category == QStringLiteral("Mono")) {
        return monoTable();
    }
    return sansTable();
}

QStringList labelsOf(const QList<FontEntry>& table)
{
    QStringList out;
    out.reserve(table.size());
    for (const FontEntry& e : table) {
        out << QString::fromUtf8(e.label);
    }
    return out;
}

bool isKnownCategory(const QString& category)
{
    return category == QStringLiteral("Sans") || category == QStringLiteral("Serif")
        || category == QStringLiteral("Mono");
}

bool isKnownTheme(const QString& theme)
{
    // Single-sourced with the GUI palette + the TUI: the set of valid theme names
    // lives in the shared theme lib.
    return theme::ThemePalette::isKnown(theme);
}

} // namespace

UiSettings::UiSettings(QObject* parent) : QObject(parent)
{
    load();
}

void UiSettings::load()
{
    m_settings.beginGroup(QStringLiteral("ui"));
    const QString theme = m_settings.value(QStringLiteral("theme"), m_theme).toString();
    if (isKnownTheme(theme)) {
        m_theme = theme;
    }
    const QString category
        = m_settings.value(QStringLiteral("fontCategory"), m_fontCategory).toString();
    if (isKnownCategory(category)) {
        m_fontCategory = category;
    }
    m_sansIndex = m_settings.value(QStringLiteral("sansIndex"), m_sansIndex).toInt();
    m_serifIndex = m_settings.value(QStringLiteral("serifIndex"), m_serifIndex).toInt();
    m_monoIndex = m_settings.value(QStringLiteral("monoIndex"), m_monoIndex).toInt();
    m_editorFontSize
        = m_settings.value(QStringLiteral("fontSize"), m_editorFontSize).toInt();
    m_centerText = m_settings.value(QStringLiteral("centerText"), m_centerText).toBool();
    m_distractionFree
        = m_settings.value(QStringLiteral("distractionFree"), m_distractionFree).toBool();
    m_showNotesList
        = m_settings.value(QStringLiteral("showNotesList"), m_showNotesList).toBool();
    m_showFoldersTree
        = m_settings.value(QStringLiteral("showFoldersTree"), m_showFoldersTree).toBool();
    m_showPlainText
        = m_settings.value(QStringLiteral("showPlainText"), m_showPlainText).toBool();
    m_showUserRail
        = m_settings.value(QStringLiteral("showUserRail"), m_showUserRail).toBool();
    m_settings.endGroup();

    // Clamp persisted values defensively (font tables / limits can change between
    // versions).
    m_editorFontSize = std::clamp(m_editorFontSize, kMinFontSize, kMaxFontSize);
    m_sansIndex = std::clamp(m_sansIndex, 0, static_cast<int>(sansTable().size()) - 1);
    m_serifIndex = std::clamp(m_serifIndex, 0, static_cast<int>(serifTable().size()) - 1);
    m_monoIndex = std::clamp(m_monoIndex, 0, static_cast<int>(monoTable().size()) - 1);
}

void UiSettings::store(const QString& key, const QVariant& value)
{
    m_settings.setValue(QStringLiteral("ui/") + key, value);
}

int* UiSettings::indexForCategory(const QString& category)
{
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

void UiSettings::setTheme(const QString& theme)
{
    if (theme == m_theme || !isKnownTheme(theme)) {
        return;
    }
    m_theme = theme;
    store(QStringLiteral("theme"), m_theme);
    emit themeChanged();
}

void UiSettings::setFontCategory(const QString& category)
{
    if (category == m_fontCategory || !isKnownCategory(category)) {
        return;
    }
    m_fontCategory = category;
    store(QStringLiteral("fontCategory"), m_fontCategory);
    emit fontStyleChanged();
}

QStringList UiSettings::sansFonts() const { return labelsOf(sansTable()); }
QStringList UiSettings::serifFonts() const { return labelsOf(serifTable()); }
QStringList UiSettings::monoFonts() const { return labelsOf(monoTable()); }

QString UiSettings::editorFontFamily() const
{
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

QString UiSettings::familyAt(const QString& category, int index) const
{
    const QList<FontEntry>& table = tableForCategory(category);
    const int clamped = std::clamp(index, 0, static_cast<int>(table.size()) - 1);
    return QString::fromUtf8(table.at(clamped).family);
}

void UiSettings::selectFont(const QString& category, int index)
{
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

void UiSettings::cycleFont(const QString& category)
{
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

void UiSettings::setEditorFontSize(int size)
{
    const int clamped = std::clamp(size, kMinFontSize, kMaxFontSize);
    if (clamped == m_editorFontSize) {
        return;
    }
    m_editorFontSize = clamped;
    store(QStringLiteral("fontSize"), m_editorFontSize);
    emit editorFontSizeChanged();
}

void UiSettings::setCenterText(bool on)
{
    if (on == m_centerText) {
        return;
    }
    m_centerText = on;
    store(QStringLiteral("centerText"), m_centerText);
    emit centerTextChanged();
}

void UiSettings::setDistractionFree(bool on)
{
    if (on == m_distractionFree) {
        return;
    }
    m_distractionFree = on;
    store(QStringLiteral("distractionFree"), m_distractionFree);
    emit distractionFreeChanged();
}

void UiSettings::setShowNotesList(bool on)
{
    if (on == m_showNotesList) {
        return;
    }
    m_showNotesList = on;
    store(QStringLiteral("showNotesList"), m_showNotesList);
    emit showNotesListChanged();
}

void UiSettings::setShowFoldersTree(bool on)
{
    if (on == m_showFoldersTree) {
        return;
    }
    m_showFoldersTree = on;
    store(QStringLiteral("showFoldersTree"), m_showFoldersTree);
    emit showFoldersTreeChanged();
}

void UiSettings::setShowPlainText(bool on)
{
    if (on == m_showPlainText) {
        return;
    }
    m_showPlainText = on;
    store(QStringLiteral("showPlainText"), m_showPlainText);
    emit showPlainTextChanged();
}

void UiSettings::setShowUserRail(bool on)
{
    if (on == m_showUserRail) {
        return;
    }
    m_showUserRail = on;
    store(QStringLiteral("showUserRail"), m_showUserRail);
    emit showUserRailChanged();
}

void UiSettings::resetAll()
{
    setTheme(QStringLiteral("Light"));
    setFontCategory(QStringLiteral("Sans"));
    selectFont(QStringLiteral("Sans"), 0);
    selectFont(QStringLiteral("Serif"), 0);
    selectFont(QStringLiteral("Mono"), 0);
    setFontCategory(QStringLiteral("Sans"));
    setEditorFontSize(kDefaultFontSize);
    setCenterText(false);
    setDistractionFree(false);
    setShowNotesList(true);
    setShowFoldersTree(true);
    setShowPlainText(false);
    setShowUserRail(true);
}
