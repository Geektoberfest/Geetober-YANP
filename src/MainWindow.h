#pragma once

#include <QMainWindow>
#include <QLabel>
#include <QActionGroup>
#include <memory>

#include "EditorWidget.h"
#include "FileManager.h"
#include "SearchController.h"
#include "PrintManager.h"
#include "themes/ThemeManager.h"

namespace YANP {

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override = default;

    bool openFileFromPath(const QString& filePath);

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    // File actions
    void onNew();
    void onOpen();
    void openRecentFile(const QString& path);
    void updateRecentFilesMenu();
    bool onSave();
    bool onSaveAs();
    void onPageSetup();
    void onPrint();

    // Edit actions
    void onUndo();
    void onCut();
    void onCopy();
    void onPaste();
    void onDelete();
    void onFind();
    void onFindNext();
    void onReplace();
    void onGoTo();
    void onSelectAll();
    void onTimeDate();
    void onToggleReadOnly(bool checked);
    void updateEditActions();

    // Format actions
    void onToggleWordWrap(bool checked);
    void onSelectFont();

    // View actions
    void onToggleStatusBar(bool checked);
    void onDocumentStatistics();
    void onThemeSelected(QAction* action);

    // Help actions
    void onViewHelp();
    void onAbout();

    // Internal coordinators
    void onDocumentTextChanged();
    void onCursorCoordinatesChanged(int line, int column);
    void updateStatusBar();
    void updateWindowTitle();
    void onFileChangedExternally(const QString& filePath);
    void reloadCurrentFile();

private:
    void createActions();
    void createMenus();
    void createStatusBar();
    bool maybeSave();
    bool loadFile(const QString& filePath);
    void loadSettings();
    void saveSettings();
    void populateThemesMenu(QMenu* themeMenu);

    EditorWidget* m_editor = nullptr;
    std::unique_ptr<FileManager> m_fileManager;
    bool m_isReloadDialogOpen = false;
    std::unique_ptr<SearchController> m_searchController;
    std::unique_ptr<PrintManager> m_printManager;
    std::unique_ptr<ThemeManager> m_themeManager;

    // Status bar widgets
    QLabel* m_cursorPosLabel = nullptr;
    QLabel* m_encodingLabel = nullptr;
    QLabel* m_lineEndingLabel = nullptr;
    QLabel* m_readOnlyLabel = nullptr;

    // Actions that need dynamic state updates
    QAction* m_undoAction = nullptr;
    QAction* m_cutAction = nullptr;
    QAction* m_copyAction = nullptr;
    QAction* m_pasteAction = nullptr;
    QAction* m_deleteAction = nullptr;
    QAction* m_replaceAction = nullptr;
    QAction* m_timeDateAction = nullptr;
    QAction* m_wordWrapAction = nullptr;
    QAction* m_statusBarAction = nullptr;

    QMenu* m_recentFilesMenu = nullptr;
    QActionGroup* m_themeActionGroup = nullptr;
};

} // namespace YANP
