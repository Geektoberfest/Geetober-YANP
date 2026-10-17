#include <QTest>
#include <QTemporaryDir>
#include <QFile>
#include <QFileInfo>
#include <QSignalSpy>
#include <stdio.h>
#include "FileManager.h"

using namespace YANP;

class TestFileManager : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void testInitialState();
    void testOpenNonExistentFile();
    void testSaveAndOpenUtf8();
    void testUtf8BomHandling();
    void testUtf16LeHandling();
    void testLineEndingDetectionAndPreservation();
    void testModifiedStateAndTitle();
    void testRecentFilesEmptyInitially();
    void testRecordRecentFileDeduplicationAndOrder();
    void testRecentFilesMaxCap();
    void testRemoveRecentFile();
    void testClearRecentFiles();
    void testOpenFileAndSaveFileUpdatesRecentFiles();
    void testRecentFilesSignals();
    void testExternalChangeDetectionClean();
    void testSelfSaveDoesNotTriggerExternalChange();
    void testAtomicRenameDetected();
    void testRapidChangesDebouncedToOneEvent();

private:
    std::unique_ptr<QTemporaryDir> m_tempDir;
};

void TestFileManager::initTestCase() {
    m_tempDir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_tempDir->isValid());
    QCoreApplication::setOrganizationName(QStringLiteral("Geektober"));
    QCoreApplication::setApplicationName(QStringLiteral("YANP_Test"));
}

void TestFileManager::cleanupTestCase() {
    FileManager fm;
    fm.clearRecentFiles();
}

void TestFileManager::init() {
    FileManager fm;
    fm.clearRecentFiles();
}

void TestFileManager::testInitialState() {
    FileManager fm;
    QCOMPARE(fm.filePath(), QString());
    QCOMPARE(fm.fileName(), QStringLiteral("Untitled"));
    QCOMPARE(fm.isModified(), false);
    QCOMPARE(fm.windowTitle(), QStringLiteral("Untitled - YANP"));
}

void TestFileManager::testOpenNonExistentFile() {
    FileManager fm;
    FileResult res = fm.openFile(QStringLiteral("/path/does/not/exist/yanp_test.txt"));
    QVERIFY(!res.success);
    QVERIFY(!res.errorMessage.isEmpty());
    QCOMPARE(fm.filePath(), QString());
}

void TestFileManager::testSaveAndOpenUtf8() {
    FileManager fm;
    QString filePath = m_tempDir->filePath(QStringLiteral("test_utf8.txt"));
    QString content = QStringLiteral("Hello World from YANP!\nSecond line with unicode: 🚀 — é à ü ñ");

    FileResult saveRes = fm.saveFile(filePath, content);
    QVERIFY(saveRes.success);
    QCOMPARE(fm.isModified(), false);
    QCOMPARE(fm.filePath(), filePath);
    QCOMPARE(fm.fileName(), QStringLiteral("test_utf8.txt"));

    FileManager fm2;
    FileResult openRes = fm2.openFile(filePath);
    QVERIFY(openRes.success);
    QCOMPARE(openRes.content, content);
    QCOMPARE(fm2.encoding(), FileEncoding::UTF8);
}

void TestFileManager::testUtf8BomHandling() {
    QString filePath = m_tempDir->filePath(QStringLiteral("test_bom.txt"));

    // Write file with UTF-8 BOM
    {
        QFile file(filePath);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("\xEF\xBB\xBF");
        file.write("BOM content text");
        file.close();
    }

    FileManager fm;
    FileResult openRes = fm.openFile(filePath);
    QVERIFY(openRes.success);
    QCOMPARE(openRes.content, QStringLiteral("BOM content text"));
    QCOMPARE(fm.encoding(), FileEncoding::UTF8_BOM);

    // Save and verify BOM is preserved
    FileResult saveRes = fm.saveFile(filePath, QStringLiteral("Updated BOM content"));
    QVERIFY(saveRes.success);

    QFile checkFile(filePath);
    QVERIFY(checkFile.open(QIODevice::ReadOnly));
    QByteArray data = checkFile.readAll();
    checkFile.close();

    QVERIFY(data.startsWith("\xEF\xBB\xBF"));
    QCOMPARE(data.mid(3), QByteArray("Updated BOM content"));
}

void TestFileManager::testUtf16LeHandling() {
    QString filePath = m_tempDir->filePath(QStringLiteral("test_utf16le.txt"));

    // Write UTF-16 LE file
    {
        QFile file(filePath);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("\xFF\xFE");
        // "Hi" in UTF-16LE: 'H'(0x48, 0x00), 'i'(0x69, 0x00)
        const char hi[] = {'H', 0, 'i', 0};
        file.write(hi, 4);
        file.close();
    }

    FileManager fm;
    FileResult openRes = fm.openFile(filePath);
    QVERIFY(openRes.success);
    QCOMPARE(openRes.content, QStringLiteral("Hi"));
    QCOMPARE(fm.encoding(), FileEncoding::UTF16_LE);
}

void TestFileManager::testLineEndingDetectionAndPreservation() {
    QString filePathCrlf = m_tempDir->filePath(QStringLiteral("test_crlf.txt"));
    {
        QFile file(filePathCrlf);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("Line1\r\nLine2\r\nLine3");
        file.close();
    }

    FileManager fm;
    FileResult openRes = fm.openFile(filePathCrlf);
    QVERIFY(openRes.success);
    QCOMPARE(fm.lineEnding(), LineEnding::CRLF);

    // Save with CRLF
    fm.saveFile(filePathCrlf, QStringLiteral("A\nB"));
    QFile checkFile(filePathCrlf);
    QVERIFY(checkFile.open(QIODevice::ReadOnly));
    QByteArray data = checkFile.readAll();
    checkFile.close();
    QCOMPARE(data, QByteArray("A\r\nB"));
}

void TestFileManager::testModifiedStateAndTitle() {
    FileManager fm;
    QCOMPARE(fm.windowTitle(), QStringLiteral("Untitled - YANP"));

    fm.setModified(true);
    QCOMPARE(fm.windowTitle(), QStringLiteral("*Untitled - YANP"));

    fm.setFilePath(QStringLiteral("/some/path/notes.txt"));
    QCOMPARE(fm.windowTitle(), QStringLiteral("*notes.txt - YANP"));

    fm.setModified(false);
    QCOMPARE(fm.windowTitle(), QStringLiteral("notes.txt - YANP"));
}

void TestFileManager::testRecentFilesEmptyInitially() {
    FileManager fm;
    QVERIFY(fm.recentFiles().isEmpty());
}

void TestFileManager::testRecordRecentFileDeduplicationAndOrder() {
    FileManager fm;
    QString file1 = m_tempDir->filePath(QStringLiteral("file1.txt"));
    QString file2 = m_tempDir->filePath(QStringLiteral("file2.txt"));
    QString file3 = m_tempDir->filePath(QStringLiteral("file3.txt"));

    for (const QString& path : {file1, file2, file3}) {
        QFile f(path);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("content");
        f.close();
    }

    fm.recordRecentFile(file1);
    fm.recordRecentFile(file2);
    fm.recordRecentFile(file3);

    QStringList list = fm.recentFiles();
    QCOMPARE(list.size(), 3);
    QCOMPARE(list.at(0), QFileInfo(file3).canonicalFilePath());
    QCOMPARE(list.at(1), QFileInfo(file2).canonicalFilePath());
    QCOMPARE(list.at(2), QFileInfo(file1).canonicalFilePath());

    // Record file1 again - should move to top with no duplicates
    fm.recordRecentFile(file1);
    list = fm.recentFiles();
    QCOMPARE(list.size(), 3);
    QCOMPARE(list.at(0), QFileInfo(file1).canonicalFilePath());
    QCOMPARE(list.at(1), QFileInfo(file3).canonicalFilePath());
    QCOMPARE(list.at(2), QFileInfo(file2).canonicalFilePath());
}

void TestFileManager::testRecentFilesMaxCap() {
    FileManager fm;
    QStringList createdFiles;
    for (int i = 1; i <= 15; ++i) {
        QString path = m_tempDir->filePath(QStringLiteral("cap_file_%1.txt").arg(i));
        QFile f(path);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("test");
        f.close();
        createdFiles.append(path);
        fm.recordRecentFile(path);
    }

    QStringList list = fm.recentFiles();
    QCOMPARE(list.size(), 10);
    QCOMPARE(list.first(), QFileInfo(createdFiles.last()).canonicalFilePath());
    QCOMPARE(list.last(), QFileInfo(createdFiles.at(5)).canonicalFilePath());
}

void TestFileManager::testRemoveRecentFile() {
    FileManager fm;
    QString file1 = m_tempDir->filePath(QStringLiteral("rm1.txt"));
    QString file2 = m_tempDir->filePath(QStringLiteral("rm2.txt"));
    for (const QString& path : {file1, file2}) {
        QFile f(path);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("data");
        f.close();
        fm.recordRecentFile(path);
    }

    QCOMPARE(fm.recentFiles().size(), 2);
    fm.removeRecentFile(file1);

    QStringList list = fm.recentFiles();
    QCOMPARE(list.size(), 1);
    QCOMPARE(list.first(), QFileInfo(file2).canonicalFilePath());
}

void TestFileManager::testClearRecentFiles() {
    FileManager fm;
    QString file1 = m_tempDir->filePath(QStringLiteral("clear1.txt"));
    QFile f(file1);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("data");
    f.close();
    fm.recordRecentFile(file1);
    QCOMPARE(fm.recentFiles().size(), 1);

    fm.clearRecentFiles();
    QVERIFY(fm.recentFiles().isEmpty());
}

void TestFileManager::testOpenFileAndSaveFileUpdatesRecentFiles() {
    FileManager fm;
    QString path = m_tempDir->filePath(QStringLiteral("save_open_recent.txt"));

    FileResult saveRes = fm.saveFile(path, QStringLiteral("Testing"));
    QVERIFY(saveRes.success);
    QCOMPARE(fm.recentFiles().size(), 1);
    QCOMPARE(fm.recentFiles().first(), QFileInfo(path).canonicalFilePath());

    fm.clearRecentFiles();
    QVERIFY(fm.recentFiles().isEmpty());

    FileResult openRes = fm.openFile(path);
    QVERIFY(openRes.success);
    QCOMPARE(fm.recentFiles().size(), 1);
    QCOMPARE(fm.recentFiles().first(), QFileInfo(path).canonicalFilePath());
}

void TestFileManager::testRecentFilesSignals() {
    FileManager fm;
    QSignalSpy spy(&fm, &FileManager::recentFilesChanged);

    QString path = m_tempDir->filePath(QStringLiteral("sig_test.txt"));
    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("signal test");
    f.close();

    fm.recordRecentFile(path);
    QCOMPARE(spy.count(), 1);

    fm.removeRecentFile(path);
    QCOMPARE(spy.count(), 2);

    fm.clearRecentFiles();
    QCOMPARE(spy.count(), 3);
}

void TestFileManager::testExternalChangeDetectionClean() {
    FileManager fm;
    QString filePath = m_tempDir->filePath(QStringLiteral("watch_test.txt"));
    FileResult res = fm.saveFile(filePath, QStringLiteral("Original content"));
    QVERIFY(res.success);

    QSignalSpy spy(&fm, &FileManager::fileChangedExternally);

    // Modify file externally
    {
        QFile file(filePath);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
        file.write("External new content");
        file.close();
    }

    QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, 2000);
    QCOMPARE(spy.first().at(0).toString(), filePath);
}

void TestFileManager::testSelfSaveDoesNotTriggerExternalChange() {
    FileManager fm;
    QString filePath = m_tempDir->filePath(QStringLiteral("self_save_test.txt"));
    FileResult res = fm.saveFile(filePath, QStringLiteral("Initial content"));
    QVERIFY(res.success);

    QSignalSpy spy(&fm, &FileManager::fileChangedExternally);

    // Save again from within YANP
    FileResult res2 = fm.saveFile(filePath, QStringLiteral("Updated content by YANP"));
    QVERIFY(res2.success);

    QTest::qWait(400);
    QCOMPARE(spy.count(), 0);
}

void TestFileManager::testAtomicRenameDetected() {
    FileManager fm;
    QString filePath = m_tempDir->filePath(QStringLiteral("atomic_test.txt"));
    FileResult res = fm.saveFile(filePath, QStringLiteral("Before atomic rename"));
    QVERIFY(res.success);

    QSignalSpy spy(&fm, &FileManager::fileChangedExternally);

    // Simulate atomic write 1 (temp file rename over original)
    QString tempPath1 = m_tempDir->filePath(QStringLiteral("atomic_temp1.tmp"));
    {
        QFile tempFile(tempPath1);
        QVERIFY(tempFile.open(QIODevice::WriteOnly));
        tempFile.write("First atomic content");
        tempFile.close();
        QCOMPARE(::rename(tempPath1.toLocal8Bit().constData(), filePath.toLocal8Bit().constData()), 0);
    }

    QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, 2000);

    // Simulate atomic write 2 to verify watcher re-hooking worked
    QString tempPath2 = m_tempDir->filePath(QStringLiteral("atomic_temp2.tmp"));
    {
        QFile tempFile(tempPath2);
        QVERIFY(tempFile.open(QIODevice::WriteOnly));
        tempFile.write("Second atomic content");
        tempFile.close();
        QCOMPARE(::rename(tempPath2.toLocal8Bit().constData(), filePath.toLocal8Bit().constData()), 0);
    }

    QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 2, 2000);
}

void TestFileManager::testRapidChangesDebouncedToOneEvent() {
    FileManager fm;
    QString filePath = m_tempDir->filePath(QStringLiteral("debounce_test.txt"));
    FileResult res = fm.saveFile(filePath, QStringLiteral("Initial debounce content"));
    QVERIFY(res.success);

    QSignalSpy spy(&fm, &FileManager::fileChangedExternally);

    // Write rapidly 5 times
    for (int i = 0; i < 5; ++i) {
        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            file.write(QString("Rapid content %1").arg(i).toUtf8());
            file.close();
        }
        QTest::qWait(10);
    }

    // Wait for the debounce timer to settle and fire exactly once
    QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, 2000);
    QTest::qWait(300);
    QCOMPARE(spy.count(), 1);
}

QTEST_MAIN(TestFileManager)
#include "test_filemanager.moc"
