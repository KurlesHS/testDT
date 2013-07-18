#-------------------------------------------------
#
# Project created by QtCreator 2013-05-13T10:00:05
#
#-------------------------------------------------

QT       += core gui network xml
QMAKE_CXXFLAGS += -std=c++11

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = TestDigitalTelevision
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    vplaymanager.cpp \
    addcommanddialog.cpp

HEADERS  += mainwindow.h \
    vplaymanager.h \
    addcommanddialog.h

FORMS    += mainwindow.ui \
    addcommanddialog.ui
