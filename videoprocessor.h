#ifndef VIDEOPROCESSOR_H
#define VIDEOPROCESSOR_H

#include <QObject>
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QStringList>

class VideoProcessor : public QObject {
    Q_OBJECT

public:
    VideoProcessor(const QString &directory, bool verbose, bool orderByQuality);

public slots:
    void process();
    void processManualPlaylist(const QStringList &filePaths);

signals:
    void outputGenerated(const QString &output);
    void errorOccurred(const QString &error);
    void logMessage(const QString &message);
    void finished();

private:
    QString m_directory;
    bool m_verbose;
    bool m_orderByQuality;
    QFile m_logFile;
    QTextStream m_logStream;
    
    QStringList findVideoFiles(const QString &directory);
    int getVideoDuration(const QString &filePath);
    QString getVideoCodec(const QString &filePath);
    QString getVideoResolution(const QString &filePath);
    double getVideoBitrate(const QString &filePath);
    QString getAudioCodec(const QString &filePath);
    int getAudioBitrate(const QString &filePath);
    QString getFileExtension(const QString &filePath);
    void log(const QString &message);
    QString generatePlaylist(const QStringList &videoFiles);
};

#endif // VIDEOPROCESSOR_H