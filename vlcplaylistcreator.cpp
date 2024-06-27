#include "vlcplaylistcreator.h"
#include "videoprocessor.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QThread>
#include <QLabel>
#include <QComboBox>
#include <QStandardPaths>

VLCPlaylistCreator::VLCPlaylistCreator(QWidget *parent) 
    : QMainWindow(parent), m_settings("VLCPlaylistCreator", "VLCPlaylistCreator") {
    setWindowTitle("VLC Playlist Creator");

    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    setCentralWidget(centralWidget);

    m_mainTabWidget = new QTabWidget(this);
    mainLayout->addWidget(m_mainTabWidget);

    QWidget *processTab = new QWidget(this);
    QVBoxLayout *processLayout = new QVBoxLayout(processTab);

    QHBoxLayout *inputLayout = new QHBoxLayout();
    m_directoryInput = new QLineEdit(this);
    QPushButton *browseButton = new QPushButton("Browse", this);
    inputLayout->addWidget(m_directoryInput);
    inputLayout->addWidget(browseButton);
    processLayout->addLayout(inputLayout);

    m_verboseCheckbox = new QCheckBox("Verbose", this);
    processLayout->addWidget(m_verboseCheckbox);

    QHBoxLayout *sortLayout = new QHBoxLayout();
    QLabel *sortLabel = new QLabel("Sort by:", this);
    m_sortTypeComboBox = new QComboBox(this);
    m_sortTypeComboBox->addItem("No Sort", VideoProcessor::NoSort);
    m_sortTypeComboBox->addItem("Quality", VideoProcessor::Quality);
    m_sortTypeComboBox->addItem("Name", VideoProcessor::Name);
    m_sortTypeComboBox->addItem("Duration", VideoProcessor::Duration);
    m_sortTypeComboBox->addItem("Size", VideoProcessor::Size);
    sortLayout->addWidget(sortLabel);
    sortLayout->addWidget(m_sortTypeComboBox);
    processLayout->addLayout(sortLayout);

    QSplitter *splitter = new QSplitter(Qt::Vertical, this);
    processLayout->addWidget(splitter);

    m_outputTextEdit = new QTextEdit(this);
    m_outputTextEdit->setReadOnly(true);
    splitter->addWidget(m_outputTextEdit);

    m_logTextEdit = new QTextEdit(this);
    m_logTextEdit->setReadOnly(true);
    splitter->addWidget(m_logTextEdit);

    QPushButton *processButton = new QPushButton("Process", this);
    processLayout->addWidget(processButton);

    m_mainTabWidget->addTab(processTab, "Process Directory");

    // Manual Playlist Tab
    QWidget *manualTab = new QWidget(this);
    QVBoxLayout *manualLayout = new QVBoxLayout(manualTab);

    QHBoxLayout *manualInputLayout = new QHBoxLayout();
    m_videoInput = new QLineEdit(this);
    m_videoInput->setPlaceholderText("Enter video file path...");
    manualInputLayout->addWidget(m_videoInput);

    QPushButton *browseVideoButton = new QPushButton("Browse", this);
    manualInputLayout->addWidget(browseVideoButton);
    manualLayout->addLayout(manualInputLayout);

    QPushButton *addButton = new QPushButton("Add to Playlist", this);
    addButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    manualLayout->addWidget(addButton);

    m_displayTabWidget = new QTabWidget(this);
    m_playlist = new QListWidget(this);
    m_displayTabWidget->addTab(m_playlist, "Full Path");
    m_displayTabWidget->addTab(new QListWidget(this), "File Name");
    m_displayTabWidget->addTab(new QListWidget(this), "Parent Folder");

    manualLayout->addWidget(m_displayTabWidget);

    QHBoxLayout *manualButtonLayout = new QHBoxLayout();
    QPushButton *processManualButton = new QPushButton("Process", this);
    QPushButton *clearManualButton = new QPushButton("Clear Playlist", this);
    manualButtonLayout->addWidget(processManualButton);
    manualButtonLayout->addWidget(clearManualButton);
    manualLayout->addLayout(manualButtonLayout);

    m_mainTabWidget->addTab(manualTab, "Manual Playlist");

    // Create menu bar
    menuBar = new QMenuBar(this);
    fileMenu = new QMenu("File", this);
    addVideoAction = new QAction("Add Video", this);
    fileMenu->addAction(addVideoAction);
    menuBar->addMenu(fileMenu);
    setMenuBar(menuBar);

    connect(browseButton, &QPushButton::clicked, this, &VLCPlaylistCreator::browseDirectory);
    connect(processButton, &QPushButton::clicked, this, &VLCPlaylistCreator::processDirectory);
    connect(addButton, &QPushButton::clicked, this, &VLCPlaylistCreator::addVideoToPlaylist);
    connect(browseVideoButton, &QPushButton::clicked, this, &VLCPlaylistCreator::browseVideoFile);
    connect(addVideoAction, &QAction::triggered, this, &VLCPlaylistCreator::openAddVideoDialog);
    connect(m_displayTabWidget, &QTabWidget::currentChanged, this, &VLCPlaylistCreator::switchDisplayMode);
    connect(processManualButton, &QPushButton::clicked, this, &VLCPlaylistCreator::processManualPlaylist);
    connect(clearManualButton, &QPushButton::clicked, this, &VLCPlaylistCreator::clearManualPlaylist);

    resize(800, 600);
}

void VLCPlaylistCreator::browseDirectory() {
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::Directory);
    dialog.setOption(QFileDialog::ShowDirsOnly, true);
    dialog.setViewMode(QFileDialog::List);
    dialog.setDirectory(getLastDirectory());

    QList<QUrl> sidebarUrls = dialog.sidebarUrls();
    sidebarUrls.prepend(QUrl::fromLocalFile("/"));
    dialog.setSidebarUrls(sidebarUrls);

    if (dialog.exec()) {
        QStringList directories = dialog.selectedFiles();
        if (!directories.isEmpty()) {
            QString directory = directories.first();
            m_directoryInput->setText(directory);
            saveLastDirectory(directory);
        }
    }
}

void VLCPlaylistCreator::processDirectory() {
    QString directory = m_directoryInput->text();
    if (directory.isEmpty()) {
        QMessageBox::warning(this, "Error", "Please select a directory.");
        return;
    }

    QString outputPath = getOutputFilePath();
    if (outputPath.isEmpty()) {
        return;
    }

    bool verbose = m_verboseCheckbox->isChecked();
    VideoProcessor::SortType sortType = getCurrentSortType();

    QThread *thread = new QThread(this);
    VideoProcessor *processor = new VideoProcessor(directory, verbose, sortType);
    processor->moveToThread(thread);
    connect(thread, &QThread::started, [processor, outputPath]() {
        processor->process(outputPath);
    });
    connect(processor, &VideoProcessor::finished, thread, &QThread::quit);
    connect(processor, &VideoProcessor::finished, processor, &VideoProcessor::deleteLater);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    connect(processor, &VideoProcessor::outputGenerated, this, &VLCPlaylistCreator::updateOutput);
    connect(processor, &VideoProcessor::errorOccurred, this, &VLCPlaylistCreator::displayError);
    connect(processor, &VideoProcessor::logMessage, this, &VLCPlaylistCreator::appendLog);
    thread->start();

    m_outputTextEdit->clear();
    m_logTextEdit->clear();
    appendLog("Processing started...");
}

void VLCPlaylistCreator::processManualPlaylist() {
    if (m_videoPaths.isEmpty()) {
        QMessageBox::warning(this, "Error", "Please add videos to the playlist.");
        return;
    }

    QString outputPath = getOutputFilePath();
    if (outputPath.isEmpty()) {
        return;
    }

    VideoProcessor::SortType sortType = getCurrentSortType();

    QThread *thread = new QThread(this);
    VideoProcessor *processor = new VideoProcessor("", false, sortType);
    processor->moveToThread(thread);
    connect(thread, &QThread::started, [processor, this, outputPath]() {
        processor->processManualPlaylist(m_videoPaths, outputPath);
    });
    connect(processor, &VideoProcessor::finished, thread, &QThread::quit);
    connect(processor, &VideoProcessor::finished, processor, &VideoProcessor::deleteLater);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    connect(processor, &VideoProcessor::outputGenerated, this, &VLCPlaylistCreator::updateOutput);
    connect(processor, &VideoProcessor::errorOccurred, this, &VLCPlaylistCreator::displayError);
    connect(processor, &VideoProcessor::logMessage, this, &VLCPlaylistCreator::appendLog);
    thread->start();

    m_outputTextEdit->clear();
    m_logTextEdit->clear();
    appendLog("Processing manual playlist...");
}

void VLCPlaylistCreator::updateOutput(const QString &output) {
    m_outputTextEdit->setPlainText(output);
    QMessageBox::information(this, "Processing Complete", "VLC playlist creation completed.");
}

void VLCPlaylistCreator::displayError(const QString &error) {
    QMessageBox::critical(this, "Error", error);
    appendLog("Error: " + error);
}

void VLCPlaylistCreator::appendLog(const QString &message) {
    m_logTextEdit->append(message);
}

void VLCPlaylistCreator::openAddVideoDialog() {
    m_videoInput->setFocus();
}

void VLCPlaylistCreator::addVideoToPlaylist() {
    QString videoPath = m_videoInput->text();
    if (!videoPath.isEmpty()) {
        m_videoPaths.append(videoPath);
        updatePlaylistDisplay();
        m_videoInput->clear();
    } else {
        QMessageBox::warning(this, "Input Error", "Please enter a valid video file path.");
    }
}

void VLCPlaylistCreator::browseVideoFile() {
    QString videoPath = QFileDialog::getOpenFileName(this, "Select Video File", getLastDirectory());
    if (!videoPath.isEmpty()) {
        m_videoPaths.append(videoPath);
        updatePlaylistDisplay();
        saveLastDirectory(QFileInfo(videoPath).absolutePath());
    }
}

void VLCPlaylistCreator::switchDisplayMode(int /* index */) {
    updatePlaylistDisplay();
}

void VLCPlaylistCreator::updatePlaylistDisplay() {
    QListWidget *currentListWidget = qobject_cast<QListWidget *>(m_displayTabWidget->currentWidget());
    if (!currentListWidget) return;

    currentListWidget->clear();
    for (const QString &videoPath : qAsConst(m_videoPaths)) {
        switch (m_displayTabWidget->currentIndex()) {
            case 0: // Full Path
                currentListWidget->addItem(videoPath);
                break;
            case 1: // File Name
                currentListWidget->addItem(QFileInfo(videoPath).fileName());
                break;
            case 2: // Parent Folder
                currentListWidget->addItem(QFileInfo(videoPath).absolutePath());
                break;
        }
    }
}

QString VLCPlaylistCreator::getOutputFilePath() {
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setNameFilter("VLC Playlist (*.xspf)");
    dialog.setDefaultSuffix("xspf");
    dialog.setDirectory(getLastDirectory());

    QList<QUrl> sidebarUrls = dialog.sidebarUrls();
    sidebarUrls.prepend(QUrl::fromLocalFile("/"));
    dialog.setSidebarUrls(sidebarUrls);

    if (dialog.exec()) {
        QStringList files = dialog.selectedFiles();
        if (!files.isEmpty()) {
            QString outputPath = files.first();
            if (!outputPath.endsWith(".xspf", Qt::CaseInsensitive)) {
                outputPath += ".xspf";
            }
            saveLastDirectory(QFileInfo(outputPath).absolutePath());
            return outputPath;
        }
    }
    return QString();
}

void VLCPlaylistCreator::saveLastDirectory(const QString &path) {
    m_settings.setValue("lastDirectory", path);
    m_settings.sync();
}

QString VLCPlaylistCreator::getLastDirectory() {
    return m_settings.value("lastDirectory", QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).toString();
}

VideoProcessor::SortType VLCPlaylistCreator::getCurrentSortType() {
    return static_cast<VideoProcessor::SortType>(m_sortTypeComboBox->currentData().toInt());
}

void VLCPlaylistCreator::clearManualPlaylist() {
    m_videoPaths.clear();
    updatePlaylistDisplay();
    appendLog("Manual playlist cleared.");
}