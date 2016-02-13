#ifndef SETTINGS_H
#define SETTINGS_H

#include <QObject>

#define INI_FILE_NAME   "kds_hit.ini" // короткое имя файла

class Settings : public QObject
{
    Q_OBJECT
public:
    explicit Settings(QObject *parent = 0);

    void saveSettings();

    // общие переменные, которые настраиваются
    int num_batteries_types; ///< кол-во уникальных типов батарей
    float coefADC1; ///< коэффициент пересчёта кода АЦП в вольты
    float coefADC2; ///< коэффициент пересчёта кода АЦП в вольты
    float voltage_corpus_limit; ///< предельное напряжение на корпусе батареи, В
    float voltage_circuit_type; ///< минимальное напряжение для цепи при проверке подтипа батареи, В
    float voltage_power_uutbb; ///< минимальное напряжение БП УУТББ при проверке наличия телеметрии
    float isolation_resistance_limit; ///< предельное сопротивление изоляции батареи
    float opencircuitgroup_limit_min; ///< предельные напряжения разомкнутой цепи группы, вещественные, вольты
    float opencircuitgroup_limit_max; ///< предельные напряжения разомкнутой цепи группы, вещественные, вольты
    float opencircuitbattery_limit;  ///< предельное напряжение разомкнутой цепи батареи, вольты
    float closecircuitgroup_limit; ///< предельное напряжение замкнутой цепи группы, вольт
    float closecircuitbattery_limit; ///< предельное напряжение замкнутой цепи батареи, вольты

    int number_depassivation_stage; ///< кол-во ступеней распассивации
    float depassivation_current[3]; ///< токи нагрузки замкнутых цепей групп по ступеням, амперы  (!!! 3=number_discharge_stage)
    int time_depassivation[3]; ///< время по ступеням

    float uutbb_isolation_resist_limit; ///< предельное сопротивление изоляции платы измерительной УУТББ, МОм
    float uutbb_opencircuitpower_limit_min; ///< предельные напряжение разомкнутой цепи блока питания УУТББ, 7.05 +/- 0.15 вольт
    float uutbb_opencircuitpower_limit_max; ///<
    float uutbb_closecircuitpower_limit; ///< предельное напряжение замкнутой цепи блока питания УУТББ, при токе 0.1А, вольт
    int uutbb_time_ccp; ///< время подключения нагрузки на БП УУТББ, секунды

private:
    QString m_sSettingsFile; ///< полное имя ini-файла

signals:

public slots:
    void loadSettings(); ///< загрузить конфиг из ini-файла
};

#endif // SETTINGS_H
