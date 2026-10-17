#include <QTest>
#include "themes/ThemeRegistry.h"
#include "themes/ThemeManager.h"

using namespace YANP;

class TestThemes : public QObject {
    Q_OBJECT

private slots:
    void init();
    void testBuiltInThemesRegistered();
    void testEnchantedLibraryPalette();
    void testDreamscapeIslandsPalette();
    void testMidnightObservatoryPalette();
    void testThemeValidity();
    void testDuplicateRegistrationRejected();
    void testInvalidThemeRejected();
    void testThemeLookupAndFallback();
    void testThemeManagerSwitching();
};

void TestThemes::init() {
    ThemeRegistry::instance().resetToDefaults();
}

void TestThemes::testBuiltInThemesRegistered() {
    auto themes = ThemeRegistry::instance().registeredThemes();
    QVERIFY(themes.size() >= 3);

    QVERIFY(ThemeRegistry::instance().hasTheme(QStringLiteral("system-default")));
    QVERIFY(ThemeRegistry::instance().hasTheme(QStringLiteral("classic-light")));
    QVERIFY(ThemeRegistry::instance().hasTheme(QStringLiteral("classic-dark")));
    QVERIFY(ThemeRegistry::instance().hasTheme(QStringLiteral("retro-green")));
    QVERIFY(ThemeRegistry::instance().hasTheme(QStringLiteral("amber-crt")));
    QVERIFY(ThemeRegistry::instance().hasTheme(QStringLiteral("solarized-dark")));
    QVERIFY(ThemeRegistry::instance().hasTheme(QStringLiteral("enchanted-library")));
    QVERIFY(ThemeRegistry::instance().hasTheme(QStringLiteral("dreamscape-islands")));
    QVERIFY(ThemeRegistry::instance().hasTheme(QStringLiteral("midnight-observatory")));
}

void TestThemes::testMidnightObservatoryPalette() {
    auto theme = ThemeRegistry::instance().getTheme(QStringLiteral("midnight-observatory"));
    QVERIFY(theme.has_value());
    QCOMPARE(theme->name, QStringLiteral("Midnight Observatory"));
    QCOMPARE(theme->editorBackground, QColor(0x0B, 0x10, 0x20));
    QCOMPARE(theme->textColor, QColor(0xDC, 0xE7, 0xFF));
    QCOMPARE(theme->windowBackground, QColor(0x08, 0x0B, 0x16));
    QCOMPARE(theme->selectionBackground, QColor(0x29, 0x3B, 0x62));
    QCOMPARE(theme->selectedTextColor, QColor(0xFF, 0xFF, 0xFF));
    QCOMPARE(theme->caretColor, QColor(0x7D, 0xE2, 0xFF));
    QCOMPARE(theme->statusBarBackground, QColor(0x15, 0x1B, 0x30));
    QCOMPARE(theme->statusBarTextColor, QColor(0xA9, 0xB9, 0xE8));
    QCOMPARE(theme->borderColor, QColor(0x1F, 0x29, 0x42));
}

void TestThemes::testDreamscapeIslandsPalette() {
    auto theme = ThemeRegistry::instance().getTheme(QStringLiteral("dreamscape-islands"));
    QVERIFY(theme.has_value());
    QCOMPARE(theme->name, QStringLiteral("Dreamscape: Floating Islands"));
    QCOMPARE(theme->editorBackground, QColor(0x17, 0x18, 0x2B));
    QCOMPARE(theme->textColor, QColor(0xE9, 0xE5, 0xFF));
    QCOMPARE(theme->windowBackground, QColor(0x10, 0x13, 0x23));
    QCOMPARE(theme->selectionBackground, QColor(0x51, 0x45, 0x6F));
    QCOMPARE(theme->selectedTextColor, QColor(0xFF, 0xFF, 0xFF));
    QCOMPARE(theme->caretColor, QColor(0xFF, 0xCA, 0x9B));
    QCOMPARE(theme->statusBarBackground, QColor(0x25, 0x25, 0x40));
    QCOMPARE(theme->statusBarTextColor, QColor(0xC4, 0xBD, 0xE8));
    QCOMPARE(theme->borderColor, QColor(0x38, 0x38, 0x5C));
}

void TestThemes::testEnchantedLibraryPalette() {
    auto theme = ThemeRegistry::instance().getTheme(QStringLiteral("enchanted-library"));
    QVERIFY(theme.has_value());
    QCOMPARE(theme->name, QStringLiteral("The Enchanted Library"));
    QCOMPARE(theme->editorBackground, QColor(0xE8, 0xD8, 0xB5));
    QCOMPARE(theme->textColor, QColor(0x38, 0x29, 0x1F));
    QCOMPARE(theme->windowBackground, QColor(0x21, 0x17, 0x13));
    QCOMPARE(theme->selectionBackground, QColor(0xB9, 0xA0, 0x78));
    QCOMPARE(theme->selectedTextColor, QColor(0xFF, 0xF5, 0xDC));
    QCOMPARE(theme->caretColor, QColor(0xA6, 0x6B, 0x2E));
    QCOMPARE(theme->statusBarBackground, QColor(0x35, 0x25, 0x1D));
    QCOMPARE(theme->statusBarTextColor, QColor(0xD7, 0xBC, 0x87));
    QCOMPARE(theme->borderColor, QColor(0x8F, 0x70, 0x45));
}

void TestThemes::testThemeValidity() {
    auto themes = ThemeRegistry::instance().registeredThemes();
    for (const auto& theme : themes) {
        QVERIFY2(theme.isValid(), qPrintable(QString("Theme '%1' failed validation").arg(theme.id)));
        QVERIFY(!theme.id.isEmpty());
        QVERIFY(!theme.name.isEmpty());

        if (!theme.isSystemDefault) {
            QVERIFY(theme.windowBackground.isValid());
            QVERIFY(theme.editorBackground.isValid());
            QVERIFY(theme.textColor.isValid());
            QVERIFY(theme.selectionBackground.isValid());
            QVERIFY(theme.selectedTextColor.isValid());
            QVERIFY(theme.statusBarBackground.isValid());
            QVERIFY(theme.statusBarTextColor.isValid());
            // Contrast sanity check: text color must not equal editor background
            QVERIFY(theme.textColor != theme.editorBackground);
            // Selection text must not equal selection background
            QVERIFY(theme.selectedTextColor != theme.selectionBackground);
        }
    }
}

void TestThemes::testDuplicateRegistrationRejected() {
    Theme dup;
    dup.id = QStringLiteral("classic-light");
    dup.name = QStringLiteral("Duplicate Classic Light");
    dup.isSystemDefault = false;
    dup.windowBackground = QColor(255, 255, 255);
    dup.editorBackground = QColor(255, 255, 255);
    dup.textColor = QColor(0, 0, 0);
    dup.selectionBackground = QColor(0, 0, 255);
    dup.selectedTextColor = QColor(255, 255, 255);
    dup.caretColor = QColor(0, 0, 0);
    dup.statusBarBackground = QColor(200, 200, 200);
    dup.statusBarTextColor = QColor(0, 0, 0);

    bool result = ThemeRegistry::instance().registerTheme(dup);
    QVERIFY(!result);
}

void TestThemes::testInvalidThemeRejected() {
    Theme invalid;
    invalid.id = ""; // Empty ID
    invalid.name = "Invalid";

    bool result = ThemeRegistry::instance().registerTheme(invalid);
    QVERIFY(!result);
}

void TestThemes::testThemeLookupAndFallback() {
    auto theme = ThemeRegistry::instance().getTheme(QStringLiteral("classic-dark"));
    QVERIFY(theme.has_value());
    QCOMPARE(theme->id, QStringLiteral("classic-dark"));

    auto nonExistent = ThemeRegistry::instance().getTheme(QStringLiteral("non-existent-theme"));
    QVERIFY(!nonExistent.has_value());

    Theme defaultTheme = ThemeRegistry::instance().defaultTheme();
    QCOMPARE(defaultTheme.id, QStringLiteral("system-default"));
}

void TestThemes::testThemeManagerSwitching() {
    ThemeManager tm;
    QVERIFY(tm.setTheme(QStringLiteral("classic-dark")));
    QCOMPARE(tm.currentTheme().id, QStringLiteral("classic-dark"));

    // Setting invalid theme ID should return false and not change current theme
    QVERIFY(!tm.setTheme(QStringLiteral("unknown-theme")));
    QCOMPARE(tm.currentTheme().id, QStringLiteral("classic-dark"));

    // Switch back to system default
    QVERIFY(tm.setTheme(QStringLiteral("system-default")));
    QCOMPARE(tm.currentTheme().id, QStringLiteral("system-default"));
}

QTEST_MAIN(TestThemes)
#include "test_themes.moc"
