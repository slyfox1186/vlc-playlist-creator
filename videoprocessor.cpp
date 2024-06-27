#include "videoprocessor.h"
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QDateTime>
#include <QUrl>
#include <QTextStream>
#include <algorithm>

VideoProcessor::VideoProcessor(const QString &directory, bool verbose, SortType sortType)
    : m_directory(directory), m_verbose(verbose), m_sortType(sortType) {
    QString logFileName = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + "_vlc_playlist_creator.log";
    m_logFile.setFileName(logFileName);
    m_logFile.open(QIODevice::WriteOnly | QIODevice::Text);
    m_logStream.setDevice(&m_logFile);
}

void VideoProcessor::process(const QString &outputPath) {
    log("Starting process");
    QDir dir(m_directory);
    if (!dir.exists()) {
        emit errorOccurred("Directory does not exist: " + m_directory);
        log("Error: Directory does not exist: " + m_directory);
        emit finished();
        return;
    }

    QStringList videoFiles = findVideoFiles(m_directory);
    if (videoFiles.isEmpty()) {
        emit errorOccurred("No video files found in directory: " + m_directory);
        log("Error: No video files found in directory: " + m_directory);
        emit finished();
        return;
    }
    
    log("Found " + QString::number(videoFiles.size()) + " video files");

    QString output = generatePlaylist(videoFiles, outputPath);

    emit outputGenerated(output);
    log("Process completed");
    emit finished();
}

void VideoProcessor::processManualPlaylist(const QStringList &filePaths, const QString &outputPath) {
    log("Starting manual playlist process");
    
    if (filePaths.isEmpty()) {
        emit errorOccurred("No files provided for manual playlist");
        log("Error: No files provided for manual playlist");
        emit finished();
        return;
    }
    
    log("Processing " + QString::number(filePaths.size()) + " files");

    QString output = generatePlaylist(filePaths, outputPath);

    emit outputGenerated(output);
    log("Manual playlist process completed");
    emit finished();
}

QString VideoProcessor::generatePlaylist(const QStringList &videoFiles, const QString &outputPath) {
    QList<QPair<QString, int>> videoQualityList;

    for (const QString &filePath : videoFiles) {
        log("Processing file: " + filePath);
        QString videoCodec = getVideoCodec(filePath);
        QString videoResolution = getVideoResolution(filePath);
        double videoBitrate = getVideoBitrate(filePath);
        QString audioCodec = getAudioCodec(filePath);
        int audioBitrate = getAudioBitrate(filePath);
        int duration = getVideoDuration(filePath);
        qint64 fileSize = getFileSize(filePath);

        int qualityScore = 0;
        qualityScore += videoCodec.contains("h264", Qt::CaseInsensitive) ? 10 : 5;
        qualityScore += videoResolution.contains("1920x1080") ? 20 : (videoResolution.contains("1280x720") ? 10 : 5);
        qualityScore += static_cast<int>(videoBitrate / 1000);
        qualityScore += audioCodec.contains("aac", Qt::CaseInsensitive) ? 10 : 5;
        qualityScore += audioBitrate / 32;
        qualityScore += duration / 60000; // Add points for longer videos (1 point per minute)
        qualityScore += static_cast<int>(fileSize / (1024 * 1024)); // Add points for larger files (1 point per MB)

        videoQualityList.append(qMakePair(filePath, qualityScore));
        log("File processed: " + filePath + ", Quality Score: " + QString::number(qualityScore));
    }

    sortVideoFiles(videoQualityList);

    QString output;
    QTextStream stream(&output);
    stream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    stream << "<playlist xmlns=\"http://xspf.org/ns/0/\" xmlns:vlc=\"http://www.videolan.org/vlc/playlist/ns/0/\" version=\"1\">\n";
    stream << "\t<title>Playlist</title>\n";
    stream << "\t<trackList>\n";

    int trackId = 0;
    for (const auto &video : videoQualityList) {
        stream << "\t\t<track>\n";

        QString filePath = QDir::toNativeSeparators(video.first);
        filePath.replace(0, 2, "C:");  // Replace 'c:' with 'C:'
        QString encodedPath = QUrl::toPercentEncoding(filePath, ":/");
        stream << "\t\t\t<location>file:///" << encodedPath << "</location>\n";

        int duration = getVideoDuration(video.first);
        stream << "\t\t\t<duration>" << duration << "</duration>\n";
        
        stream << "\t\t\t<extension application=\"http://www.videolan.org/vlc/playlist/0\">\n";
        stream << "\t\t\t\t<vlc:id>" << trackId << "</vlc:id>\n";
        stream << "\t\t\t</extension>\n";
        
        stream << "\t\t</track>\n";

        trackId++;
    }

    stream << "\t</trackList>\n";
    stream << "\t<extension application=\"http://www.videolan.org/vlc/playlist/0\">\n";
    
    for (int i = 0; i < trackId; i++) {
        stream << "\t\t<vlc:item tid=\"" << i << "\"/>\n";
    }
    
    stream << "\t</extension>\n";
    stream << "</playlist>\n";

    QFile playlistFile(outputPath);
    if (playlistFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream fileStream(&playlistFile);
        fileStream.setCodec("UTF-8");
        fileStream << output;
        playlistFile.close();
        log("Playlist saved to: " + outputPath);
    } else {
        emit errorOccurred("Failed to save playlist file: " + outputPath);
        log("Error: Failed to save playlist file: " + outputPath);
    }

    return output;
}

void VideoProcessor::sortVideoFiles(QList<QPair<QString, int>> &videoList) {
    switch (m_sortType) {
        case Quality:
            std::sort(videoList.begin(), videoList.end(),
                      [](const QPair<QString, int> &a, const QPair<QString, int> &b) {
                          return a.second > b.second;
                      });
            break;
        case Name:
            std::sort(videoList.begin(), videoList.end(),
                      [](const QPair<QString, int> &a, const QPair<QString, int> &b) {
                          return QFileInfo(a.first).fileName() < QFileInfo(b.first).fileName();
                      });
            break;
        case Duration:
            std::sort(videoList.begin(), videoList.end(),
                      [this](const QPair<QString, int> &a, const QPair<QString, int> &b) {
                          return getVideoDuration(a.first) > getVideoDuration(b.first);
                      });
            break;
        case Size:
            std::sort(videoList.begin(), videoList.end(),
                      [this](const QPair<QString, int> &a, const QPair<QString, int> &b) {
                          return getFileSize(a.first) > getFileSize(b.first);
                      });
            break;
        case NoSort:
        default:
            break;
    }
}

QStringList VideoProcessor::findVideoFiles(const QString &directory) {
    QStringList videoFiles;
    QDir dir(directory);
    QStringList videoExtensions = {"*.mp4", "*.avi", "*.mkv", "*.mov", "*.wmv", "*.flv", "*.webm"};
    
    QFileInfoList entries = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);
    for (const QFileInfo &entry : entries) {
        if (entry.isDir()) {
            videoFiles.append(findVideoFiles(entry.filePath()));
        } else if (entry.isFile()) {
            if (videoExtensions.contains("*." + entry.suffix().toLower())) {
                videoFiles.append(entry.filePath());
            }
        }
    }
    
    return videoFiles;
}

int VideoProcessor::getVideoDuration(const QString &filePath) {
    QProcess process;
    process.start("ffprobe", QStringList() << "-v" << "error" << "-show_entries" << "format=duration" 
                  << "-of" << "default=noprint_wrappers=1:nokey=1" << filePath);
    process.waitForFinished();
    bool ok;
    double duration = QString(process.readAllStandardOutput()).trimmed().toDouble(&ok);
    return ok ? static_cast<int>(duration * 1000) : 0; // Convert to milliseconds
}

QString VideoProcessor::getVideoCodec(const QString &filePath) {
    QProcess process;
    process.start("ffprobe", QStringList() << "-v" << "error" << "-select_streams" << "v:0" 
                  << "-show_entries" << "stream=codec_name" << "-of" << "default=noprint_wrappers=1:nokey=1" << filePath);
    process.waitForFinished();
    return QString(process.readAllStandardOutput()).trimmed();
}

QString VideoProcessor::getVideoResolution(const QString &filePath) {
    QProcess process;
    process.start("ffprobe", QStringList() << "-v" << "error" << "-select_streams" << "v:0"
                  << "-show_entries" << "stream=width,height" << "-of" << "csv=s=x:p=0" << filePath);
    process.waitForFinished();
    return QString(process.readAllStandardOutput()).trimmed();
}

double VideoProcessor::getVideoBitrate(const QString &filePath) {
    QProcess process;
    process.start("ffprobe", QStringList() << "-v" << "error" << "-select_streams" << "v:0"
                  << "-show_entries" << "stream=bit_rate" << "-of" << "default=noprint_wrappers=1:nokey=1" << filePath);
    process.waitForFinished();
    bool ok;
    double bitrate = QString(process.readAllStandardOutput()).trimmed().toDouble(&ok);
    return ok ? bitrate / 1000.0 : 0.0;
}

QString VideoProcessor::getAudioCodec(const QString &filePath) {
    QProcess process;
    process.start("ffprobe", QStringList() << "-v" << "error" << "-select_streams" << "a:0"
                  << "-show_entries" << "stream=codec_name" << "-of" << "default=noprint_wrappers=1:nokey=1" << filePath);
    process.waitForFinished();
    return QString(process.readAllStandardOutput()).trimmed();
}

int VideoProcessor::getAudioBitrate(const QString &filePath) {
    QProcess process;
    process.start("ffprobe", QStringList() << "-v" << "error" << "-select_streams" << "a:0"
                  << "-show_entries" << "stream=bit_rate" << "-of" << "default=noprint_wrappers=1:nokey=1" << filePath);
    process.waitForFinished();
    bool ok;
    int bitrate = QString(process.readAllStandardOutput()).trimmed().toInt(&ok);
    return ok ? bitrate / 1000 : 0;
}

qint64 VideoProcessor::getFileSize(const QString &filePath) {
    return QFileInfo(filePath).size();
}

QString VideoProcessor::getFileExtension(const QString &filePath) {
    return QFileInfo(filePath).suffix();
}

void VideoProcessor::log(const QString &message) {
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    m_logStream << timestamp << " - " << message << "\n";
    m_logStream.flush();
    emit logMessage(timestamp + " - " + message);
}