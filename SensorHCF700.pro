#-------------------------------------------------
#
# Project created by QtCreator 2017-05-04T19:25:21
#
#-------------------------------------------------

QT       += core gui serialport printsupport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

CONFIG(release,debug|release){
DEFINES += QT_NO_DEBUG_OUTPUT
}

TARGET = SensorHCF700Calibration
TEMPLATE = app

LIBS += -lWS2_32

SOURCES += main.cpp\
        mainwindow.cpp \
    nodeparam.cpp \
    sensorglobal.cpp \
    calibrationDisplay/calibrationdisplay.cpp \
    calibrationDisplay/qcustomplot.cpp

HEADERS  += mainwindow.h \
    nodeparam.h \
    sensorglobal.h \
    fitting.h \
    calibrationDisplay/calibrationdisplay.h \
    calibrationDisplay/qcustomplot.h

FORMS    += mainwindow.ui \
    calibrationDisplay/calibrationdisplay.ui

RC_ICONS = softico.ico
