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
    assetdbmodel.cpp \
    assetdbwidget.cpp \
    session.cpp \
    project.cpp \
    assetdbexplorer.cpp \
    assetdb.cpp \
    logging.cpp \
    modprojectnodes.cpp \
    modproject.cpp \
    assert_qt.cpp

HEADERS  += assetmanager.h \
    assetdbnodes.h \
    assetdbmodel.h \
    assetdbwidget.h \
    session.h \
    project.h \
    assetdbexplorer.h \
    assetdb.h \
    logging.h \
    modprojectnodes.h \
    modproject.h \
    stdafx.h \
    assert_qt.h

FORMS    += assetmanager.ui


INCLUDEPATH += "../../"
INCLUDEPATH += "../../Pulp/Common"

QMAKE_CXXFLAGS += -std:c++latest

# precompressed header, used for engine headers.
CONFIG += precompile_header
PRECOMPILED_HEADER = stdafx.h

contains(QT_ARCH, i386) {
   Release:DESTDIR = ../../../../build/win32/Release
   Debug:DESTDIR = ../../../../build/win32/Debug
} else {
   Release:DESTDIR = ../../../../build/x64/Release
   Debug:DESTDIR = ../../../../build/x64/Debug
}

RESOURCES += \
    resources.qrc
