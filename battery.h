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
    QVector<QString> str_isolation_resistance;
    /// протокольные номера точек измерения изоляции
    QVector<int> isolation_resistance_nn;
    /// кол-во точек измерения сопротивления изоляции
    int i_isolation_resistance_num;
    /// точки измерения цепей групп
    QString circuitgroup[28];
    /// точка измерения цепи батареи
    QString circuitbattery;
    /// признак того, что эта батарея имеет плату измерительную УУТББ
    bool uutbb;
    /// точки измерений сопротивления изоляции платы УУТББ
    QVector<QString> uutbb_resist;
    /// протокольные номера точек измерения изоляции УУТББ
    QVector<int> uutbb_resist_nn;
    /// кол-во точек измерения сопротивления изоляции платы УУТББ
    int i_uutbb_resist_num;
    /// точки измерения напряжения цепи БП УУТББ (точка, в принципе, одна.  только вторая - с контролем тока нагрузки)
    QString uutbb_closecircuitpower[2];

};

#endif // BATTERY_H

