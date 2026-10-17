#include "SearchController.h"
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QTextBlock>
#include <QIntValidator>

namespace YANP {

// --- FindDialog ---

FindDialog::FindDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Find"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    auto* layout = new QGridLayout(this);

    auto* label = new QLabel(tr("Fi&nd what:"), this);
    m_findEdit = new QLineEdit(this);
    label->setBuddy(m_findEdit);

    m_caseCheckBox = new QCheckBox(tr("Match &case"), this);
    m_wrapCheckBox = new QCheckBox(tr("Wrap aroun&d"), this);
    m_wrapCheckBox->setChecked(true);

    auto* dirGroup = new QGroupBox(tr("Direction"), this);
    auto* dirLayout = new QHBoxLayout(dirGroup);
    m_upRadio = new QRadioButton(tr("&Up"), dirGroup);
    m_downRadio = new QRadioButton(tr("&Down"), dirGroup);
    m_downRadio->setChecked(true);
    dirLayout->addWidget(m_upRadio);
    dirLayout->addWidget(m_downRadio);

    m_findButton = new QPushButton(tr("&Find Next"), this);
    m_findButton->setDefault(true);
    m_findButton->setEnabled(false);

    m_cancelButton = new QPushButton(tr("Cancel"), this);

    layout->addWidget(label, 0, 0);
    layout->addWidget(m_findEdit, 0, 1, 1, 2);
    layout->addWidget(m_findButton, 0, 3);

    layout->addWidget(m_caseCheckBox, 1, 0, 1, 2);
    layout->addWidget(dirGroup, 1, 2);
    layout->addWidget(m_cancelButton, 1, 3);

    layout->addWidget(m_wrapCheckBox, 2, 0, 1, 2);

    connect(m_findEdit, &QLineEdit::textChanged, this, &FindDialog::updateButtons);
    connect(m_findButton, &QPushButton::clicked, this, &FindDialog::onFindClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::hide);
}

QString FindDialog::searchText() const {
    return m_findEdit->text();
}

void FindDialog::setSearchText(const QString& text) {
    m_findEdit->setText(text);
    m_findEdit->selectAll();
    updateButtons();
}

bool FindDialog::matchCase() const {
    return m_caseCheckBox->isChecked();
}

bool FindDialog::searchBackward() const {
    return m_upRadio->isChecked();
}

bool FindDialog::wrapAround() const {
    return m_wrapCheckBox->isChecked();
}

void FindDialog::onFindClicked() {
    emit findRequested(searchText(), matchCase(), searchBackward(), wrapAround());
}

void FindDialog::updateButtons() {
    m_findButton->setEnabled(!m_findEdit->text().isEmpty());
}


// --- ReplaceDialog ---

ReplaceDialog::ReplaceDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Replace"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    auto* layout = new QGridLayout(this);

    auto* findLabel = new QLabel(tr("Fi&nd what:"), this);
    m_findEdit = new QLineEdit(this);
    findLabel->setBuddy(m_findEdit);

    auto* replaceLabel = new QLabel(tr("Re&place with:"), this);
    m_replaceEdit = new QLineEdit(this);
    replaceLabel->setBuddy(m_replaceEdit);

    m_caseCheckBox = new QCheckBox(tr("Match &case"), this);
    m_wrapCheckBox = new QCheckBox(tr("Wrap aroun&d"), this);
    m_wrapCheckBox->setChecked(true);

    m_findButton = new QPushButton(tr("&Find Next"), this);
    m_findButton->setDefault(true);
    m_findButton->setEnabled(false);

    m_replaceButton = new QPushButton(tr("&Replace"), this);
    m_replaceButton->setEnabled(false);

    m_replaceAllButton = new QPushButton(tr("Replace &All"), this);
    m_replaceAllButton->setEnabled(false);

    m_cancelButton = new QPushButton(tr("Cancel"), this);

    layout->addWidget(findLabel, 0, 0);
    layout->addWidget(m_findEdit, 0, 1);
    layout->addWidget(m_findButton, 0, 2);

    layout->addWidget(replaceLabel, 1, 0);
    layout->addWidget(m_replaceEdit, 1, 1);
    layout->addWidget(m_replaceButton, 1, 2);

    layout->addWidget(m_caseCheckBox, 2, 0, 1, 2);
    layout->addWidget(m_replaceAllButton, 2, 2);

    layout->addWidget(m_wrapCheckBox, 3, 0, 1, 2);
    layout->addWidget(m_cancelButton, 3, 2);

    connect(m_findEdit, &QLineEdit::textChanged, this, &ReplaceDialog::updateButtons);
    connect(m_findButton, &QPushButton::clicked, this, &ReplaceDialog::onFindClicked);
    connect(m_replaceButton, &QPushButton::clicked, this, &ReplaceDialog::onReplaceClicked);
    connect(m_replaceAllButton, &QPushButton::clicked, this, &ReplaceDialog::onReplaceAllClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::hide);
}

QString ReplaceDialog::searchText() const {
    return m_findEdit->text();
}

void ReplaceDialog::setSearchText(const QString& text) {
    m_findEdit->setText(text);
    m_findEdit->selectAll();
    updateButtons();
}

QString ReplaceDialog::replaceText() const {
    return m_replaceEdit->text();
}

bool ReplaceDialog::matchCase() const {
    return m_caseCheckBox->isChecked();
}

bool ReplaceDialog::wrapAround() const {
    return m_wrapCheckBox->isChecked();
}

void ReplaceDialog::onFindClicked() {
    emit findRequested(searchText(), matchCase(), false, wrapAround());
}

void ReplaceDialog::onReplaceClicked() {
    emit replaceRequested(searchText(), replaceText(), matchCase());
}

void ReplaceDialog::onReplaceAllClicked() {
    emit replaceAllRequested(searchText(), replaceText(), matchCase());
}

void ReplaceDialog::updateButtons() {
    bool hasText = !m_findEdit->text().isEmpty();
    m_findButton->setEnabled(hasText);
    m_replaceButton->setEnabled(hasText);
    m_replaceAllButton->setEnabled(hasText);
}


// --- GoToDialog ---

GoToDialog::GoToDialog(int maxLine, int currentLine, QWidget* parent)
    : QDialog(parent), m_maxLine(qMax(1, maxLine))
{
    setWindowTitle(tr("Go To Line"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    auto* layout = new QVBoxLayout(this);

    auto* label = new QLabel(tr("&Line number (1 - %1):").arg(m_maxLine), this);
    m_lineInput = new QLineEdit(QString::number(currentLine), this);
    m_lineInput->setValidator(new QIntValidator(1, m_maxLine, this));
    label->setBuddy(m_lineInput);

    m_errorLabel = new QLabel(this);
    m_errorLabel->setStyleSheet(QStringLiteral("color: red;"));
    m_errorLabel->setVisible(false);

    auto* buttonBox = new QHBoxLayout();
    auto* goButton = new QPushButton(tr("Go To"), this);
    goButton->setDefault(true);
    auto* cancelButton = new QPushButton(tr("Cancel"), this);
    buttonBox->addStretch();
    buttonBox->addWidget(goButton);
    buttonBox->addWidget(cancelButton);

    layout->addWidget(label);
    layout->addWidget(m_lineInput);
    layout->addWidget(m_errorLabel);
    layout->addLayout(buttonBox);

    connect(goButton, &QPushButton::clicked, this, &GoToDialog::onGoToClicked);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    m_lineInput->selectAll();
    m_lineInput->setFocus();
}

int GoToDialog::targetLine() const {
    return m_targetLine;
}

void GoToDialog::onGoToClicked() {
    bool ok = false;
    int line = m_lineInput->text().toInt(&ok);
    if (!ok || line < 1 || line > m_maxLine) {
        m_errorLabel->setText(tr("The line number must be between 1 and %1.").arg(m_maxLine));
        m_errorLabel->setVisible(true);
        m_lineInput->selectAll();
        m_lineInput->setFocus();
        return;
    }
    m_targetLine = line;
    accept();
}


// --- SearchController ---

SearchController::SearchController(EditorWidget* editor, QWidget* parentWindow)
    : QObject(parentWindow), m_editor(editor), m_parentWindow(parentWindow)
{
    m_findDialog = new FindDialog(parentWindow);
    m_replaceDialog = new ReplaceDialog(parentWindow);

    connect(m_findDialog, &FindDialog::findRequested, this,
        [this](const QString& text, bool matchCase, bool backward, bool wrap) {
            m_lastSearchText = text;
            m_lastMatchCase = matchCase;
            m_lastSearchBackward = backward;
            m_lastWrapAround = wrap;
            bool found = find(text, matchCase, backward, wrap);
            if (!found && m_parentWindow) {
                QMessageBox::information(m_findDialog, tr("YANP"),
                    tr("Cannot find \"%1\"").arg(text));
            }
        });

    connect(m_replaceDialog, &ReplaceDialog::findRequested, this,
        [this](const QString& text, bool matchCase, bool backward, bool wrap) {
            m_lastSearchText = text;
            m_lastMatchCase = matchCase;
            m_lastSearchBackward = backward;
            m_lastWrapAround = wrap;
            bool found = find(text, matchCase, backward, wrap);
            if (!found && m_parentWindow) {
                QMessageBox::information(m_replaceDialog, tr("YANP"),
                    tr("Cannot find \"%1\"").arg(text));
            }
        });

    connect(m_replaceDialog, &ReplaceDialog::replaceRequested, this,
        [this](const QString& findText, const QString& replaceText, bool matchCase) {
            m_lastSearchText = findText;
            m_lastMatchCase = matchCase;
            replace(findText, replaceText, matchCase);
        });

    connect(m_replaceDialog, &ReplaceDialog::replaceAllRequested, this,
        [this](const QString& findText, const QString& replaceText, bool matchCase) {
            m_lastSearchText = findText;
            m_lastMatchCase = matchCase;
            int count = replaceAll(findText, replaceText, matchCase);
            if (m_parentWindow) {
                QMessageBox::information(m_replaceDialog, tr("YANP"),
                    tr("Replaced %1 occurrence(s).").arg(count));
            }
        });
}

void SearchController::showFindDialog() {
    if (!m_editor) return;

    QString selected = m_editor->textCursor().selectedText();
    if (!selected.isEmpty() && !selected.contains(QChar::ParagraphSeparator)) {
        m_findDialog->setSearchText(selected);
    } else if (!m_lastSearchText.isEmpty()) {
        m_findDialog->setSearchText(m_lastSearchText);
    }
    m_findDialog->show();
    m_findDialog->activateWindow();
    m_findDialog->raise();
}

void SearchController::showReplaceDialog() {
    if (!m_editor) return;

    QString selected = m_editor->textCursor().selectedText();
    if (!selected.isEmpty() && !selected.contains(QChar::ParagraphSeparator)) {
        m_replaceDialog->setSearchText(selected);
    } else if (!m_lastSearchText.isEmpty()) {
        m_replaceDialog->setSearchText(m_lastSearchText);
    }
    m_replaceDialog->show();
    m_replaceDialog->activateWindow();
    m_replaceDialog->raise();
}

void SearchController::showGoToDialog() {
    if (!m_editor) return;

    int totalLines = m_editor->document()->blockCount();
    int currentLine = m_editor->currentLine();

    GoToDialog dlg(totalLines, currentLine, m_parentWindow);
    if (dlg.exec() == QDialog::Accepted) {
        goToLine(dlg.targetLine());
    }
}

void SearchController::findNext() {
    if (m_lastSearchText.isEmpty()) {
        showFindDialog();
        return;
    }
    bool found = find(m_lastSearchText, m_lastMatchCase, m_lastSearchBackward, m_lastWrapAround);
    if (!found && m_parentWindow) {
        QMessageBox::information(m_parentWindow, tr("YANP"),
            tr("Cannot find \"%1\"").arg(m_lastSearchText));
    }
}

bool SearchController::find(const QString& query, bool matchCase, bool backward, bool wrap) {
    if (!m_editor || query.isEmpty()) {
        return false;
    }

    QTextDocument::FindFlags flags;
    if (matchCase) {
        flags |= QTextDocument::FindCaseSensitively;
    }
    if (backward) {
        flags |= QTextDocument::FindBackward;
    }

    // Try finding from current position
    bool found = m_editor->find(query, flags);
    if (found) {
        return true;
    }

    if (wrap) {
        // Move to top or bottom and find again
        QTextCursor cursor = m_editor->textCursor();
        if (backward) {
            cursor.movePosition(QTextCursor::End);
        } else {
            cursor.movePosition(QTextCursor::Start);
        }
        m_editor->setTextCursor(cursor);

        found = m_editor->find(query, flags);
        if (found) {
            return true;
        }
    }

    return false;
}

bool SearchController::replace(const QString& query, const QString& replacement, bool matchCase) {
    if (!m_editor || query.isEmpty() || m_editor->isReadOnly()) {
        return false;
    }

    QTextCursor cursor = m_editor->textCursor();
    Qt::CaseSensitivity cs = matchCase ? Qt::CaseSensitive : Qt::CaseInsensitive;

    if (cursor.hasSelection() && QString::compare(cursor.selectedText(), query, cs) == 0) {
        cursor.insertText(replacement);
        find(query, matchCase, false, m_lastWrapAround);
        return true;
    }

    // If current selection is not the match, find the next match first
    return find(query, matchCase, false, m_lastWrapAround);
}

int SearchController::replaceAll(const QString& query, const QString& replacement, bool matchCase) {
    if (!m_editor || query.isEmpty() || m_editor->isReadOnly()) {
        return 0;
    }

    QTextDocument::FindFlags flags;
    if (matchCase) {
        flags |= QTextDocument::FindCaseSensitively;
    }

    QTextCursor cursor(m_editor->document());
    cursor.beginEditBlock();

    int count = 0;
    while (true) {
        cursor = m_editor->document()->find(query, cursor, flags);
        if (cursor.isNull()) {
            break;
        }
        cursor.insertText(replacement);
        count++;
    }

    cursor.endEditBlock();
    return count;
}

bool SearchController::goToLine(int line) {
    if (!m_editor || line < 1) {
        return false;
    }

    int blockCount = m_editor->document()->blockCount();
    if (line > blockCount) {
        return false;
    }

    QTextBlock block = m_editor->document()->findBlockByLineNumber(line - 1);
    if (!block.isValid()) {
        return false;
    }

    QTextCursor cursor(block);
    m_editor->setTextCursor(cursor);
    m_editor->ensureCursorVisible();
    return true;
}

QString SearchController::lastSearchText() const {
    return m_lastSearchText;
}

bool SearchController::lastMatchCase() const {
    return m_lastMatchCase;
}

} // namespace YANP
