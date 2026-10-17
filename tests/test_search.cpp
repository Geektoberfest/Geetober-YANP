#include <QTest>
#include <QSignalSpy>
#include <QWheelEvent>
#include "EditorWidget.h"
#include "SearchController.h"

using namespace YANP;

class TestSearch : public QObject {
    Q_OBJECT

private slots:
    void testFindBasic();
    void testFindCaseSensitive();
    void testFindWrapAround();
    void testReplaceSingle();
    void testReplaceAll();
    void testGoToLine();
    void testEmptyQuery();
    void testReadOnlyBlocksEdits();
    void testZoomInAndOut();
    void testZoomSettingsIsolation();
    void testWheelZoom();
};

void TestSearch::testFindBasic() {
    EditorWidget editor;
    editor.setPlainText(QStringLiteral("The quick brown fox jumps over the lazy dog."));

    SearchController sc(&editor);
    bool found = sc.find(QStringLiteral("brown"), false, false, false);
    QVERIFY(found);
    QCOMPARE(editor.textCursor().selectedText(), QStringLiteral("brown"));

    bool notFound = sc.find(QStringLiteral("elephant"), false, false, false);
    QVERIFY(!notFound);
}

void TestSearch::testFindCaseSensitive() {
    EditorWidget editor;
    editor.setPlainText(QStringLiteral("Apple apple APPLE"));

    SearchController sc(&editor);

    // Case insensitive find
    editor.moveCursor(QTextCursor::Start);
    QVERIFY(sc.find(QStringLiteral("apple"), false, false, false));
    QCOMPARE(editor.textCursor().selectedText(), QStringLiteral("Apple"));

    // Case sensitive find
    editor.moveCursor(QTextCursor::Start);
    QVERIFY(sc.find(QStringLiteral("apple"), true, false, false));
    QCOMPARE(editor.textCursor().selectedText(), QStringLiteral("apple"));

    // Exact uppercase
    editor.moveCursor(QTextCursor::Start);
    QVERIFY(sc.find(QStringLiteral("APPLE"), true, false, false));
    QCOMPARE(editor.textCursor().selectedText(), QStringLiteral("APPLE"));
}

void TestSearch::testFindWrapAround() {
    EditorWidget editor;
    editor.setPlainText(QStringLiteral("First line\nSecond line\nThird line"));

    SearchController sc(&editor);
    // Cursor at the very end
    editor.moveCursor(QTextCursor::End);

    // Without wrap: false
    QVERIFY(!sc.find(QStringLiteral("First"), false, false, false));

    // With wrap: true
    QVERIFY(sc.find(QStringLiteral("First"), false, false, true));
    QCOMPARE(editor.textCursor().selectedText(), QStringLiteral("First"));
}

void TestSearch::testReplaceSingle() {
    EditorWidget editor;
    editor.setPlainText(QStringLiteral("cat sat on the cat mat"));

    SearchController sc(&editor);
    editor.moveCursor(QTextCursor::Start);

    // Find first "cat"
    QVERIFY(sc.find(QStringLiteral("cat"), false, false, false));
    // Replace it
    QVERIFY(sc.replace(QStringLiteral("cat"), QStringLiteral("dog"), false));
    QCOMPARE(editor.toPlainText(), QStringLiteral("dog sat on the cat mat"));
}

void TestSearch::testReplaceAll() {
    EditorWidget editor;
    editor.setPlainText(QStringLiteral("foo bar foo baz foo"));

    SearchController sc(&editor);
    int count = sc.replaceAll(QStringLiteral("foo"), QStringLiteral("qux"), false);
    QCOMPARE(count, 3);
    QCOMPARE(editor.toPlainText(), QStringLiteral("qux bar qux baz qux"));
}

void TestSearch::testGoToLine() {
    EditorWidget editor;
    editor.setPlainText(QStringLiteral("Line 1\nLine 2\nLine 3\nLine 4\nLine 5"));

    SearchController sc(&editor);
    QVERIFY(sc.goToLine(3));
    QCOMPARE(editor.currentLine(), 3);

    QVERIFY(sc.goToLine(1));
    QCOMPARE(editor.currentLine(), 1);

    QVERIFY(sc.goToLine(5));
    QCOMPARE(editor.currentLine(), 5);

    // Out of bounds
    QVERIFY(!sc.goToLine(0));
    QVERIFY(!sc.goToLine(10));
}

void TestSearch::testEmptyQuery() {
    EditorWidget editor;
    editor.setPlainText(QStringLiteral("Some text"));

    SearchController sc(&editor);
    QVERIFY(!sc.find(QString(), false, false, false));
    QCOMPARE(sc.replaceAll(QString(), QStringLiteral("xyz"), false), 0);
}

void TestSearch::testReadOnlyBlocksEdits() {
    EditorWidget editor;
    editor.setPlainText(QStringLiteral("Keep me"));
    editor.setReadOnly(true);

    SearchController sc(&editor);

    // Select a word, then try every edit path that bypasses QPlainTextEdit's own checks
    editor.moveCursor(QTextCursor::Start);
    editor.moveCursor(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
    editor.deleteSelection();
    QCOMPARE(editor.toPlainText(), QStringLiteral("Keep me"));

    editor.insertDateTime();
    QCOMPARE(editor.toPlainText(), QStringLiteral("Keep me"));

    QCOMPARE(sc.replaceAll(QStringLiteral("Keep"), QStringLiteral("Lose"), false), 0);
    QCOMPARE(editor.toPlainText(), QStringLiteral("Keep me"));

    // Copy and selection still work in read-only mode
    QVERIFY(editor.textCursor().hasSelection());
    QCOMPARE(editor.textCursor().selectedText(), QStringLiteral("Keep"));
}

void TestSearch::testZoomInAndOut() {
    EditorWidget editor;
    QCOMPARE(editor.zoomPercent(), 100);

    QSignalSpy spy(&editor, &EditorWidget::zoomChanged);

    editor.zoomIn();
    QCOMPARE(editor.zoomPercent(), 110);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().at(0).toInt(), 110);

    editor.zoomIn();
    QCOMPARE(editor.zoomPercent(), 120);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().at(0).toInt(), 120);

    editor.zoomOut();
    QCOMPARE(editor.zoomPercent(), 110);

    editor.resetZoom();
    QCOMPARE(editor.zoomPercent(), 100);

    // Test upper clamp
    editor.setZoomPercent(600);
    QCOMPARE(editor.zoomPercent(), EditorWidget::MaxZoom);

    // Test lower clamp
    editor.setZoomPercent(5);
    QCOMPARE(editor.zoomPercent(), EditorWidget::MinZoom);
}

void TestSearch::testZoomSettingsIsolation() {
    EditorWidget editor;
    QFont baseFont(QStringLiteral("Monospace"), 12);
    editor.setBaseFont(baseFont);

    QCOMPARE(editor.baseFont().pointSize(), 12);

    editor.setZoomPercent(200);
    QCOMPARE(editor.zoomPercent(), 200);

    // The displayed font scales to 24pt
    QCOMPARE(qRound(editor.font().pointSizeF()), 24);

    // The base font preserved for settings remains 12pt
    QCOMPARE(editor.baseFont().pointSize(), 12);

    editor.resetZoom();
    QCOMPARE(qRound(editor.font().pointSizeF()), 12);
}

void TestSearch::testWheelZoom() {
    EditorWidget editor;
    QCOMPARE(editor.zoomPercent(), 100);

    // Wheel event with Ctrl modifier zooming in via viewport
    QWheelEvent wheelIn(
        QPointF(10, 10),
        QPointF(10, 10),
        QPoint(0, 0),
        QPoint(0, 120),
        Qt::NoButton,
        Qt::ControlModifier,
        Qt::NoScrollPhase,
        false
    );
    QCoreApplication::sendEvent(editor.viewport(), &wheelIn);
    QCOMPARE(editor.zoomPercent(), 110);

    // Wheel event with Ctrl modifier zooming in via editor directly
    QCoreApplication::sendEvent(&editor, &wheelIn);
    QCOMPARE(editor.zoomPercent(), 120);

    // Wheel event with Ctrl modifier zooming out
    QWheelEvent wheelOut(
        QPointF(10, 10),
        QPointF(10, 10),
        QPoint(0, 0),
        QPoint(0, -120),
        Qt::NoButton,
        Qt::ControlModifier,
        Qt::NoScrollPhase,
        false
    );
    QCoreApplication::sendEvent(editor.viewport(), &wheelOut);
    QCOMPARE(editor.zoomPercent(), 110);

    // Wheel event without Ctrl modifier should NOT change zoom
    QWheelEvent wheelNoCtrl(
        QPointF(10, 10),
        QPointF(10, 10),
        QPoint(0, 0),
        QPoint(0, 120),
        Qt::NoButton,
        Qt::NoModifier,
        Qt::NoScrollPhase,
        false
    );
    QCoreApplication::sendEvent(editor.viewport(), &wheelNoCtrl);
    QCOMPARE(editor.zoomPercent(), 110);
}

QTEST_MAIN(TestSearch)
#include "test_search.moc"
