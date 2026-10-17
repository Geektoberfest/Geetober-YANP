#pragma once

#include <QPlainTextEdit>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QWheelEvent>

namespace YANP {

class EditorWidget : public QPlainTextEdit {
    Q_OBJECT
public:
    static constexpr int MinZoom = 10;
    static constexpr int MaxZoom = 500;
    static constexpr int ZoomStep = 10;

    explicit EditorWidget(QWidget* parent = nullptr);

    void setWordWrapEnabled(bool enabled);
    bool isWordWrapEnabled() const;

    void insertDateTime();
    void deleteSelection();

    int currentLine() const;
    int currentColumn() const;

    void setBaseFont(const QFont& font);
    QFont baseFont() const;
    void setFont(const QFont& font);

    int zoomPercent() const;
    void setZoomPercent(int percent);

public slots:
    void zoomIn();
    void zoomOut();
    void resetZoom();

signals:
    void cursorCoordinatesChanged(int line, int column);
    void fileDropped(const QString& filePath);
    void zoomChanged(int percent);

protected:
    bool event(QEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private slots:
    void onCursorPositionChanged();

private:
    void applyZoom();

    QFont m_baseFont;
    int m_zoomPercent = 100;
};

} // namespace YANP
