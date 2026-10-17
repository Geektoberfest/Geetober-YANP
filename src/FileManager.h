#pragma once

#include <QString>
#include <QStringList>
#include <QByteArray>
#include <QObject>

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

    // Characters of @p content that the current encoding cannot represent and
    // would therefore replace with a placeholder during a save. Empty when
    // saving @p content round-trips losslessly.
    QStringList lossyCharacters(const QString& content) const;

    // Round-trip comparison behind lossyCharacters(). Exposed so the check can
    // be unit tested without depending on the machine's locale.
    static QStringList findLossyCharacters(const QString& original,
                                           const QString& roundTripped);

signals:
    void modifiedChanged(bool modified);
    void fileChanged(const QString& filePath);
    void recentFilesChanged();

private:
    QString m_filePath;
    bool m_isModified = false;
    FileEncoding m_encoding = FileEncoding::UTF8;
    LineEnding m_lineEnding = LineEnding::LF;
};

} // namespace YANP
