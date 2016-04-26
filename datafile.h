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
    QList<QVariant> itemsVoltageOnTheHousing; //// состояние чекбоксов "Напряжения на корпусе"
    QList<QVariant> itemsInsulationResistance; //// состояние чекбоксов "Сопротивления изоляции"
    int icbClosedCircuitVoltagePowerSupply; //// индекс "Напряжение замкнутой цепи БП"
    QList<QVariant> itemsOpenCircuitVoltageGroup; //// состояние чекбоксов "Напряжение разомкнутой цепи группы"
    QList<QVariant> itemsClosedCircuitVoltageGroup; //// состояние чекбоксов "Напряжение замкнутой цепи группы"
    QList<QVariant> itemsDepassivation; //// состояние чекбоксов "Распассивация"
    QList<QVariant> itemsInsulationResistanceUUTBB; //// состояние чекбоксов "Сопротивления изоляции УУТББ"
    QList<double> dArrayVoltageOnTheHousing; /// масив текущих значений проверок
    QList<double> dArrayInsulationResistance; /// масив текущих значений проверок
    QList<double> dArrayOpenCircuitVoltageGroup; /// масив текущих значений проверок
    QList<double> dArrayOpenCircuitVoltageBattery; /// масив текущих значений проверок
    QList<double> dArrayClosedCircuitVoltageGroup; /// масив текущих значений проверок
    QList<double> dArrayDepassivation; /// масив текущих значений проверок
    QList<double> dArrayClosedCircuitVoltageBattery; /// масив текущих значений проверок
    QList<double> dArrayInsulationResistanceUUTBB; /// масив текущих значений проверок
    QList<double> dArrayOpenCircuitVoltagePowerSupply; /// масив текущих значений проверок
    QList<double> dArrayClosedCircuitVoltagePowerSupply; /// масив текущих значений проверок
    QList<QString> sArrayReportVoltageOnTheHousing; /// масив выполненых проверк для отчета
    QList<QString> sArrayReportInsulationResistance; /// масив выполненых проверк для отчета
    QList<QString> sArrayReportOpenCircuitVoltageGroup; /// масив выполненых проверк для отчета
    QList<QString> sArrayReportOpenCircuitVoltageBattery; /// масив выполненых проверк для отчета
    QList<QString> sArrayReportClosedCircuitVoltageGroup; /// масив выполненых проверк для отчета
    QList<QString> sArrayReportDepassivation; /// масив выполненых проверк для отчета
    QList<QString> sArrayReportClosedCircuitVoltageBattery; /// масив выполненых проверк для отчета
    QList<QString> sArrayReportInsulationResistanceUUTBB; /// масив выполненых проверк для отчета
    QList<QString> sArrayReportOpenCircuitVoltagePowerSupply; /// масив выполненых проверк для отчета
    QList<QString> sArrayReportClosedCircuitVoltagePowerSupply; /// масив выполненых проверк для отчета
    QList<QString> sArrayReportGraphDescription; /// масив описания графиков
    QList<QImage> imgArrayReportGraph; /// масив графиков
};

#endif // DATAFILE_H

