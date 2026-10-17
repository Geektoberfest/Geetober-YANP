#include "DreamscapeIslandsTheme.h"

namespace YANP::Themes {

Theme createDreamscapeIslandsTheme() {
    Theme theme;
    theme.id = QStringLiteral("dreamscape-islands");
    theme.name = QStringLiteral("Dreamscape: Floating Islands");
    theme.isSystemDefault = false;

    theme.windowBackground = QColor(0x10, 0x13, 0x23);
    theme.editorBackground = QColor(0x17, 0x18, 0x2B);
    theme.textColor = QColor(0xE9, 0xE5, 0xFF);
    theme.selectionBackground = QColor(0x51, 0x45, 0x6F);
    theme.selectedTextColor = QColor(0xFF, 0xFF, 0xFF);
    theme.caretColor = QColor(0xFF, 0xCA, 0x9B);
    theme.statusBarBackground = QColor(0x25, 0x25, 0x40);
    theme.statusBarTextColor = QColor(0xC4, 0xBD, 0xE8);
    theme.borderColor = QColor(0x38, 0x38, 0x5C);

    theme.defaultFontFamily = QStringLiteral("Monospace");
    theme.defaultFontSize = 11;

    return theme;
}

} // namespace YANP::Themes
