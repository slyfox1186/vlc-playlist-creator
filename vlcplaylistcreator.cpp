#include "vlcplaylistcreator.h"
#include "videoprocessor.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QThread>
#include <QSplitter>

VLCPlaylistCreator::VLCPlaylistCreator(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("VLC Playlist Creator");
    
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    setCentralWidget(centralWidget);

    QHBoxLayout *inputLayout = new QHBoxLayout();
    m_directoryInput = new QLineEdit(this);
    QPushButton *browseButton = new QPushButton("Browse", this);
    inputLayout->addWidget(m_directoryInput);
    inputLayout->addWidget(browseButton);
    mainLayout->addLayout(inputLayout);

    m_verboseCheckbox = new QCheckBox("Verbose", this);
    m_orderByQualityCheckbox = new QCheckBox("Order by Quality", this);
    mainLayout->addWidget(m_verboseCheckbox);
    mainLayout->addWidget(m_orderByQualityCheckbox);

    QSplitter *splitter = new QSplitter(Qt::Vertical, this);
    mainLayout->addWidget(splitter);

    m_outputTextEdit = new QTextEdit(this);
    m_outputTextEdit->setReadOnly(true);
    splitter->addWidget(m_outputTextEdit);

    m_logTextEdit = new QTextEdit(this);
    m_logTextEdit->setReadOnly(true);
    splitter->addWidget(m_logTextEdit);

    QPushButton *processButton = new QPushButton("Process", this);
    mainLayout->addWidget(processButton);

    connect(browseButton, &QPushButton::clicked, this, &VLCPlaylistCreator::browseDirectory);
    connect(processButton, &QPushButton::clicked, this, &VLCPlaylistCreator::processDirectory);

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
