#include "MidnightObservatoryTheme.h"

namespace YANP::Themes {

Theme createMidnightObservatoryTheme() {
    Theme theme;
    theme.id = QStringLiteral("midnight-observatory");
    theme.name = QStringLiteral("Midnight Observatory");
    theme.isSystemDefault = false;

    theme.windowBackground = QColor(0x08, 0x0B, 0x16);
    theme.editorBackground = QColor(0x0B, 0x10, 0x20);
    theme.textColor = QColor(0xDC, 0xE7, 0xFF);
    theme.selectionBackground = QColor(0x29, 0x3B, 0x62);
    theme.selectedTextColor = QColor(0xFF, 0xFF, 0xFF);
    theme.caretColor = QColor(0x7D, 0xE2, 0xFF);
    theme.statusBarBackground = QColor(0x15, 0x1B, 0x30);
    theme.statusBarTextColor = QColor(0xA9, 0xB9, 0xE8);
    theme.borderColor = QColor(0x1F, 0x29, 0x42);

    theme.defaultFontFamily = QStringLiteral("Monospace");
    theme.defaultFontSize = 11;

    return theme;
}

} // namespace YANP::Themes
