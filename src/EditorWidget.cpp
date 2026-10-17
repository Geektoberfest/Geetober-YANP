#include "EditorWidget.h"
#include <QDateTime>
#include <QMimeData>
#include <QUrl>
#include <QTextCursor>
#include <QScrollBar>
#include <algorithm>

namespace YANP {

EditorWidget::EditorWidget(QWidget* parent)
    : QPlainTextEdit(parent),
      m_baseFont(font()),
      m_zoomPercent(100)
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

bool EditorWidget::event(QEvent* event) {
    if (event->type() == QEvent::Wheel) {
        wheelEvent(static_cast<QWheelEvent*>(event));
        return event->isAccepted();
    }
    return QPlainTextEdit::event(event);
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

void EditorWidget::setBaseFont(const QFont& font) {
    m_baseFont = font;
    applyZoom();
}

QFont EditorWidget::baseFont() const {
    return m_baseFont;
}

void EditorWidget::setFont(const QFont& font) {
    setBaseFont(font);
}

int EditorWidget::zoomPercent() const {
    return m_zoomPercent;
}

void EditorWidget::setZoomPercent(int percent) {
    percent = std::clamp(percent, MinZoom, MaxZoom);
    if (m_zoomPercent != percent) {
        m_zoomPercent = percent;
        applyZoom();
        emit zoomChanged(m_zoomPercent);
    }
}

void EditorWidget::zoomIn() {
    setZoomPercent(m_zoomPercent + ZoomStep);
}

void EditorWidget::zoomOut() {
    setZoomPercent(m_zoomPercent - ZoomStep);
}

void EditorWidget::resetZoom() {
    setZoomPercent(100);
}

void EditorWidget::applyZoom() {
    qreal baseSize = m_baseFont.pointSizeF();
    if (baseSize <= 0) {
        baseSize = m_baseFont.pointSize();
    }
    if (baseSize <= 0) {
        baseSize = 11.0;
    }

    qreal scaledSize = baseSize * (static_cast<qreal>(m_zoomPercent) / 100.0);
    if (scaledSize < 1.0) {
        scaledSize = 1.0;
    }

    QFont zoomedFont = m_baseFont;
    zoomedFont.setPointSizeF(scaledSize);
    QPlainTextEdit::setFont(zoomedFont);
}

void EditorWidget::wheelEvent(QWheelEvent* event) {
    if (event->modifiers() & Qt::ControlModifier) {
        const int delta = event->angleDelta().y();
        if (delta > 0) {
            zoomIn();
            event->accept();
            return;
        } else if (delta < 0) {
            zoomOut();
            event->accept();
            return;
        }
    }
    QPlainTextEdit::wheelEvent(event);
}

} // namespace YANP
