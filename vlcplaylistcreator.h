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

private:
    QLineEdit *m_directoryInput;
    QCheckBox *m_verboseCheckbox;
    QCheckBox *m_orderByQualityCheckbox;
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

    void updatePlaylistDisplay();
};

#endif // VLCPLAYLISTCREATOR_H
