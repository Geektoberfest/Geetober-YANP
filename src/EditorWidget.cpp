#include "EditorWidget.h"
#include <QDateTime>
#include <QMimeData>
#include <QUrl>
#include <QTextCursor>
#include <QScrollBar>

namespace YANP {

EditorWidget::EditorWidget(QWidget* parent)
    : QPlainTextEdit(parent)
{
    setAcceptDrops(true);
    setWordWrapEnabled(true);

    connect(this, &QPlainTextEdit::cursorPositionChanged,
            this, &EditorWidget::onCursorPositionChanged);

    onCursorPositionChanged();
}

void EditorWidget::setWordWrapEnabled(bool enabled) {
    if (enabled) {
        setLineWrapMode(QPlainTextEdit::WidgetWidth);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    } else {
        setLineWrapMode(QPlainTextEdit::NoWrap);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    }
}

bool EditorWidget::isWordWrapEnabled() const {
    return lineWrapMode() != QPlainTextEdit::NoWrap;
}

void EditorWidget::insertDateTime() {
    // textCursor() edits bypass QPlainTextEdit's read-only check, so guard here
    if (isReadOnly()) {
        return;
    }
    // Standard Notepad format: e.g. "11:05 PM 10/13/2026"
    QString dtStr = QDateTime::currentDateTime().toString(QStringLiteral("hh:mm AP MM/dd/yyyy"));
    textCursor().insertText(dtStr);
}

void EditorWidget::deleteSelection() {
    if (isReadOnly()) {
        return;
    }
    QTextCursor cursor = textCursor();
    if (cursor.hasSelection()) {
        cursor.removeSelectedText();
    }
}

int EditorWidget::currentLine() const {
    return textCursor().blockNumber() + 1;
}

int EditorWidget::currentColumn() const {
    return textCursor().positionInBlock() + 1;
}

void EditorWidget::onCursorPositionChanged() {
    emit cursorCoordinatesChanged(currentLine(), currentColumn());
}

void EditorWidget::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) {
        const QList<QUrl> urls = event->mimeData()->urls();
        if (!urls.isEmpty() && urls.first().isLocalFile()) {
            event->acceptProposedAction();
            return;
        }
    }
    QPlainTextEdit::dragEnterEvent(event);
}

void EditorWidget::dragMoveEvent(QDragMoveEvent* event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
        return;
    }
    QPlainTextEdit::dragMoveEvent(event);
}

void EditorWidget::dropEvent(QDropEvent* event) {
    if (event->mimeData()->hasUrls()) {
        const QList<QUrl> urls = event->mimeData()->urls();
        if (!urls.isEmpty() && urls.first().isLocalFile()) {
            event->acceptProposedAction();
            emit fileDropped(urls.first().toLocalFile());
            return;
        }
    }
    QPlainTextEdit::dropEvent(event);
}

} // namespace YANP
