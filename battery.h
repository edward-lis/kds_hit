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
    float voltage_corpus_limit; ///< предельное напряжение на корпусе батареи, В
    /// точки измерения сопротивления изоляции
    QString str_isolation_resistance_1, str_isolation_resistance_2, str_isolation_resistance_3, str_isolation_resistance_4;
    float isolation_resistance_limit; ///< предельное сопротивление изоляции, МОм

    // !!! сделать массивом, а не отдельными строками
    QString opencircuitgroup_1, opencircuitgroup_2, opencircuitgroup_3, opencircuitgroup_4, opencircuitgroup_5, opencircuitgroup_6,
    opencircuitgroup_7, opencircuitgroup_8, opencircuitgroup_9, opencircuitgroup_10, opencircuitgroup_11, opencircuitgroup_12, opencircuitgroup_13,
    opencircuitgroup_14, opencircuitgroup_15, opencircuitgroup_16, opencircuitgroup_17, opencircuitgroup_18, opencircuitgroup_19, opencircuitgroup_20,
    opencircuitgroup_21, opencircuitgroup_22, opencircuitgroup_23, opencircuitgroup_24, opencircuitgroup_25, opencircuitgroup_26, opencircuitgroup_27,
    opencircuitgroup_28; ///< точки измерения цепей групп
    float opencircuitgroup_limit_min, opencircuitgroup_limit_max; ///< пределы напряжения разомкнутых цепей групп

    float closecircuitgroup_limit_01a, closecircuitgroup_limit_05a, closecircuitgroup_limit_10a; ///< предельные напряжения замкнутой цепи группы при токах 0.1, 0.5 и 1.0 ампер
    int number_discharge_stage; ///< кол-во ступеней проверки замкнутых цепей групп под нагрузкой
    float disharge_current_1, disharge_current_2, disharge_current_3; ///< токи нагрузки замкнутых цепей групп по ступеням, амперы
    int time_uccg_1_1; ///< время первой проверки на первой ступени, секунды
    int time_uccg_1; ///< продолжительность первой ступени, секунды (15 минут = 60*15 = 900)
    int time_uccg_2_1; ///< время первой проверки на второй ступени, секунды
    int time_uccg_2; ///< продолжительность второй ступени, секунды (5 минут = 60*5 = 300)
    int time_uccg_3_1; ///< время первой проверки на третьей  ступени, секунды
    int time_uccg_3; ///< продолжительность третьей  ступени, секунды (1 минута)

    QString str_closecircuitbattery; ///< точка измерения замкнутой цепи батареи (она же и открытой?)
    float discharge_current_battery; ///< ток проверки напряжения замкнутой цепи батареи, ампер
    int time_uccb; ///< время проверки напряжения ЗЦб после начала подачи нагрузки, секунды
    float closecircuitbattery_limit;  ///< предельное напряжение замкнутой цепи батареи, вольты

    bool uutbb; ///< признак того, что эта батарея имеет плату измерительную УУТББ
    // точки измерения изоляции платы УУТББ
    // [9ER20P_20_v2] - 38 штук. [9ER14PS_24_v2] - 28 штук.  кол-во и состав устаканить с Григорием. !!!
    //!!! QString str_uutbb_0-38
    // добавить строки для УУТБ, ввод их из ини-файла.
    // нарисовать на форме лабели для УУТББ

    float uutbb_isolation_resist_limit; ///< предельное сопротивление изоляции платы измерительной УУТББ, МОм
    /// предельные напряжение разомкнутой цепи блока питания УУТББ, 7.05 +/- 0.15 вольт
    float opencircuitpower_limit_min;
    float opencircuitpower_limit_max;
    float closecircuitpower_limit; ///< предельное напряжение замкнутой цепи блока питания УУТББ, при токе 0.1А, вольт
    int time_ccp; ///< время подключения нагрузки на БП УУТББ, секунды
};

#endif // BATTERY_H

