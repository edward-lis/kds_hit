#ifndef DATAFILE_H
#define DATAFILE_H

#include <QFile>
#include <QFileDialog>
#include <QDataStream>
#include <QDateTime>
#include <QPrinter>
#include <QDebug>

#include "mainwindow.h"
#include "ui_mainwindow.h"

struct dataBattery {
    int iBatteryIndex; /// индекс батареи
    bool bIsUTTBB; /// признак УТТББ
    bool bIsImitator; /// признак имитатора
    QDate dateBuild; /// дата производства
    QString sNumber; /// номер батареи
    bool bModeDiagnosticAuto; /// признак автоматического режима диагностики
    bool bModeDiagnosticManual; /// признак ручного режима диагностики
    int icbParamsAutoMode; /// индекс комбокса параметра проверки в автоматического режима
    int icbSubParamsAutoMode; /// индекс комбокса подпараметра проверки в автоматического режима
    int icbVoltageOnTheHousing; //// индекс комбкса "Напряжения на корпусе"
    int icbInsulationResistance; //// индекс комбокса "Сопротивления изоляции"
    //QStandardItemModel *modelClosedCircuitVoltageGroup;
    QList<int> imDepassivation;
    int icbDepassivation; //// индекс комбокса "Расспасивация"
    int icbClosedCircuitVoltagePowerSupply; //// индекс "Напряжение замкнутой цепи БП"
};

#endif // DATAFILE_H

