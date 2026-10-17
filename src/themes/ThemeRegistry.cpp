#include "ThemeRegistry.h"
#include "definitions/SystemDefaultTheme.h"
#include "definitions/ClassicLightTheme.h"
#include "definitions/ClassicDarkTheme.h"
#include "definitions/RetroGreenTheme.h"
#include "definitions/AmberCRTTheme.h"
#include "definitions/SolarizedDarkTheme.h"
#include "definitions/EnchantedLibraryTheme.h"
#include "definitions/DreamscapeIslandsTheme.h"
#include "definitions/MidnightObservatoryTheme.h"

#include <algorithm>

namespace YANP {

ThemeRegistry& ThemeRegistry::instance() {
    static ThemeRegistry reg;
    return reg;
}

ThemeRegistry::ThemeRegistry() {
    registerDefaults();
}

void ThemeRegistry::registerDefaults() {
    m_themes.clear();
    registerTheme(Themes::createSystemDefaultTheme());
    registerTheme(Themes::createClassicLightTheme());
    registerTheme(Themes::createClassicDarkTheme());
    registerTheme(Themes::createRetroGreenTheme());
    registerTheme(Themes::createAmberCRTTheme());
    registerTheme(Themes::createSolarizedDarkTheme());
    registerTheme(Themes::createEnchantedLibraryTheme());
    registerTheme(Themes::createDreamscapeIslandsTheme());
    registerTheme(Themes::createMidnightObservatoryTheme());
}

void ThemeRegistry::resetToDefaults() {
    registerDefaults();
}

bool ThemeRegistry::registerTheme(const Theme& theme) {
    if (!theme.isValid()) {
        return false;
    }
    // Prevent duplicate identifiers
    if (hasTheme(theme.id)) {
        return false;
    }
    m_themes.push_back(theme);
    return true;
}

std::vector<Theme> ThemeRegistry::registeredThemes() const {
    return m_themes;
}

std::optional<Theme> ThemeRegistry::getTheme(const QString& id) const {
    for (const auto& theme : m_themes) {
        if (theme.id == id) {
            return theme;
        }
    }
    return std::nullopt;
}

bool ThemeRegistry::hasTheme(const QString& id) const {
    return std::any_of(m_themes.begin(), m_themes.end(),
        [&id](const Theme& t) { return t.id == id; });
}

Theme ThemeRegistry::defaultTheme() const {
    auto sys = getTheme(QStringLiteral("system-default"));
    if (sys.has_value()) {
        return *sys;
    }
    if (!m_themes.empty()) {
        return m_themes.front();
    }
    return Themes::createSystemDefaultTheme();
}

} // namespace YANP
