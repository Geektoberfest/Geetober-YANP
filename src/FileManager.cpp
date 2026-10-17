#include "FileManager.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QStringDecoder>
#include <QStringEncoder>
#include <QSaveFile>
#include <QSettings>
#include <QFileSystemWatcher>
#include <QTimer>
#include <QCryptographicHash>

namespace {

QString normalizePath(const QString& path) {
    if (path.isEmpty()) {
        return QString();
    }
    QFileInfo fi(path);
    QString canonical = fi.canonicalFilePath();
    if (!canonical.isEmpty()) {
        return canonical;
    }
    return QDir::cleanPath(fi.absoluteFilePath());
}

} // namespace

namespace YANP {

FileManager::FileManager(QObject* parent)
    : QObject(parent),
      m_watcher(new QFileSystemWatcher(this)),
      m_debounceTimer(new QTimer(this))
{
#if defined(Q_OS_WIN)
    m_lineEnding = LineEnding::CRLF;
#else
    m_lineEnding = LineEnding::LF;
#endif

    m_debounceTimer->setSingleShot(true);
    m_debounceTimer->setInterval(150);
    connect(m_debounceTimer, &QTimer::timeout, this, &FileManager::checkExternalChange);

    connect(m_watcher, &QFileSystemWatcher::fileChanged, this, &FileManager::onWatchedFileChanged);
    connect(m_watcher, &QFileSystemWatcher::directoryChanged, this, &FileManager::onWatchedDirectoryChanged);
}

void FileManager::reset() {
    stopWatching();
    m_filePath.clear();
    m_isModified = false;
    m_lastDiskHash.clear();
    m_lastDiskTime = QDateTime();
    m_encoding = FileEncoding::UTF8;
#if defined(Q_OS_WIN)
    m_lineEnding = LineEnding::CRLF;
#else
    m_lineEnding = LineEnding::LF;
#endif
    emit modifiedChanged(false);
    emit fileChanged(QString());
}

FileResult FileManager::openFile(const QString& filePath) {
    FileResult result;
    QFile file(filePath);

    if (!file.exists()) {
        result.success = false;
        result.errorMessage = tr("File does not exist: %1").arg(filePath);
        return result;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        result.success = false;
        result.errorMessage = tr("Cannot open file for reading: %1\nError: %2")
            .arg(filePath, file.errorString());
        return result;
    }

    QByteArray rawData = file.readAll();
    file.close();

    // Detect BOM and encoding
    FileEncoding detectedEncoding = FileEncoding::UTF8;
    QString decodedContent;

    if (rawData.size() >= 3 &&
        static_cast<unsigned char>(rawData[0]) == 0xEF &&
        static_cast<unsigned char>(rawData[1]) == 0xBB &&
        static_cast<unsigned char>(rawData[2]) == 0xBF) {
        detectedEncoding = FileEncoding::UTF8_BOM;
        decodedContent = QString::fromUtf8(rawData.constData() + 3, rawData.size() - 3);
    } else if (rawData.size() >= 2 &&
               static_cast<unsigned char>(rawData[0]) == 0xFF &&
               static_cast<unsigned char>(rawData[1]) == 0xFE) {
        detectedEncoding = FileEncoding::UTF16_LE;
        auto decoder = QStringDecoder(QStringDecoder::Utf16LE);
        decodedContent = decoder(rawData.mid(2));
    } else if (rawData.size() >= 2 &&
               static_cast<unsigned char>(rawData[0]) == 0xFE &&
               static_cast<unsigned char>(rawData[1]) == 0xFF) {
        detectedEncoding = FileEncoding::UTF16_BE;
        auto decoder = QStringDecoder(QStringDecoder::Utf16BE);
        decodedContent = decoder(rawData.mid(2));
    } else {
        // Try UTF-8 first
        auto decoder = QStringDecoder(QStringDecoder::Utf8, QStringDecoder::Flag::Stateless);
        decodedContent = decoder(rawData);
        if (decoder.hasError()) {
            // Fallback to local 8-bit
            detectedEncoding = FileEncoding::SystemLocale;
            decodedContent = QString::fromLocal8Bit(rawData);
        } else {
            detectedEncoding = FileEncoding::UTF8;
        }
    }

    // Detect line ending from raw bytes or content
    if (rawData.contains("\r\n")) {
        m_lineEnding = LineEnding::CRLF;
    } else if (rawData.contains("\n")) {
        m_lineEnding = LineEnding::LF;
    }

    m_encoding = detectedEncoding;
    m_filePath = QFileInfo(filePath).absoluteFilePath();
    m_isModified = false;
    m_lastDiskHash = QCryptographicHash::hash(rawData, QCryptographicHash::Sha256);
    m_lastDiskTime = QFileInfo(filePath).lastModified();

    result.success = true;
    result.content = decodedContent;

    recordRecentFile(m_filePath);
    startWatching(m_filePath);

    emit modifiedChanged(false);
    emit fileChanged(m_filePath);

    return result;
}

FileResult FileManager::saveFile(const QString& filePath, const QString& content) {
    FileResult result;

    // Use QSaveFile for safe atomic writing (does not overwrite/corrupt original on error)
    QSaveFile saveFile(filePath);
    if (!saveFile.open(QIODevice::WriteOnly)) {
        result.success = false;
        result.errorMessage = tr("Cannot open file for writing: %1\nError: %2")
            .arg(filePath, saveFile.errorString());
        return result;
    }

    // Normalize newlines based on line ending setting
    QString normalizedContent = content;
    normalizedContent.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    normalizedContent.replace(QStringLiteral("\r"), QStringLiteral("\n"));

    if (m_lineEnding == LineEnding::CRLF) {
        normalizedContent.replace(QStringLiteral("\n"), QStringLiteral("\r\n"));
    }

    QByteArray encodedData;
    switch (m_encoding) {
    case FileEncoding::UTF8_BOM: {
        encodedData.append("\xEF\xBB\xBF");
        encodedData.append(normalizedContent.toUtf8());
        break;
    }
    case FileEncoding::UTF16_LE: {
        encodedData.append("\xFF\xFE");
        auto encoder = QStringEncoder(QStringEncoder::Utf16LE);
        encodedData.append(encoder(normalizedContent));
        break;
    }
    case FileEncoding::UTF16_BE: {
        encodedData.append("\xFE\xFF");
        auto encoder = QStringEncoder(QStringEncoder::Utf16BE);
        encodedData.append(encoder(normalizedContent));
        break;
    }
    case FileEncoding::SystemLocale: {
        encodedData.append(normalizedContent.toLocal8Bit());
        break;
    }
    case FileEncoding::UTF8:
    default: {
        encodedData.append(normalizedContent.toUtf8());
        break;
    }
    }

    qint64 bytesWritten = saveFile.write(encodedData);
    if (bytesWritten != encodedData.size()) {
        saveFile.cancelWriting();
        result.success = false;
        result.errorMessage = tr("Failed to write all data to file: %1").arg(filePath);
        return result;
    }

    if (!saveFile.commit()) {
        result.success = false;
        result.errorMessage = tr("Failed to finalize file: %1\nError: %2")
            .arg(filePath, saveFile.errorString());
        return result;
    }

    m_filePath = QFileInfo(filePath).absoluteFilePath();
    m_isModified = false;
    m_lastDiskHash = QCryptographicHash::hash(encodedData, QCryptographicHash::Sha256);
    m_lastDiskTime = QFileInfo(m_filePath).lastModified();

    result.success = true;
    result.content = content;

    recordRecentFile(m_filePath);
    startWatching(m_filePath);

    emit modifiedChanged(false);
    emit fileChanged(m_filePath);

    return result;
}

QString FileManager::filePath() const {
    return m_filePath;
}

void FileManager::setFilePath(const QString& path) {
    m_filePath = path;
    if (!m_filePath.isEmpty()) {
        startWatching(m_filePath);
    } else {
        stopWatching();
    }
    emit fileChanged(m_filePath);
}

bool FileManager::isModified() const {
    return m_isModified;
}

void FileManager::setModified(bool modified) {
    if (m_isModified != modified) {
        m_isModified = modified;
        emit modifiedChanged(m_isModified);
    }
}

QString FileManager::fileName() const {
    if (m_filePath.isEmpty()) {
        return tr("Untitled");
    }
    return QFileInfo(m_filePath).fileName();
}

QString FileManager::windowTitle() const {
    QString title = fileName();
    if (m_isModified) {
        title.prepend(QStringLiteral("*"));
    }
    title.append(QStringLiteral(" - YANP"));
    return title;
}

FileEncoding FileManager::encoding() const {
    return m_encoding;
}

QString FileManager::encodingName() const {
    switch (m_encoding) {
    case FileEncoding::UTF8:
        return QStringLiteral("UTF-8");
    case FileEncoding::UTF8_BOM:
        return QStringLiteral("UTF-8 with BOM");
    case FileEncoding::UTF16_LE:
        return QStringLiteral("UTF-16 LE");
    case FileEncoding::UTF16_BE:
        return QStringLiteral("UTF-16 BE");
    case FileEncoding::SystemLocale:
        return QStringLiteral("System Locale");
    }
    return QStringLiteral("UTF-8");
}

void FileManager::setEncoding(FileEncoding enc) {
    m_encoding = enc;
}

LineEnding FileManager::lineEnding() const {
    return m_lineEnding;
}

QString FileManager::lineEndingName() const {
    switch (m_lineEnding) {
    case LineEnding::CRLF:
        return QStringLiteral("Windows (CRLF)");
    case LineEnding::LF:
        return QStringLiteral("Unix (LF)");
    }
    return QStringLiteral("Unix (LF)");
}

void FileManager::setLineEnding(LineEnding le) {
    m_lineEnding = le;
}

QStringList FileManager::recentFiles() const {
    QSettings settings;
    QVariant val = settings.value(QStringLiteral("recentFiles/list"));
    QStringList result;

    if (val.isValid()) {
        const QStringList rawList = val.toStringList();
        for (const QString& item : rawList) {
            QString trimmed = item.trimmed();
            if (!trimmed.isEmpty() && !result.contains(trimmed)) {
                result.append(trimmed);
                if (result.size() >= MaxRecentFiles) {
                    break;
                }
            }
        }
    }
    return result;
}

void FileManager::recordRecentFile(const QString& path) {
    QString normalized = normalizePath(path);
    if (normalized.isEmpty()) {
        return;
    }

    QStringList files = recentFiles();
    files.removeAll(normalized);
    files.prepend(normalized);

    while (files.size() > MaxRecentFiles) {
        files.removeLast();
    }

    QSettings settings;
    settings.setValue(QStringLiteral("recentFiles/list"), files);

    emit recentFilesChanged();
}

void FileManager::removeRecentFile(const QString& path) {
    if (path.isEmpty()) {
        return;
    }

    QString normalized = normalizePath(path);
    QStringList files = recentFiles();
    bool changed = false;

    if (files.removeAll(path) > 0) {
        changed = true;
    }
    if (!normalized.isEmpty() && normalized != path && files.removeAll(normalized) > 0) {
        changed = true;
    }

    if (changed) {
        QSettings settings;
        settings.setValue(QStringLiteral("recentFiles/list"), files);
        emit recentFilesChanged();
    }
}

void FileManager::clearRecentFiles() {
    QSettings settings;
    settings.remove(QStringLiteral("recentFiles/list"));
    emit recentFilesChanged();
}

void FileManager::startWatching(const QString& path) {
    stopWatching();
    if (path.isEmpty() || !QFile::exists(path)) {
        return;
    }
    m_watcher->addPath(path);
    QString dir = QFileInfo(path).absolutePath();
    if (QDir(dir).exists()) {
        m_watchedDirectory = dir;
        m_watcher->addPath(dir);
    }
}

void FileManager::stopWatching() {
    if (m_debounceTimer) {
        m_debounceTimer->stop();
    }
    if (m_watcher) {
        const QStringList files = m_watcher->files();
        if (!files.isEmpty()) {
            m_watcher->removePaths(files);
        }
        if (!m_watchedDirectory.isEmpty()) {
            m_watcher->removePath(m_watchedDirectory);
            m_watchedDirectory.clear();
        }
    }
}

void FileManager::rehookWatcher() {
    if (m_filePath.isEmpty() || !m_watcher) {
        return;
    }
    if (m_watcher->files().contains(m_filePath)) {
        m_watcher->removePath(m_filePath);
    }
    if (QFile::exists(m_filePath)) {
        m_watcher->addPath(m_filePath);
    } else {
        // Retry shortly in case file was momentarily unlinked during atomic rename
        QTimer::singleShot(50, this, [this]() {
            if (!m_filePath.isEmpty() && QFile::exists(m_filePath) && m_watcher) {
                if (!m_watcher->files().contains(m_filePath)) {
                    m_watcher->addPath(m_filePath);
                }
            }
        });
    }
}

void FileManager::onWatchedFileChanged(const QString& path) {
    if (path != m_filePath) {
        return;
    }
    rehookWatcher();
    m_debounceTimer->start();
}

void FileManager::onWatchedDirectoryChanged(const QString& dir) {
    if (dir != m_watchedDirectory || m_filePath.isEmpty()) {
        return;
    }
    // Re-hook if the file was replaced via atomic rename
    if (m_watcher && !m_watcher->files().contains(m_filePath) && QFile::exists(m_filePath)) {
        m_watcher->addPath(m_filePath);
    }
    m_debounceTimer->start();
}

void FileManager::checkExternalChange() {
    if (m_filePath.isEmpty() || !QFile::exists(m_filePath)) {
        return;
    }

    QFile file(m_filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }
    QByteArray currentData = file.readAll();
    file.close();

    QByteArray currentHash = QCryptographicHash::hash(currentData, QCryptographicHash::Sha256);
    if (currentHash == m_lastDiskHash) {
        m_lastDiskTime = QFileInfo(m_filePath).lastModified();
        return;
    }

    m_lastDiskHash = currentHash;
    m_lastDiskTime = QFileInfo(m_filePath).lastModified();
    emit fileChangedExternally(m_filePath);
}

} // namespace YANP
