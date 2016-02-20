#ifndef DATAFILE_H
#define DATAFILE_H

#include <QFile>
#include <QFileDialog>
#include <QDataStream>
#include <QDateTime>
#include <QDebug>

#include "mainwindow.h"
#include "ui_mainwindow.h"

struct dataBattery {
    int iBatteryIndex; /// индекс батареи
    bool bIsUTTBB; /// признак УТТББ
    bool bIsImitator; /// признак имитатора
    QDate dateBuild; /// дата производства
    QString sNumber; /// номер батареи
};

#endif // DATAFILE_H

