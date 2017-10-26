#-------------------------------------------------
#
# Project created by QtCreator 2017-05-04T19:25:21
#
#-------------------------------------------------

QT       += core gui serialport printsupport multimedia sql concurrent

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

#CONFIG(release,debug|release){
#    DEFINES += QT_NO_DEBUG_OUTPUT
#}

TARGET = SensorCamellia
TEMPLATE = app

LIBS += -lWS2_32

SOURCES += main.cpp\
        mainwindow.cpp \
    nodeparam.cpp \
    sensorglobal.cpp \
    calibrationDisplay/calibrationdisplay.cpp \
    components/qcustomplot.cpp \
    customframe/headerview.cpp \
    customframe/tableviewmodel.cpp \
    customframe/nofocusstyle.cpp \
    dboperate.cpp \
    calibrationDisplay/originaldatadisplay.cpp \
    components/ccalendarwidget.cpp \
    components/cdatetimebutton.cpp \
    customframe/itemdelegate.cpp \
    components/ccalendarwnd.cpp \
    customframe/standarditemmodel.cpp \
    customframe/standarditem.cpp

HEADERS  += mainwindow.h \
    nodeparam.h \
    sensorglobal.h \
    matlab/fitting.h \
    calibrationDisplay/calibrationdisplay.h \
    components/qcustomplot.h \
    customframe/headerview.h \
    customframe/tableviewmodel.h \
    customframe/nofocusstyle.h \
    dboperate.h \
    calibrationDisplay/originaldatadisplay.h \
    components/ccalendarwidget.h \
    components/cdatetimebutton.h \
    customframe/itemdelegate.h \
    components/ccalendarwnd.h \
    customframe/standarditemmodel.h \
    customframe/standarditem.h

FORMS    += mainwindow.ui \
    calibrationDisplay/calibrationdisplay.ui \
    calibrationDisplay/originaldatadisplay.ui \
    components/ccalendarwidget.ui

RC_ICONS = $$PWD/resource/softico.ico

RESOURCES += \
    $$PWD/resource/resource.qrc
