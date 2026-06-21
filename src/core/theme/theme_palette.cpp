#include "theme_palette.h"

#include <QHash>

namespace theme {

namespace {

// Pick the per-theme hex for a token. Order matches ThemeName: Light/Dark/Sepia/
// Midnight. QColor::fromString accepts "#rrggbb" and "#aarrggbb" (e.g. the
// translucent listSeparator in Dark).
QColor pick(ThemeName t, const char* light, const char* dark, const char* sepia,
            const char* midnight)
{
    switch (t) {
    case ThemeName::Light:
        return QColor::fromString(QLatin1String(light));
    case ThemeName::Dark:
        return QColor::fromString(QLatin1String(dark));
    case ThemeName::Sepia:
        return QColor::fromString(QLatin1String(sepia));
    case ThemeName::Midnight:
        return QColor::fromString(QLatin1String(midnight));
    }
    return {};
}

const QHash<QString, Token>& tokenIndex()
{
    static const QHash<QString, Token> index = {
        { QStringLiteral("background"), Token::Background },
        { QStringLiteral("listBackground"), Token::ListBackground },
        { QStringLiteral("sidebar"), Token::Sidebar },
        { QStringLiteral("border"), Token::Border },
        { QStringLiteral("splitter"), Token::Splitter },
        { QStringLiteral("text"), Token::Text },
        { QStringLiteral("textMuted"), Token::TextMuted },
        { QStringLiteral("countText"), Token::CountText },
        { QStringLiteral("hover"), Token::Hover },
        { QStringLiteral("pressed"), Token::Pressed },
        { QStringLiteral("rowHover"), Token::RowHover },
        { QStringLiteral("rowActive"), Token::RowActive },
        { QStringLiteral("rowActiveInactive"), Token::RowActiveInactive },
        { QStringLiteral("accent"), Token::Accent },
        { QStringLiteral("sidebarText"), Token::SidebarText },
        { QStringLiteral("listText"), Token::ListText },
        { QStringLiteral("listSnippet"), Token::ListSnippet },
        { QStringLiteral("listSeparator"), Token::ListSeparator },
        { QStringLiteral("searchBackground"), Token::SearchBackground },
        { QStringLiteral("searchBorder"), Token::SearchBorder },
        { QStringLiteral("searchFocusBorder"), Token::SearchFocusBorder },
        { QStringLiteral("searchText"), Token::SearchText },
        { QStringLiteral("searchSelection"), Token::SearchSelection },
        { QStringLiteral("codeBackground"), Token::CodeBackground },
        { QStringLiteral("surfaceRaised"), Token::SurfaceRaised },
        { QStringLiteral("activeBlockBackground"), Token::ActiveBlockBackground },
        { QStringLiteral("activeBlockBorder"), Token::ActiveBlockBorder },
        { QStringLiteral("toolHeader"), Token::ToolHeader },
        { QStringLiteral("statusOk"), Token::StatusOk },
        { QStringLiteral("diffAddBackground"), Token::DiffAddBackground },
        { QStringLiteral("diffDelBackground"), Token::DiffDelBackground },
        { QStringLiteral("diffAddText"), Token::DiffAddText },
        { QStringLiteral("diffDelText"), Token::DiffDelText },
        { QStringLiteral("bubbleUser"), Token::BubbleUser },
        { QStringLiteral("bubbleUserBorder"), Token::BubbleUserBorder },
        { QStringLiteral("roleAvatarAssistant"), Token::RoleAvatarAssistant },
        { QStringLiteral("roleAvatarUserIcon"), Token::RoleAvatarUserIcon },
        { QStringLiteral("iconColor"), Token::IconColor },
        { QStringLiteral("iconMuted"), Token::IconMuted },
        { QStringLiteral("chipLight"), Token::ChipLight },
        { QStringLiteral("chipDark"), Token::ChipDark },
        { QStringLiteral("chipSepia"), Token::ChipSepia },
        { QStringLiteral("chipMidnight"), Token::ChipMidnight },
        { QStringLiteral("statusBarBackground"), Token::StatusBarBackground },
        { QStringLiteral("statusBarText"), Token::StatusBarText },
        { QStringLiteral("statusBarHover"), Token::StatusBarHover },
        { QStringLiteral("warning"), Token::Warning },
        { QStringLiteral("danger"), Token::Danger },
    };
    return index;
}

} // namespace

bool ThemePalette::isKnown(const QString& name)
{
    return name == QStringLiteral("Light") || name == QStringLiteral("Dark")
        || name == QStringLiteral("Sepia") || name == QStringLiteral("Midnight");
}

QStringList ThemePalette::allNames()
{
    return { QStringLiteral("Light"), QStringLiteral("Dark"), QStringLiteral("Sepia"),
             QStringLiteral("Midnight") };
}

ThemeName ThemePalette::fromString(const QString& name)
{
    if (name == QStringLiteral("Dark")) {
        return ThemeName::Dark;
    }
    if (name == QStringLiteral("Sepia")) {
        return ThemeName::Sepia;
    }
    if (name == QStringLiteral("Midnight")) {
        return ThemeName::Midnight;
    }
    return ThemeName::Light;
}

QString ThemePalette::toString(ThemeName name)
{
    switch (name) {
    case ThemeName::Light:
        return QStringLiteral("Light");
    case ThemeName::Dark:
        return QStringLiteral("Dark");
    case ThemeName::Sepia:
        return QStringLiteral("Sepia");
    case ThemeName::Midnight:
        return QStringLiteral("Midnight");
    }
    return QStringLiteral("Light");
}

bool ThemePalette::hasToken(const QString& token)
{
    return tokenIndex().contains(token);
}

QColor ThemePalette::color(const QString& theme, const QString& token)
{
    const auto it = tokenIndex().constFind(token);
    if (it == tokenIndex().constEnd()) {
        return {};
    }
    return color(fromString(theme), it.value());
}

QColor ThemePalette::color(ThemeName t, Token token)
{
    switch (token) {
    case Token::Background:
        return pick(t, "#ffffff", "#191919", "#fbf0d9", "#0d162d");
    case Token::ListBackground:
        // isDark || isMidnight || isSepia ? sidebar : "#f7f7f7"
        return pick(t, "#f7f7f7", "#333333", "#ece0c2", "#09286f");
    case Token::Sidebar:
        return pick(t, "#ededed", "#333333", "#ece0c2", "#09286f");
    case Token::Border:
        return pick(t, "#ededec", "#3a3a3a", "#e6dcc4", "#233a6b");
    case Token::Splitter:
        return pick(t, "#d9d9d9", "#363739", "#bfbfbf", "#1b2d57");
    case Token::Text:
        return pick(t, "#37352e", "#d6d6d6", "#321e03", "#ffe6cb");
    case Token::TextMuted:
        return pick(t, "#7d7c78", "#868686", "#7d7c78", "#b5c7f3");
    case Token::CountText:
        return pick(t, "#848484", "#848484", "#848484", "#8fa6d8");
    case Token::Hover:
        return pick(t, "#efefef", "#313131", "#f1e7d2", "#16224a");
    case Token::Pressed:
        return pick(t, "#dfdfde", "#2c2c2c", "#f1e7d2", "#1b2a55");
    case Token::RowHover:
        return pick(t, "#e6e6e6", "#3d3d3d", "#e3d4ad", "#123882");
    case Token::RowActive:
        return pick(t, "#dcdcdc", "#474747", "#d8c79a", "#1a4499");
    case Token::RowActiveInactive:
        return pick(t, "#e1e1e1", "#424242", "#dfd0a4", "#163a8c");
    case Token::Accent:
        return pick(t, "#2383e2", "#757575", "#b06a2c", "#9fb3e6");
    case Token::SidebarText:
        return pick(t, "#5f5f5c", "#9a9a9a", "#6b5836", "#aebfe6");
    case Token::ListText:
        return pick(t, "#4a4a48", "#b0b0b0", "#5a4a2e", "#cdd9f5");
    case Token::ListSnippet:
        return pick(t, "#8e9296", "#8e9296", "#8e9296", "#8e9296");
    case Token::ListSeparator:
        return pick(t, "#bfbfbf", "#7fffffff", "#bfbfbf", "#2a3f70");
    case Token::SearchBackground:
        return pick(t, "#ffffff", "#2a2a2a", "#fbf0d9", "#0b2150");
    case Token::SearchBorder:
        return pick(t, "#cdcdcd", "#2a2a2a", "#cdcdcd", "#233a6b");
    case Token::SearchFocusBorder:
        return pick(t, "#a6c6e4", "#2c536f", "#a6c6e4", "#3a63bd");
    case Token::SearchText:
        return pick(t, "#1a1a1a", "#cfcfcf", "#321e03", "#e6ecff");
    case Token::SearchSelection:
        return pick(t, "#d2e4fa", "#d2e4fa", "#d2e4fa", "#2f4f99");
    case Token::CodeBackground:
        return pick(t, "#f1f1ef", "#2a2a2a", "#efe6d2", "#1a2c5c");
    case Token::SurfaceRaised:
        return pick(t, "#f7f7f5", "#202020", "#f3ebd6", "#122247");
    case Token::ActiveBlockBackground:
        return pick(t, "#f8fbff", "#1f2733", "#f3ead2", "#14224a");
    case Token::ActiveBlockBorder:
        return pick(t, "#d7e9fb", "#2c4a63", "#d8c79a", "#2c4a8f");
    case Token::ToolHeader:
        return pick(t, "#eef0f2", "#242424", "#efe4c8", "#0e1d40");
    case Token::StatusOk:
        // isDarkMode ? "#5fbf73" : "#2f9e44"
        return pick(t, "#2f9e44", "#5fbf73", "#2f9e44", "#5fbf73");
    case Token::DiffAddBackground:
        return pick(t, "#e6ffec", "#10301c", "#e6ffec", "#10301c");
    case Token::DiffDelBackground:
        return pick(t, "#ffebe9", "#3a1d1d", "#ffebe9", "#3a1d1d");
    case Token::DiffAddText:
        return pick(t, "#1a7f37", "#7ee787", "#1a7f37", "#7ee787");
    case Token::DiffDelText:
        return pick(t, "#cf222e", "#ff7b72", "#cf222e", "#ff7b72");
    case Token::BubbleUser:
        return pick(t, "#eef4fb", "#222a36", "#f0e6cf", "#13234d");
    case Token::BubbleUserBorder:
        return pick(t, "#d7e6f6", "#33414f", "#dccca3", "#28467f");
    case Token::RoleAvatarAssistant:
        return pick(t, "#e2e2e2", "#3a3a3a", "#d8c7a0", "#2a3a63");
    case Token::RoleAvatarUserIcon:
        return pick(t, "#ffffff", "#ffffff", "#ffffff", "#ffffff");
    case Token::IconColor:
        return pick(t, "#000000", "#5b94f5", "#000000", "#6fa0ff");
    case Token::IconMuted:
        return pick(t, "#646464", "#a2a3a4", "#646464", "#9fb3e6");
    case Token::ChipLight:
        return QColor::fromString(QLatin1String("#f7f7f7"));
    case Token::ChipDark:
        return QColor::fromString(QLatin1String("#191919"));
    case Token::ChipSepia:
        return QColor::fromString(QLatin1String("#f7cc6f"));
    case Token::ChipMidnight:
        return QColor::fromString(QLatin1String("#0d2f86"));
    case Token::StatusBarBackground:
        return pick(t, "#f2f2f2", "#272727", "#efe4c8", "#0a1f57");
    case Token::StatusBarText:
        return pick(t, "#6b6a66", "#8a8a8a", "#6b6a66", "#8fa6d8");
    case Token::StatusBarHover:
        return pick(t, "#e4e4e4", "#343434", "#e4d8b8", "#142b66");
    case Token::Warning:
        return pick(t, "#b97e16", "#e0a93b", "#b97e16", "#e0a93b");
    case Token::Danger:
        return pick(t, "#c0392b", "#f06a6a", "#c0392b", "#f06a6a");
    }
    return {};
}

} // namespace theme

ThemeTokens::ThemeTokens(QObject* parent) : QObject(parent) {}

QColor ThemeTokens::colorFor(const QString& theme, const QString& token) const
{
    return theme::ThemePalette::color(theme, token);
}

bool ThemeTokens::isKnownTheme(const QString& name) const
{
    return theme::ThemePalette::isKnown(name);
}
