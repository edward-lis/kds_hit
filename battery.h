/*
 * Описание объекта батареи
*/

#ifndef BATTERY_H
#define BATTERY_H
#include <QString>

class Battery
{
public:
    Battery();
    // количество групп/цепей groupe/circle
    int group_num;
    // тип батареи
    QString type_name;
    // точки измерения напряжения на корпусе батареи
    QString voltage_corpus_1, voltage_corpus_2;
    // точки измерения сопротивления изоляции
    QString isolation_resistance_1, isolation_resistance_2, isolation_resistance_3, isolation_resistance_4;
};

#endif // BATTERY_H
