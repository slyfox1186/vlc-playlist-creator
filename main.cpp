#include "vlcplaylistcreator.h"
#include <QApplication>
#include <QDir>
#include <QStandardPaths>
#include <QProcess>

int main(int argc, char *argv[]) {
    // Set a custom XDG_RUNTIME_DIR
    QString customRuntimeDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + 
                               "/qt-runtime-" + 
                               QString::number(QCoreApplication::applicationPid());
    QDir().mkpath(customRuntimeDir);
    qputenv("XDG_RUNTIME_DIR", customRuntimeDir.toUtf8());

    // Ensure the directory has the correct permissions
    QProcess::execute("chmod", QStringList() << "700" << customRuntimeDir);

    QApplication app(argc, argv);
    VLCPlaylistCreator window;
    window.show();
    return app.exec();
}
