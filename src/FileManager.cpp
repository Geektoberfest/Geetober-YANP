#include "FileManager.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QStringDecoder>
#include <QStringEncoder>
#include <QSaveFile>
#include <QSettings>

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
namespace {

// Collapses every newline form to '\n' and re-applies the target line ending,
// so text coming from the editor (always '\n' separated) is written back with
// the line endings of the document being saved.
QString normalizeNewlines(const QString& content, LineEnding lineEnding) {
    QString normalized = content;
    normalized.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    normalized.replace(QStringLiteral("\r"), QStringLiteral("\n"));

    if (lineEnding == LineEnding::CRLF) {
        normalized.replace(QStringLiteral("\n"), QStringLiteral("\r\n"));
    }
    return normalized;
}

// Serializes already-normalized text using @p encoding. Shared by saveFile()
// and lossyCharacters() so the bytes that are checked for loss are exactly the
// bytes that reach the disk.
QByteArray encodeNormalized(const QString& normalized, FileEncoding encoding) {
    QByteArray data;
    switch (encoding) {
    case FileEncoding::UTF8_BOM: {
        data.append("\xEF\xBB\xBF");
        data.append(normalized.toUtf8());
        break;
    }
    case FileEncoding::UTF16_LE: {
        data.append("\xFF\xFE");
        auto encoder = QStringEncoder(QStringEncoder::Utf16LE);
        data.append(encoder(normalized));
        break;
    }
    case FileEncoding::UTF16_BE: {
        data.append("\xFE\xFF");
        auto encoder = QStringEncoder(QStringEncoder::Utf16BE);
        data.append(encoder(normalized));
        break;
    }
    case FileEncoding::SystemLocale: {
        data.append(normalized.toLocal8Bit());
        break;
    }
    case FileEncoding::UTF8:
    default: {
        data.append(normalized.toUtf8());
        break;
    }
    }
    return data;
}

} // namespace

FileManager::FileManager(QObject* parent)
    : QObject(parent)
{
#if defined(Q_OS_WIN)
    m_lineEnding = LineEnding::CRLF;
#else
    m_lineEnding = LineEnding::LF;
#endif
}

void FileManager::reset() {
    m_filePath.clear();
    m_isModified = false;
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

    result.success = true;
    result.content = decodedContent;

    recordRecentFile(m_filePath);

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

    // Normalize newlines to the document's line ending, then serialize with
    // the document's encoding. Both steps live in shared helpers so the lossy
    // detection can inspect exactly the bytes that are written below.
    const QByteArray encodedData =
        encodeNormalized(normalizeNewlines(content, m_lineEnding), m_encoding);

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

    result.success = true;
    result.content = content;

    recordRecentFile(m_filePath);

    emit modifiedChanged(false);
    emit fileChanged(m_filePath);

    return result;
}

QString FileManager::filePath() const {
    return m_filePath;
}

void FileManager::setFilePath(const QString& path) {
    m_filePath = path;
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

QStringList FileManager::lossyCharacters(const QString& content) const {
    // Every supported encoding except SystemLocale is a Unicode encoding, so
    // its round trip reproduces the input exactly.
    if (m_encoding != FileEncoding::SystemLocale) {
        return {};
    }

    const QString normalized = normalizeNewlines(content, m_lineEnding);
    const QByteArray encoded = encodeNormalized(normalized, m_encoding);
    const QString decoded = QString::fromLocal8Bit(encoded);
    return findLossyCharacters(normalized, decoded);
}

QStringList FileManager::findLossyCharacters(const QString& original,
                                             const QString& roundTripped) {
    QStringList lost;
    const qsizetype count = qMin(original.size(), roundTripped.size());

    for (qsizetype i = 0; i < count; ++i) {
        if (original.at(i) == roundTripped.at(i)) {
            continue;
        }

        // Report whole code points so surrogate pairs stay intact.
        const bool surrogatePair =
            i + 1 < count &&
            original.at(i).isHighSurrogate() &&
            original.at(i + 1).isLowSurrogate();
        const QString character = surrogatePair ? original.mid(i, 2)
                                                : QString(original.at(i));
        if (!lost.contains(character)) {
            lost.append(character);
        }
        if (surrogatePair) {
            ++i;
        }
    }

    // Characters the encoder dropped entirely are lost as well.
    for (qsizetype i = count; i < original.size(); ++i) {
        const QString character(original.at(i));
        if (!lost.contains(character)) {
            lost.append(character);
        }
    }

    return lost;
}

} // namespace YANP
