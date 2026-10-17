#include "EnchantedLibraryTheme.h"

namespace YANP::Themes {

Theme createEnchantedLibraryTheme() {
    Theme theme;
    theme.id = QStringLiteral("enchanted-library");
    theme.name = QStringLiteral("The Enchanted Library");
    theme.isSystemDefault = false;

    theme.windowBackground = QColor(0x21, 0x17, 0x13);
    theme.editorBackground = QColor(0xE8, 0xD8, 0xB5);
    theme.textColor = QColor(0x38, 0x29, 0x1F);
    theme.selectionBackground = QColor(0xB9, 0xA0, 0x78);
    theme.selectedTextColor = QColor(0xFF, 0xF5, 0xDC);
    theme.caretColor = QColor(0xA6, 0x6B, 0x2E);
    theme.statusBarBackground = QColor(0x35, 0x25, 0x1D);
    theme.statusBarTextColor = QColor(0xD7, 0xBC, 0x87);
    theme.borderColor = QColor(0x8F, 0x70, 0x45);

    theme.defaultFontFamily = QStringLiteral("Monospace");
    theme.defaultFontSize = 11;

    return theme;
}

} // namespace YANP::Themes
