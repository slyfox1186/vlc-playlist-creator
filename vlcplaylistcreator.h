#ifndef VLCPLAYLISTCREATOR_H
#define VLCPLAYLISTCREATOR_H

#include <QMainWindow>
#include <QLineEdit>
#include <QCheckBox>
#include <QTextEdit>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QListWidget>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QSplitter>
#include <QSettings>
#include <QComboBox>
#include "videoprocessor.h"

class VLCPlaylistCreator : public QMainWindow {
    Q_OBJECT

public:
    VLCPlaylistCreator(QWidget *parent = nullptr);

private slots:
    void browseDirectory();
    void processDirectory();
    void updateOutput(const QString &output);
    void displayError(const QString &error);
    void appendLog(const QString &message);
    void openAddVideoDialog();
    void addVideoToPlaylist();
    void browseVideoFile();
    void switchDisplayMode(int index);
    void processManualPlaylist();
    void clearManualPlaylist();

private:
    QLineEdit *m_directoryInput;
    QCheckBox *m_verboseCheckbox;
    QComboBox *m_sortTypeComboBox;
    QTextEdit *m_outputTextEdit;
    QTextEdit *m_logTextEdit;
    QLineEdit *m_videoInput;
    QListWidget *m_playlist;
    QTabWidget *m_mainTabWidget;
    QTabWidget *m_displayTabWidget;
    QMenuBar *menuBar;
    QMenu *fileMenu;
    QAction *addVideoAction;
    QStringList m_videoPaths;
    QSettings m_settings;

    void updatePlaylistDisplay();
    QString getOutputFilePath();
    void saveLastDirectory(const QString &path);
    QString getLastDirectory();
    VideoProcessor::SortType getCurrentSortType();
};

#endif // VLCPLAYLISTCREATOR_H