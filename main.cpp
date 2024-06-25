#include <QApplication>
#include "vlcplaylistcreator.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    VLCPlaylistCreator window;
    window.setWindowTitle("VLC Playlist Creator");
    window.resize(800, 600);
    window.show();

    return app.exec();
}
