#ifndef BATTERY_H
#define BATTERY_H

#include <QString>

struct Battery
{
    /// количество групп/цепей groupe/circle
    int group_num;
    /// тип батареи
    QString str_type_name;
    /// признак того, что не натуральная батарея, а имитатор
    bool simulator;
    /// точки измерения напряжения на корпусе батареи
    QString str_voltage_corpus[2];
    /// точки измерения сопротивления изоляции
    QString str_isolation_resistance[4];
    /// точки измерения цепей групп
    QString circuitgroup[28];
    /// точка измерения цепи батареи
    QString circuitbattery;
    /// признак того, что эта батарея имеет плату измерительную УУТББ
    bool uutbb;
    /// точки измерений сопротивления изоляции платы УУТББ
    QVector<QString> uutbb_resist;
    /// кол-во точек измерения сопротивления изоляции платы УУТББ
    int uutbb_resist_num;

};

#endif // BATTERY_H

