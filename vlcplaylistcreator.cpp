#include "vlcplaylistcreator.h"
#include "videoprocessor.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QThread>

VLCPlaylistCreator::VLCPlaylistCreator(QWidget *parent) : QMainWindow(parent) {
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
    m_orderByQualityCheckbox = new QCheckBox("Order by Quality", this);
    processLayout->addWidget(m_verboseCheckbox);
    processLayout->addWidget(m_orderByQualityCheckbox);

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

    resize(800, 600);
}

void VLCPlaylistCreator::browseDirectory() {
    QString directory = QFileDialog::getExistingDirectory(this, "Select Directory");
    if (!directory.isEmpty()) {
        m_directoryInput->setText(directory);
    }
}

void VLCPlaylistCreator::processDirectory() {
    QString directory = m_directoryInput->text();
    if (directory.isEmpty()) {
        QMessageBox::warning(this, "Error", "Please select a directory.");
        return;
    }

    bool verbose = m_verboseCheckbox->isChecked();
    bool orderByQuality = m_orderByQualityCheckbox->isChecked();

    QThread *thread = new QThread(this);
    VideoProcessor *processor = new VideoProcessor(directory, verbose, orderByQuality);
    processor->moveToThread(thread);
    connect(thread, &QThread::started, processor, &VideoProcessor::process);
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
    QString videoPath = QFileDialog::getOpenFileName(this, "Select Video File");
    if (!videoPath.isEmpty()) {
        m_videoPaths.append(videoPath);
        updatePlaylistDisplay();
    }
}

void VLCPlaylistCreator::switchDisplayMode(int) {
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
