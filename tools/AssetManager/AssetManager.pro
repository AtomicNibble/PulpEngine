#-------------------------------------------------
#
# Project created by QtCreator 2016-04-23T19:54:55
#
#-------------------------------------------------

QT       += core gui sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = AssetManager
TEMPLATE = app


SOURCES += main.cpp\
        assetmanager.cpp \
    assetdbnodes.cpp \
    assert.cpp \
    assetdbmodel.cpp \
    assetdbwidget.cpp \
    session.cpp \
    project.cpp \
    assetdbexplorer.cpp

HEADERS  += assetmanager.h \
    assetdbnodes.h \
    assert.h \
    assetdbmodel.h \
    assetdbwidget.h \
    session.h \
    project.h \
    assetdbexplorer.h

FORMS    += assetmanager.ui


INCLUDEPATH += "../../"


contains(QT_ARCH, i386) {
   Release:DESTDIR = ../../../../build/win32/Release
   Debug:DESTDIR = ../../../../build/win32/Debug
} else {
   Release:DESTDIR = ../../../../build/x64/Release
   Debug:DESTDIR = ../../../../build/x64/Debug
}
