QT += core gui widgets

CONFIG += c++11

TARGET = vlc-playlist-creator
TEMPLATE = app

SOURCES += \
    main.cpp \
    vlcplaylistcreator.cpp \
    videoprocessor.cpp

HEADERS += \
    vlcplaylistcreator.h \
    videoprocessor.h
