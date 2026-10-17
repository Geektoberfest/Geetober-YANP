#include "ThemeManager.h"
#include "ThemeRegistry.h"
#include <QApplication>
#include <QPalette>
#include <QStyle>
#include <QStyleFactory>

namespace YANP {

ThemeManager::ThemeManager(QObject* parent)
    : QObject(parent)
{
    loadSavedTheme();
}

Theme ThemeManager::currentTheme() const {
    return m_currentTheme;
}

bool ThemeManager::setTheme(const QString& themeId) {
    auto maybeTheme = ThemeRegistry::instance().getTheme(themeId);
    if (!maybeTheme.has_value()) {
        return false;
    }

    m_currentTheme = *maybeTheme;
    saveCurrentTheme();
    emit themeChanged(m_currentTheme);
    return true;
}

void ThemeManager::loadSavedTheme() {
    QSettings settings;
    QString savedId = settings.value(QStringLiteral("theme/id"), QStringLiteral("system-default")).toString();
    auto theme = ThemeRegistry::instance().getTheme(savedId);
    if (theme.has_value()) {
        m_currentTheme = *theme;
    } else {
        m_currentTheme = ThemeRegistry::instance().defaultTheme();
    }
}

void ThemeManager::saveCurrentTheme() {
    QSettings settings;
    settings.setValue(QStringLiteral("theme/id"), m_currentTheme.id);
}

void ThemeManager::applyTheme(const Theme& theme, QMainWindow* window, QPlainTextEdit* editor, QStatusBar* statusBar) {
    if (theme.isSystemDefault) {
        if (window) {
            window->setStyleSheet(QString());
            window->setPalette(qApp->style()->standardPalette());
        }
        if (editor) {
            editor->setStyleSheet(QString());
            editor->setPalette(qApp->style()->standardPalette());
        }
        if (statusBar) {
            statusBar->setStyleSheet(QString());
            statusBar->setPalette(qApp->style()->standardPalette());
        }
        return;
    }

    // Set editor palette and stylesheet
    if (editor) {
        QPalette editorPalette = editor->palette();
        editorPalette.setColor(QPalette::Base, theme.editorBackground);
        editorPalette.setColor(QPalette::Text, theme.textColor);
        editorPalette.setColor(QPalette::Highlight, theme.selectionBackground);
        editorPalette.setColor(QPalette::HighlightedText, theme.selectedTextColor);
        editor->setPalette(editorPalette);

        QString editorStyle = QString(
            "QPlainTextEdit {"
            "  background-color: %1;"
            "  color: %2;"
            "  selection-background-color: %3;"
            "  selection-color: %4;"
            "  border: none;"
            "}"
        ).arg(theme.editorBackground.name(),
              theme.textColor.name(),
              theme.selectionBackground.name(),
              theme.selectedTextColor.name());
        editor->setStyleSheet(editorStyle);
    }

    // Set status bar styling
    if (statusBar) {
        QPalette sbPalette = statusBar->palette();
        sbPalette.setColor(QPalette::Window, theme.statusBarBackground);
        sbPalette.setColor(QPalette::WindowText, theme.statusBarTextColor);
        statusBar->setPalette(sbPalette);

        QString sbStyle = QString(
            "QStatusBar {"
            "  background-color: %1;"
            "  color: %2;"
            "  border-top: 1px solid %3;"
            "}"
            "QStatusBar QLabel {"
            "  color: %2;"
            "}"
        ).arg(theme.statusBarBackground.name(),
              theme.statusBarTextColor.name(),
              theme.borderColor.name());
        statusBar->setStyleSheet(sbStyle);
    }

    // Set main window & menu bar styling
    if (window) {
        QPalette winPalette = window->palette();
        winPalette.setColor(QPalette::Window, theme.windowBackground);
        winPalette.setColor(QPalette::WindowText, theme.textColor);
        window->setPalette(winPalette);

        QString winStyle = QString(
            "QMainWindow {"
            "  background-color: %1;"
            "}"
            "QMenuBar {"
            "  background-color: %1;"
            "  color: %2;"
            "}"
            "QMenuBar::item:selected {"
            "  background-color: %3;"
            "  color: %4;"
            "}"
            "QMenu {"
            "  background-color: %1;"
            "  color: %2;"
            "  border: 1px solid %5;"
            "}"
            "QMenu::item:selected {"
            "  background-color: %3;"
            "  color: %4;"
            "}"
        ).arg(theme.windowBackground.name(),
              theme.statusBarTextColor.name(),
              theme.selectionBackground.name(),
              theme.selectedTextColor.name(),
              theme.borderColor.name());
        window->setStyleSheet(winStyle);
    }
}

} // namespace YANP
