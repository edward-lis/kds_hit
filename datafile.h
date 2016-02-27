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
    int icbClosedCircuitVoltagePowerSupply; //// индекс "Напряжение замкнутой цепи БП"
    QList<QVariant> itemsOpenCircuitVoltageGroup; //// состояние чекбоксов "Напряжение разомкнутой цепи группы"
    QList<QVariant> itemsClosedCircuitVoltageGroup; //// состояние чекбоксов "Напряжение замкнутой цепи группы"
    QList<QVariant> itemsDepassivation; //// состояние чекбоксов "Распассивация"
    QList<QVariant> itemsInsulationResistanceUUTBB; //// состояние чекбоксов "Сопротивления изоляции УУТББ"
    QList<double> dArrayVoltageOnTheHousing;
    QList<double> dArrayInsulationResistance;
    QList<double> dArrayOpenCircuitVoltageGroup;
    QList<double> dArrayOpenCircuitVoltageBattery;
    QList<double> dArrayClosedCircuitVoltageGroup;
    QList<double> dArrayDepassivation;
    QList<double> dArrayClosedCircuitVoltageBattery;
    QList<double> dArrayInsulationResistanceUUTBB;
    QList<double> dArrayOpenCircuitVoltagePowerSupply;
    QList<double> dArrayClosedCircuitVoltagePowerSupply;
};

#endif // DATAFILE_H

