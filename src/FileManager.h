#pragma once

#include <QString>
#include <QStringList>
#include <QByteArray>
#include <QDateTime>
#include <QObject>

class QFileSystemWatcher;
class QTimer;

namespace YANP {

enum class LineEnding {
    LF,
    CRLF
};

enum class FileEncoding {
    UTF8,
    UTF8_BOM,
    UTF16_LE,
    UTF16_BE,
    SystemLocale
};

struct FileResult {
    bool success = false;
    QString errorMessage;
    QString content;
};

class FileManager : public QObject {
    Q_OBJECT
public:
    static constexpr int MaxRecentFiles = 10;

    explicit FileManager(QObject* parent = nullptr);

    void reset();
    FileResult openFile(const QString& filePath);
    FileResult saveFile(const QString& filePath, const QString& content);

    QString filePath() const;
    void setFilePath(const QString& path);

    bool isModified() const;
    void setModified(bool modified);

    QString fileName() const;
    QString windowTitle() const;

    FileEncoding encoding() const;
    QString encodingName() const;
    void setEncoding(FileEncoding enc);

    LineEnding lineEnding() const;
    QString lineEndingName() const;
    void setLineEnding(LineEnding le);

    QStringList recentFiles() const;
    void recordRecentFile(const QString& path);
    void removeRecentFile(const QString& path);
    void clearRecentFiles();

signals:
    void modifiedChanged(bool modified);
    void fileChanged(const QString& filePath);
    void recentFilesChanged();
    void fileChangedExternally(const QString& filePath);

private slots:
    void onWatchedFileChanged(const QString& path);
    void onWatchedDirectoryChanged(const QString& path);
    void checkExternalChange();

private:
    void startWatching(const QString& path);
    void stopWatching();
    void rehookWatcher();

    QString m_filePath;
    bool m_isModified = false;
    FileEncoding m_encoding = FileEncoding::UTF8;
    LineEnding m_lineEnding = LineEnding::LF;

    QFileSystemWatcher* m_watcher = nullptr;
    QTimer* m_debounceTimer = nullptr;
    QString m_watchedDirectory;
    QByteArray m_lastDiskHash;
    QDateTime m_lastDiskTime;
};

} // namespace YANP
