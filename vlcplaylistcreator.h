#ifndef VLCPLAYLISTCREATOR_H
#define VLCPLAYLISTCREATOR_H

#include <QMainWindow>
#include <QLineEdit>
#include <QCheckBox>
#include <QTextEdit>

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

private:
    QLineEdit *m_directoryInput;
    QCheckBox *m_verboseCheckbox;
    QCheckBox *m_orderByQualityCheckbox;
    QTextEdit *m_outputTextEdit;
    QTextEdit *m_logTextEdit;
};

#endif // VLCPLAYLISTCREATOR_H
