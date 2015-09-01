#-------------------------------------------------
#
# Project created by QtCreator 2015-07-14T12:12:01
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = kds_hit
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    settings.cpp \
    battery.cpp

HEADERS  += mainwindow.h \
    settings.h \
    battery.h

FORMS    += mainwindow.ui \
    batterydata.ui \
    closedcircuitbattery.ui \
    closedcircuitgroup.ui \
    closedcircuitgroup20.ui \
    closedcircuitgroup24.ui \
    closedcircuitgroup28.ui \
    comportwidget.ui \
    depassivation.ui \
    dialogchoosedevice.ui \
    formcomport.ui \
    insulationresistance.ui \
    opencircuitbattery.ui \
    opencircuitgroup.ui \
    opencircuitgroup20.ui \
    opencircuitgroup24.ui \
    opencircuitgroup28.ui \
    thermometer.ui
