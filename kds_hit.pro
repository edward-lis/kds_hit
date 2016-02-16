#-------------------------------------------------
#
# Project created by QtCreator 2016-01-19T17:06:49
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets serialport printsupport

TARGET = kds_hit
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    serialport.cpp \
    checkbatterytype.cpp \
    qcustomplot.cpp \
    settings.cpp \
    voltagecase.cpp \
    openciruitgroup.cpp \
    insulationresistance.cpp \
    insulationresistanceuutbb.cpp

HEADERS  += mainwindow.h \
    serialport.h \
    qcustomplot.h \
    settings.h \
    battery.h

FORMS    += mainwindow.ui

DISTFILES += \
    AGN_kds_prot09_xit_15.txt \
    kds_hit.ini
