#include "MainWindow.h"
#include "AboutDialog.h"
#include "themes/ThemeRegistry.h"

#include <QMenuBar>
#include <QMenu>
#include <QStatusBar>
#include <QFileDialog>
#include <QFontDialog>
#include <QMessageBox>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QUrl>
#include <QFontDatabase>

namespace YANP {

namespace {

struct TextStats {
    int lines = 0;
    int words = 0;
    int charsWithSpaces = 0;
    int charsWithoutSpaces = 0;
};

TextStats computeTextStats(const QString& text, int lines) {
    TextStats stats;
    stats.lines = text.isEmpty() ? 0 : lines;
    stats.charsWithSpaces = text.size();

    bool inWord = false;
    for (const QChar c : text) {
        if (c.isSpace()) {
            inWord = false;
        } else {
            stats.charsWithoutSpaces++;
            if (!inWord) {
                inWord = true;
                stats.words++;
            }
        }
    }
    return stats;
}

} // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      m_fileManager(std::make_unique<FileManager>(this)),
      m_themeManager(std::make_unique<ThemeManager>(this))
{
    // Central Editor Widget
    m_editor = new EditorWidget(this);
    setCentralWidget(m_editor);

    // Default monospaced font
    QFont defaultFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    defaultFont.setPointSize(11);
    m_editor->setFont(defaultFont);

    // Initialize Search and Print managers
    m_searchController = std::make_unique<SearchController>(m_editor, this);
    m_printManager = std::make_unique<PrintManager>(this);

    createActions();
    createMenus();
    createStatusBar();

    // Connect editor signals
    connect(m_editor, &QPlainTextEdit::textChanged, this, &MainWindow::onDocumentTextChanged);
    connect(m_editor, &EditorWidget::cursorCoordinatesChanged, this, &MainWindow::onCursorCoordinatesChanged);
    connect(m_editor, &EditorWidget::fileDropped, this, &MainWindow::openFileFromPath);
    connect(m_editor, &QPlainTextEdit::copyAvailable, this, [this](bool) { updateEditActions(); });
    connect(m_editor, &QPlainTextEdit::undoAvailable, this, [this](bool) { updateEditActions(); });

    // Connect file manager signals
    connect(m_fileManager.get(), &FileManager::modifiedChanged, this, &MainWindow::updateWindowTitle);
    connect(m_fileManager.get(), &FileManager::fileChanged, this, &MainWindow::updateWindowTitle);
    connect(m_fileManager.get(), &FileManager::recentFilesChanged, this, &MainWindow::updateRecentFilesMenu);

    updateRecentFilesMenu();
    loadSettings();
    updateWindowTitle();
    updateStatusBar();
    updateEditActions();

    // Apply current theme
    m_themeManager->applyTheme(m_themeManager->currentTheme(), this, m_editor, statusBar());
}

void MainWindow::createActions() {
    // Actions are created and wired in createMenus()
}

void MainWindow::createMenus() {
    // === FILE MENU ===
    QMenu* fileMenu = menuBar()->addMenu(tr("&File"));

    auto* newAction = fileMenu->addAction(tr("&New"), this, &MainWindow::onNew);
    newAction->setShortcut(QKeySequence::New);

    auto* openAction = fileMenu->addAction(tr("&Open..."), this, &MainWindow::onOpen);
    openAction->setShortcut(QKeySequence::Open);

    m_recentFilesMenu = fileMenu->addMenu(tr("Open &Recent"));

    auto* saveAction = fileMenu->addAction(tr("&Save"), this, &MainWindow::onSave);
    saveAction->setShortcut(QKeySequence::Save);

    auto* saveAsAction = fileMenu->addAction(tr("Save &As..."), this, &MainWindow::onSaveAs);
    saveAsAction->setShortcut(QKeySequence::SaveAs);

    fileMenu->addSeparator();

    fileMenu->addAction(tr("Page Set&up..."), this, &MainWindow::onPageSetup);

    auto* printAction = fileMenu->addAction(tr("&Print..."), this, &MainWindow::onPrint);
    printAction->setShortcut(QKeySequence::Print);

    fileMenu->addSeparator();

    auto* exitAction = fileMenu->addAction(tr("E&xit"), this, &QWidget::close);
    exitAction->setShortcut(QKeySequence::Quit);

    // === EDIT MENU ===
    QMenu* editMenu = menuBar()->addMenu(tr("&Edit"));

    m_undoAction = editMenu->addAction(tr("&Undo"), this, &MainWindow::onUndo);
    m_undoAction->setShortcut(QKeySequence::Undo);
    m_undoAction->setEnabled(false);

    editMenu->addSeparator();

    m_cutAction = editMenu->addAction(tr("Cu&t"), this, &MainWindow::onCut);
    m_cutAction->setShortcut(QKeySequence::Cut);
    m_cutAction->setEnabled(false);

    m_copyAction = editMenu->addAction(tr("&Copy"), this, &MainWindow::onCopy);
    m_copyAction->setShortcut(QKeySequence::Copy);
    m_copyAction->setEnabled(false);

    m_pasteAction = editMenu->addAction(tr("&Paste"), this, &MainWindow::onPaste);
    m_pasteAction->setShortcut(QKeySequence::Paste);

    m_deleteAction = editMenu->addAction(tr("De&lete"), this, &MainWindow::onDelete);
    m_deleteAction->setShortcut(QKeySequence::Delete);
    m_deleteAction->setEnabled(false);

    editMenu->addSeparator();

    auto* findAction = editMenu->addAction(tr("&Find..."), this, &MainWindow::onFind);
    findAction->setShortcut(QKeySequence::Find);

    auto* findNextAction = editMenu->addAction(tr("Find &Next"), this, &MainWindow::onFindNext);
    findNextAction->setShortcut(QKeySequence::FindNext);

    m_replaceAction = editMenu->addAction(tr("&Replace..."), this, &MainWindow::onReplace);
#if defined(Q_OS_MACOS)
    m_replaceAction->setShortcut(QKeySequence(tr("Ctrl+Meta+F")));
#else
    m_replaceAction->setShortcut(QKeySequence(tr("Ctrl+H")));
#endif

    auto* goToAction = editMenu->addAction(tr("&Go To..."), this, &MainWindow::onGoTo);
#if defined(Q_OS_MACOS)
    goToAction->setShortcut(QKeySequence(tr("Ctrl+L")));
#else
    goToAction->setShortcut(QKeySequence(tr("Ctrl+G")));
#endif

    editMenu->addSeparator();

    auto* selectAllAction = editMenu->addAction(tr("Select &All"), this, &MainWindow::onSelectAll);
    selectAllAction->setShortcut(QKeySequence::SelectAll);

    m_timeDateAction = editMenu->addAction(tr("Time/&Date"), this, &MainWindow::onTimeDate);
    m_timeDateAction->setShortcut(QKeySequence(Qt::Key_F5));

    editMenu->addSeparator();

    auto* readOnlyAction = editMenu->addAction(tr("&Read-Only"), this, &MainWindow::onToggleReadOnly);
    readOnlyAction->setCheckable(true);

    // === FORMAT MENU ===
    QMenu* formatMenu = menuBar()->addMenu(tr("F&ormat"));

    m_wordWrapAction = formatMenu->addAction(tr("&Word Wrap"), this, &MainWindow::onToggleWordWrap);
    m_wordWrapAction->setCheckable(true);
    m_wordWrapAction->setChecked(m_editor->isWordWrapEnabled());

    formatMenu->addAction(tr("&Font..."), this, &MainWindow::onSelectFont);

    // === VIEW MENU ===
    QMenu* viewMenu = menuBar()->addMenu(tr("&View"));

    m_statusBarAction = viewMenu->addAction(tr("&Status Bar"), this, &MainWindow::onToggleStatusBar);
    m_statusBarAction->setCheckable(true);
    m_statusBarAction->setChecked(true);

    viewMenu->addAction(tr("Document &Statistics..."), this, &MainWindow::onDocumentStatistics);

    QMenu* themeMenu = viewMenu->addMenu(tr("&Theme"));
    populateThemesMenu(themeMenu);

    // === HELP MENU ===
    QMenu* helpMenu = menuBar()->addMenu(tr("&Help"));

    auto* helpAction = helpMenu->addAction(tr("View &Help"), this, &MainWindow::onViewHelp);
    helpAction->setShortcut(QKeySequence::HelpContents);

    helpMenu->addSeparator();
    helpMenu->addAction(tr("&About YANP"), this, &MainWindow::onAbout);
}

void MainWindow::populateThemesMenu(QMenu* themeMenu) {
    m_themeActionGroup = new QActionGroup(this);
    m_themeActionGroup->setExclusive(true);

    QString currentId = m_themeManager->currentTheme().id;
    const auto themes = ThemeRegistry::instance().registeredThemes();

    for (const auto& theme : themes) {
        auto* action = themeMenu->addAction(theme.name);
        action->setCheckable(true);
        action->setData(theme.id);
        if (theme.id == currentId) {
            action->setChecked(true);
        }
        m_themeActionGroup->addAction(action);
    }

    connect(m_themeActionGroup, &QActionGroup::triggered, this, &MainWindow::onThemeSelected);
}

void MainWindow::createStatusBar() {
    m_cursorPosLabel = new QLabel(this);
    m_cursorPosLabel->setMinimumWidth(140);
    statusBar()->addWidget(m_cursorPosLabel);

    m_encodingLabel = new QLabel(this);
    m_encodingLabel->setMinimumWidth(100);
    statusBar()->addPermanentWidget(m_encodingLabel);

    m_lineEndingLabel = new QLabel(this);
    m_lineEndingLabel->setMinimumWidth(120);
    statusBar()->addPermanentWidget(m_lineEndingLabel);

    m_readOnlyLabel = new QLabel(tr("[Read-Only]"), this);
    m_readOnlyLabel->setVisible(false);
    statusBar()->addPermanentWidget(m_readOnlyLabel);
}

void MainWindow::onNew() {
    if (!maybeSave()) {
        return;
    }
    m_editor->clear();
    m_fileManager->reset();
    updateWindowTitle();
    updateStatusBar();
}

void MainWindow::onOpen() {
    if (!maybeSave()) {
        return;
    }

    QString fileName = QFileDialog::getOpenFileName(
        this,
        tr("Open Document"),
        m_fileManager->filePath().isEmpty() ? QString() : m_fileManager->filePath(),
        tr("Text Documents (*.txt);;All Files (*.*)")
    );

    if (fileName.isEmpty()) {
        return;
    }

    loadFile(fileName);
}

void MainWindow::openRecentFile(const QString& path) {
    if (!maybeSave()) {
        return;
    }

    if (!QFileInfo::exists(path)) {
        QMessageBox::warning(
            this,
            tr("YANP"),
            tr("File no longer exists: %1").arg(path)
        );
        m_fileManager->removeRecentFile(path);
        return;
    }

    loadFile(path);
}

void MainWindow::updateRecentFilesMenu() {
    if (!m_recentFilesMenu) {
        return;
    }

    m_recentFilesMenu->clear();

    const QStringList files = m_fileManager->recentFiles();
    if (files.isEmpty()) {
        auto* noRecentAction = m_recentFilesMenu->addAction(tr("No Recent Files"));
        noRecentAction->setEnabled(false);
        m_recentFilesMenu->setEnabled(false);
        return;
    }

    m_recentFilesMenu->setEnabled(true);
    for (int i = 0; i < files.size(); ++i) {
        const QString& filePath = files.at(i);
        QFileInfo fi(filePath);
        QString displayName = fi.fileName();
        if (displayName.isEmpty()) {
            displayName = filePath;
        }

        auto* action = m_recentFilesMenu->addAction(displayName, this, [this, filePath]() {
            openRecentFile(filePath);
        });
        action->setData(filePath);
        action->setToolTip(filePath);
        action->setStatusTip(filePath);
    }

    m_recentFilesMenu->addSeparator();
    m_recentFilesMenu->addAction(tr("Clear Recent Files"), this, [this]() {
        m_fileManager->clearRecentFiles();
    });
}

bool MainWindow::openFileFromPath(const QString& filePath) {
    if (m_fileManager->isModified() && !maybeSave()) {
        return false;
    }

    return loadFile(filePath);
}

bool MainWindow::loadFile(const QString& filePath) {
    FileResult res = m_fileManager->openFile(filePath);
    if (!res.success) {
        QMessageBox::critical(this, tr("Error Opening File"), res.errorMessage);
        return false;
    }

    m_editor->setPlainText(res.content);
    m_fileManager->setModified(false);
    updateWindowTitle();
    updateStatusBar();
    return true;
}

bool MainWindow::onSave() {
    if (m_fileManager->filePath().isEmpty()) {
        return onSaveAs();
    }

    FileResult res = m_fileManager->saveFile(m_fileManager->filePath(), m_editor->toPlainText());
    if (!res.success) {
        QMessageBox::critical(this, tr("Error Saving File"), res.errorMessage);
        return false;
    }

    updateWindowTitle();
    return true;
}

bool MainWindow::onSaveAs() {
    QString initialPath = m_fileManager->filePath();
    if (initialPath.isEmpty()) {
        initialPath = QStringLiteral("Untitled.txt");
    }

    QString fileName = QFileDialog::getSaveFileName(
        this,
        tr("Save As"),
        initialPath,
        tr("Text Documents (*.txt);;All Files (*.*)")
    );

    if (fileName.isEmpty()) {
        return false;
    }

    FileResult res = m_fileManager->saveFile(fileName, m_editor->toPlainText());
    if (!res.success) {
        QMessageBox::critical(this, tr("Error Saving File"), res.errorMessage);
        return false;
    }

    updateWindowTitle();
    updateStatusBar();
    return true;
}

void MainWindow::onPageSetup() {
    m_printManager->pageSetup(this);
}

void MainWindow::onPrint() {
    m_printManager->printDocument(this, m_editor->document(), m_fileManager->fileName());
}

void MainWindow::onUndo() {
    m_editor->undo();
}

void MainWindow::onCut() {
    m_editor->cut();
}

void MainWindow::onCopy() {
    m_editor->copy();
}

void MainWindow::onPaste() {
    m_editor->paste();
}

void MainWindow::onDelete() {
    m_editor->deleteSelection();
}

void MainWindow::onFind() {
    m_searchController->showFindDialog();
}

void MainWindow::onFindNext() {
    m_searchController->findNext();
}

void MainWindow::onReplace() {
    m_searchController->showReplaceDialog();
}

void MainWindow::onGoTo() {
    m_searchController->showGoToDialog();
}

void MainWindow::onSelectAll() {
    m_editor->selectAll();
}

void MainWindow::onTimeDate() {
    m_editor->insertDateTime();
}

void MainWindow::onToggleReadOnly(bool checked) {
    m_editor->setReadOnly(checked);
    m_readOnlyLabel->setVisible(checked);
    updateEditActions();
}

void MainWindow::updateEditActions() {
    // Read-only blocks every action that changes the document; copy and selection stay usable
    const bool readOnly = m_editor->isReadOnly();
    if (m_undoAction) {
        m_undoAction->setEnabled(!readOnly && m_editor->document()->isUndoAvailable());
    }
    bool hasSelection = m_editor->textCursor().hasSelection();
    if (m_cutAction) {
        m_cutAction->setEnabled(!readOnly && hasSelection);
    }
    if (m_copyAction) {
        m_copyAction->setEnabled(hasSelection);
    }
    if (m_pasteAction) {
        m_pasteAction->setEnabled(!readOnly);
    }
    if (m_deleteAction) {
        m_deleteAction->setEnabled(!readOnly && hasSelection);
    }
    if (m_replaceAction) {
        m_replaceAction->setEnabled(!readOnly);
    }
    if (m_timeDateAction) {
        m_timeDateAction->setEnabled(!readOnly);
    }
}

void MainWindow::onToggleWordWrap(bool checked) {
    m_editor->setWordWrapEnabled(checked);
    saveSettings();
}

void MainWindow::onSelectFont() {
    bool ok = false;
    QFont font = QFontDialog::getFont(&ok, m_editor->font(), this, tr("Select Font"));
    if (ok) {
        m_editor->setFont(font);
        saveSettings();
    }
}

void MainWindow::onToggleStatusBar(bool checked) {
    statusBar()->setVisible(checked);
    saveSettings();
}

void MainWindow::onThemeSelected(QAction* action) {
    if (!action) return;
    QString themeId = action->data().toString();
    if (m_themeManager->setTheme(themeId)) {
        m_themeManager->applyTheme(m_themeManager->currentTheme(), this, m_editor, statusBar());
    }
}

void MainWindow::onViewHelp() {
    QDesktopServices::openUrl(QUrl(QStringLiteral("https://github.com/Geektoberfest/Geetober-YANP/")));
}

void MainWindow::onAbout() {
    AboutDialog dlg(this);
    dlg.exec();
}

void MainWindow::onDocumentStatistics() {
    const QString fullText = m_editor->toPlainText();
    const TextStats doc = computeTextStats(fullText, m_editor->document()->blockCount());

    QString message = tr("Document\nLines: %1\nWords: %2\nCharacters (with spaces): %3\nCharacters (without spaces): %4")
        .arg(doc.lines)
        .arg(doc.words)
        .arg(doc.charsWithSpaces)
        .arg(doc.charsWithoutSpaces);

    const QTextCursor cursor = m_editor->textCursor();
    if (cursor.hasSelection()) {
        const QString selectedText = cursor.selectedText();
        // Selected text uses U+2029 between paragraphs, so count those to get selection lines
        const int selectionLines = selectedText.count(QChar::ParagraphSeparator) + 1;
        const TextStats sel = computeTextStats(selectedText, selectionLines);

        message += tr("\n\nSelection\nLines: %1\nWords: %2\nCharacters (with spaces): %3\nCharacters (without spaces): %4")
            .arg(sel.lines)
            .arg(sel.words)
            .arg(sel.charsWithSpaces)
            .arg(sel.charsWithoutSpaces);
    }

    QMessageBox::information(this, tr("Document Statistics"), message);
}

void MainWindow::onDocumentTextChanged() {
    m_fileManager->setModified(true);
    updateEditActions();
}

void MainWindow::onCursorCoordinatesChanged(int line, int column) {
    m_cursorPosLabel->setText(tr("Ln %1, Col %2").arg(line).arg(column));
    updateEditActions();
}

void MainWindow::updateStatusBar() {
    m_cursorPosLabel->setText(tr("Ln %1, Col %2").arg(m_editor->currentLine()).arg(m_editor->currentColumn()));
    m_encodingLabel->setText(m_fileManager->encodingName());
    m_lineEndingLabel->setText(m_fileManager->lineEndingName());
}

void MainWindow::updateWindowTitle() {
    setWindowTitle(m_fileManager->windowTitle());
}

bool MainWindow::maybeSave() {
    if (!m_fileManager->isModified()) {
        return true;
    }

    const QMessageBox::StandardButton ret = QMessageBox::warning(
        this,
        tr("YANP"),
        tr("Do you want to save changes to %1?").arg(m_fileManager->fileName()),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
        QMessageBox::Save
    );

    switch (ret) {
    case QMessageBox::Save:
        return onSave();
    case QMessageBox::Cancel:
        return false;
    case QMessageBox::Discard:
    default:
        return true;
    }
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (maybeSave()) {
        saveSettings();
        event->accept();
    } else {
        event->ignore();
    }
}

void MainWindow::loadSettings() {
    QSettings settings;
    resize(settings.value(QStringLiteral("window/size"), QSize(800, 600)).toSize());
    move(settings.value(QStringLiteral("window/pos"), QPoint(100, 100)).toPoint());

    bool wrap = settings.value(QStringLiteral("editor/wordWrap"), true).toBool();
    m_editor->setWordWrapEnabled(wrap);
    if (m_wordWrapAction) {
        m_wordWrapAction->setChecked(wrap);
    }

    bool showStatusBar = settings.value(QStringLiteral("view/statusBar"), true).toBool();
    statusBar()->setVisible(showStatusBar);
    if (m_statusBarAction) {
        m_statusBarAction->setChecked(showStatusBar);
    }

    QString fontFamily = settings.value(QStringLiteral("font/family")).toString();
    int fontSize = settings.value(QStringLiteral("font/size"), 0).toInt();
    if (!fontFamily.isEmpty() && fontSize > 0) {
        QFont f(fontFamily, fontSize);
        m_editor->setFont(f);
    }
}

void MainWindow::saveSettings() {
    QSettings settings;
    settings.setValue(QStringLiteral("window/size"), size());
    settings.setValue(QStringLiteral("window/pos"), pos());
    settings.setValue(QStringLiteral("editor/wordWrap"), m_editor->isWordWrapEnabled());
    settings.setValue(QStringLiteral("view/statusBar"), statusBar()->isVisible());
    settings.setValue(QStringLiteral("font/family"), m_editor->font().family());
    settings.setValue(QStringLiteral("font/size"), m_editor->font().pointSize());
}

} // namespace YANP
