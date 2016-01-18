#include "settings.h"
#include <QDebug>
#include <QTextCodec>
#include "battery.h"

extern Battery simulator, b_9ER20P_20, b_9ER20P_20_v2, b_9ER14PS_24, b_9ER14PS_24_v2, b_9ER20P_28, b_9ER14P_24;

Settings::Settings(QObject *parent) : QObject(parent)
{
    qDebug() << "App path : " << qApp->applicationDirPath() << "/" << INI_FILE_NAME; // возвращает путь к папке с исполняемым файлом и ini-файлу
    m_sSettingsFile = qApp->applicationDirPath() + "/" + INI_FILE_NAME;
    //QSettings settings(m_sSettingsFile, QSettings::NativeFormat);

}

void Settings::saveSettings()
{
    QSettings settings(m_sSettingsFile, QSettings::IniFormat);
    settings.setValue("battery/text", "sText");
    settings.sync();
}

void Settings::loadSettings()
{
    QSettings settings(m_sSettingsFile, QSettings::IniFormat);
    settings.setIniCodec("Windows-1251"); // т.к. мы в винде, то устанавливаем кодек для ini-файла
    QTextCodec *codec = QTextCodec::codecForName("cp866"); // берём кодек
    QTextCodec::setCodecForLocale(codec); // и устанавливаем его для локали, чтобы корректно показывал qDebug() русский ANSI шрифт из 1251 файла в 866 локали

    //QString sText = settings.value("battery/text", "").toString();
    //testString = sText;
    //qDebug() << "sText" << testString;

    // get battaries names, as array
    //int size = settings.beginReadArray("batteries");  // добавляет преффикс к текущей группе, и начинает чтение из массива. возвращает размер массива
    settings.beginReadArray("batteries");
    settings.setArrayIndex(0);
    simulator.str_type_name = settings.value("name").toString();
    settings.setArrayIndex(1);
    b_9ER20P_20.str_type_name = settings.value("name").toString();
    settings.setArrayIndex(2);
    b_9ER20P_20_v2.str_type_name = settings.value("name").toString();
    settings.setArrayIndex(3);
    b_9ER14PS_24.str_type_name = settings.value("name").toString();
    settings.setArrayIndex(4);
    b_9ER14PS_24_v2.str_type_name = settings.value("name").toString();
    settings.setArrayIndex(5);
    b_9ER20P_28.str_type_name = settings.value("name").toString();
    settings.setArrayIndex(6);
    b_9ER14P_24.str_type_name = settings.value("name").toString();
    settings.endArray(); // end array, don't forget!

    // value(,): первый параметр - ключ. второй параметр - параметр по умолчанию (допустим, если нет запрашиваемого ключа в тексте файла)
    // ключ - секция/ключ.   если без секции, то секция по умолчанию [General]

    // int - кол-во цепей/групп
    simulator.group_num=settings.value("simulator/group_num",0).toInt();
    b_9ER20P_20.group_num=settings.value("9ER20P_20/group_num",0).toInt();
    b_9ER20P_20_v2.group_num=settings.value("9ER20P_20_v2/group_num",0).toInt();
    b_9ER14PS_24.group_num=settings.value("9ER14PS_24/group_num",0).toInt();
    b_9ER14PS_24_v2.group_num=settings.value("9ER14PS_24_v2/group_num",0).toInt();
    b_9ER20P_28.group_num=settings.value("9ER20P_28/group_num",0).toInt();
    b_9ER14P_24.group_num=settings.value("9ER14P_24/group_num",0).toInt();

    // строки - точки измерения напряжения на корпусе
    simulator.str_voltage_corpus_1 = settings.value("simulator/voltage_corpus_1", "").toString();
    simulator.str_voltage_corpus_2 = settings.value("simulator/voltage_corpus_2", "").toString();
    b_9ER20P_20.str_voltage_corpus_1 = settings.value("9ER20P_20/voltage_corpus_1", "").toString();
    b_9ER20P_20.str_voltage_corpus_2 = settings.value("9ER20P_20/voltage_corpus_2", "").toString();
    b_9ER20P_20_v2.str_voltage_corpus_1 = settings.value("9ER20P_20_v2/voltage_corpus_1", "").toString();
    b_9ER20P_20_v2.str_voltage_corpus_2 = settings.value("9ER20P_20_v2/voltage_corpus_2", "").toString();
    b_9ER14PS_24.str_voltage_corpus_1 = settings.value("9ER14PS_24/voltage_corpus_1", "").toString();
    b_9ER14PS_24.str_voltage_corpus_2 = settings.value("9ER14PS_24/voltage_corpus_2", "").toString();
    b_9ER14PS_24_v2.str_voltage_corpus_1 = settings.value("9ER14PS_24_v2/voltage_corpus_1", "").toString();
    b_9ER14PS_24_v2.str_voltage_corpus_2 = settings.value("9ER14PS_24_v2/voltage_corpus_2", "").toString();
    b_9ER20P_28.str_voltage_corpus_1 = settings.value("9ER20P_28/voltage_corpus_1", "").toString();
    b_9ER20P_28.str_voltage_corpus_2 = settings.value("9ER20P_28/voltage_corpus_2", "").toString();
    b_9ER14P_24.str_voltage_corpus_1 = settings.value("9ER14P_24/voltage_corpus_1", "").toString();
    b_9ER14P_24.str_voltage_corpus_2 = settings.value("9ER14P_24/voltage_corpus_2", "").toString();

    // вещественные предельное напряжение на корпусе батареи
    simulator.voltage_corpus_limit = settings.value("simulator/voltage_corpus_limit", 1.0).toFloat();
    b_9ER20P_20.voltage_corpus_limit = settings.value("9ER20P_20/voltage_corpus_limit", 1.0).toFloat();
    b_9ER20P_20_v2.voltage_corpus_limit = settings.value("9ER20P_20_v2/voltage_corpus_limit", 1.0).toFloat();
    b_9ER14PS_24.voltage_corpus_limit = settings.value("9ER14PS_24/voltage_corpus_limit", 1.0).toFloat();
    b_9ER14PS_24_v2.voltage_corpus_limit = settings.value("9ER14PS_24_v2/voltage_corpus_limit", 1.0).toFloat();
    b_9ER20P_28.voltage_corpus_limit = settings.value("9ER20P_28/voltage_corpus_limit", 1.0).toFloat();
    b_9ER14P_24.voltage_corpus_limit = settings.value("9ER14P_24/voltage_corpus_limit", 1.0).toFloat();

    // строки - точки измерения сопротивления изоляции
    simulator.str_isolation_resistance_1 = settings.value("simulator/isolation_resistance_1", "").toString();
    simulator.str_isolation_resistance_2 = settings.value("simulator/isolation_resistance_2", "").toString();
    simulator.str_isolation_resistance_3 = settings.value("simulator/isolation_resistance_3", "").toString();
    simulator.str_isolation_resistance_4 = settings.value("simulator/isolation_resistance_4", "").toString();
    b_9ER20P_20.str_isolation_resistance_1 = settings.value("9ER20P_20/isolation_resistance_1", "").toString();
    b_9ER20P_20.str_isolation_resistance_2 = settings.value("9ER20P_20/isolation_resistance_2", "").toString();
    b_9ER20P_20.str_isolation_resistance_3 = settings.value("9ER20P_20/isolation_resistance_3", "").toString();
    b_9ER20P_20.str_isolation_resistance_4 = settings.value("9ER20P_20/isolation_resistance_4", "").toString();
    b_9ER20P_20_v2.str_isolation_resistance_1 = settings.value("9ER20P_20_v2/isolation_resistance_1", "").toString();
    b_9ER20P_20_v2.str_isolation_resistance_2 = settings.value("9ER20P_20_v2/isolation_resistance_2", "").toString();
    b_9ER20P_20_v2.str_isolation_resistance_3 = settings.value("9ER20P_20_v2/isolation_resistance_3", "").toString();
    b_9ER20P_20_v2.str_isolation_resistance_4 = settings.value("9ER20P_20_v2/isolation_resistance_4", "").toString();
    b_9ER14PS_24.str_isolation_resistance_1 = settings.value("9ER14PS_24/isolation_resistance_1", "").toString();
    b_9ER14PS_24.str_isolation_resistance_2 = settings.value("9ER14PS_24/isolation_resistance_2", "").toString();
    b_9ER14PS_24.str_isolation_resistance_3 = settings.value("9ER14PS_24/isolation_resistance_3", "").toString();
    b_9ER14PS_24.str_isolation_resistance_4 = settings.value("9ER14PS_24/isolation_resistance_4", "").toString();
    b_9ER14PS_24_v2.str_isolation_resistance_1 = settings.value("9ER14PS_24_v2/isolation_resistance_1", "").toString();
    b_9ER14PS_24_v2.str_isolation_resistance_2 = settings.value("9ER14PS_24_v2/isolation_resistance_2", "").toString();
    b_9ER14PS_24_v2.str_isolation_resistance_3 = settings.value("9ER14PS_24_v2/isolation_resistance_3", "").toString();
    b_9ER14PS_24_v2.str_isolation_resistance_4 = settings.value("9ER14PS_24_v2/isolation_resistance_4", "").toString();
    b_9ER20P_28.str_isolation_resistance_1 = settings.value("9ER20P_28/isolation_resistance_1", "").toString();
    b_9ER20P_28.str_isolation_resistance_2 = settings.value("9ER20P_28/isolation_resistance_2", "").toString();
    b_9ER20P_28.str_isolation_resistance_3 = settings.value("9ER20P_28/isolation_resistance_3", "").toString();
    b_9ER20P_28.str_isolation_resistance_4 = settings.value("9ER20P_28/isolation_resistance_4", "").toString();
    b_9ER14P_24.str_isolation_resistance_1 = settings.value("9ER14P_24/isolation_resistance_1", "").toString();
    b_9ER14P_24.str_isolation_resistance_2 = settings.value("9ER14P_24/isolation_resistance_2", "").toString();
    b_9ER14P_24.str_isolation_resistance_3 = settings.value("9ER14P_24/isolation_resistance_3", "").toString();
    b_9ER14P_24.str_isolation_resistance_4 = settings.value("9ER14P_24/isolation_resistance_4", "").toString();

    // вещественные предельное сопротивление изоляции батареи
    simulator.isolation_resistance_limit = settings.value("simulator/isolation_resistance_limit", 20).toFloat();
    b_9ER20P_20.isolation_resistance_limit = settings.value("9ER20P_20/isolation_resistance_limit", 20).toFloat();
    b_9ER20P_20_v2.isolation_resistance_limit = settings.value("9ER20P_20_v2/isolation_resistance_limit", 20).toFloat();
    b_9ER14PS_24.isolation_resistance_limit = settings.value("9ER14PS_24/isolation_resistance_limit", 20).toFloat();
    b_9ER14PS_24_v2.isolation_resistance_limit = settings.value("9ER14PS_24_v2/isolation_resistance_limit", 20).toFloat();
    b_9ER20P_28.isolation_resistance_limit = settings.value("9ER20P_28/isolation_resistance_limit", 20).toFloat();
    b_9ER14P_24.isolation_resistance_limit = settings.value("9ER14P_24/isolation_resistance_limit", 20).toFloat();

    // строки - точки измерения напряжения разомкнутых цепей групп
    simulator.opencircuitgroup_1 = settings.value("simulator/opencircuitgroup_1", "№ 1 - ").toString();
    simulator.opencircuitgroup_2 = settings.value("simulator/opencircuitgroup_2", "№ 2 - ").toString();
    simulator.opencircuitgroup_3 = settings.value("simulator/opencircuitgroup_3", "№ 3 - ").toString();
    simulator.opencircuitgroup_4 = settings.value("simulator/opencircuitgroup_4", "№ 4 - ").toString();
    simulator.opencircuitgroup_5 = settings.value("simulator/opencircuitgroup_5", "№ 5 - ").toString();
    simulator.opencircuitgroup_6 = settings.value("simulator/opencircuitgroup_6", "№ 6 - ").toString();
    simulator.opencircuitgroup_7 = settings.value("simulator/opencircuitgroup_7", "№ 7 - ").toString();
    simulator.opencircuitgroup_8 = settings.value("simulator/opencircuitgroup_8", "№ 8 - ").toString();
    simulator.opencircuitgroup_9 = settings.value("simulator/opencircuitgroup_9", "№ 9 - ").toString();
    simulator.opencircuitgroup_10 = settings.value("simulator/opencircuitgroup_10", "№ 10 - ").toString();
    simulator.opencircuitgroup_11 = settings.value("simulator/opencircuitgroup_11", "№ 11 - ").toString();
    simulator.opencircuitgroup_12 = settings.value("simulator/opencircuitgroup_12", "№ 12 - ").toString();
    simulator.opencircuitgroup_13 = settings.value("simulator/opencircuitgroup_13", "№ 13 - ").toString();
    simulator.opencircuitgroup_14 = settings.value("simulator/opencircuitgroup_14", "№ 14 - ").toString();
    simulator.opencircuitgroup_15 = settings.value("simulator/opencircuitgroup_15", "№ 15 - ").toString();
    simulator.opencircuitgroup_16 = settings.value("simulator/opencircuitgroup_16", "№ 16 - ").toString();
    simulator.opencircuitgroup_17 = settings.value("simulator/opencircuitgroup_17", "№ 17 - ").toString();
    simulator.opencircuitgroup_18 = settings.value("simulator/opencircuitgroup_18", "№ 18 - ").toString();
    simulator.opencircuitgroup_19 = settings.value("simulator/opencircuitgroup_19", "№ 19 - ").toString();
    simulator.opencircuitgroup_20 = settings.value("simulator/opencircuitgroup_20", "№ 20 - ").toString();
    simulator.opencircuitgroup_21 = settings.value("simulator/opencircuitgroup_21", "№ 21 - ").toString();
    simulator.opencircuitgroup_22 = settings.value("simulator/opencircuitgroup_22", "№ 22 - ").toString();
    simulator.opencircuitgroup_23 = settings.value("simulator/opencircuitgroup_23", "№ 23 - ").toString();
    simulator.opencircuitgroup_24 = settings.value("simulator/opencircuitgroup_24", "№ 24 - ").toString();
    simulator.opencircuitgroup_25 = settings.value("simulator/opencircuitgroup_25", "№ 25 - ").toString();
    simulator.opencircuitgroup_26 = settings.value("simulator/opencircuitgroup_26", "№ 26 - ").toString();
    simulator.opencircuitgroup_27 = settings.value("simulator/opencircuitgroup_27", "№ 27 - ").toString();
    simulator.opencircuitgroup_28 = settings.value("simulator/opencircuitgroup_28", "№ 28 - ").toString();

    b_9ER20P_20_v2.opencircuitgroup_1 = b_9ER20P_20.opencircuitgroup_1 = settings.value("9ER20P_20/opencircuitgroup_1", "№ 1 - ").toString();
    b_9ER20P_20_v2.opencircuitgroup_2 = b_9ER20P_20.opencircuitgroup_2 = settings.value("9ER20P_20/opencircuitgroup_2", "№ 2 - ").toString();
    b_9ER20P_20_v2.opencircuitgroup_3 = b_9ER20P_20.opencircuitgroup_3 = settings.value("9ER20P_20/opencircuitgroup_3", "№ 3 - ").toString();
    b_9ER20P_20_v2.opencircuitgroup_4 = b_9ER20P_20.opencircuitgroup_4 = settings.value("9ER20P_20/opencircuitgroup_4", "№ 4 - ").toString();
    b_9ER20P_20_v2.opencircuitgroup_5 = b_9ER20P_20.opencircuitgroup_5 = settings.value("9ER20P_20/opencircuitgroup_5", "№ 5 - ").toString();
    b_9ER20P_20_v2.opencircuitgroup_6 = b_9ER20P_20.opencircuitgroup_6 = settings.value("9ER20P_20/opencircuitgroup_6", "№ 6 - ").toString();
    b_9ER20P_20_v2.opencircuitgroup_7 = b_9ER20P_20.opencircuitgroup_7 = settings.value("9ER20P_20/opencircuitgroup_7", "№ 7 - ").toString();
    b_9ER20P_20_v2.opencircuitgroup_8 = b_9ER20P_20.opencircuitgroup_8 = settings.value("9ER20P_20/opencircuitgroup_8", "№ 8 - ").toString();
    b_9ER20P_20_v2.opencircuitgroup_9 = b_9ER20P_20.opencircuitgroup_9 = settings.value("9ER20P_20/opencircuitgroup_9", "№ 9 - ").toString();
    b_9ER20P_20_v2.opencircuitgroup_10 = b_9ER20P_20.opencircuitgroup_10 = settings.value("9ER20P_20/opencircuitgroup_10", "№ 10 - ").toString();
    b_9ER20P_20_v2.opencircuitgroup_11 = b_9ER20P_20.opencircuitgroup_11 = settings.value("9ER20P_20/opencircuitgroup_11", "№ 11 - ").toString();
    b_9ER20P_20_v2.opencircuitgroup_12 = b_9ER20P_20.opencircuitgroup_12 = settings.value("9ER20P_20/opencircuitgroup_12", "№ 12 - ").toString();
    b_9ER20P_20_v2.opencircuitgroup_13 = b_9ER20P_20.opencircuitgroup_13 = settings.value("9ER20P_20/opencircuitgroup_13", "№ 13 - ").toString();
    b_9ER20P_20_v2.opencircuitgroup_14 = b_9ER20P_20.opencircuitgroup_14 = settings.value("9ER20P_20/opencircuitgroup_14", "№ 14 - ").toString();
    b_9ER20P_20_v2.opencircuitgroup_15 = b_9ER20P_20.opencircuitgroup_15 = settings.value("9ER20P_20/opencircuitgroup_15", "№ 15 - ").toString();
    b_9ER20P_20_v2.opencircuitgroup_16 = b_9ER20P_20.opencircuitgroup_16 = settings.value("9ER20P_20/opencircuitgroup_16", "№ 16 - ").toString();
    b_9ER20P_20_v2.opencircuitgroup_17 = b_9ER20P_20.opencircuitgroup_17 = settings.value("9ER20P_20/opencircuitgroup_17", "№ 17 - ").toString();
    b_9ER20P_20_v2.opencircuitgroup_18 = b_9ER20P_20.opencircuitgroup_18 = settings.value("9ER20P_20/opencircuitgroup_18", "№ 18 - ").toString();
    b_9ER20P_20_v2.opencircuitgroup_19 = b_9ER20P_20.opencircuitgroup_19 = settings.value("9ER20P_20/opencircuitgroup_19", "№ 19 - ").toString();
    b_9ER20P_20_v2.opencircuitgroup_20 = b_9ER20P_20.opencircuitgroup_20 = settings.value("9ER20P_20/opencircuitgroup_20", "№ 20 - ").toString();

    b_9ER14PS_24_v2.opencircuitgroup_1 = b_9ER14PS_24.opencircuitgroup_1 = settings.value("9ER14PS_24/opencircuitgroup_1", "№ 1 - ").toString();
    b_9ER14PS_24_v2.opencircuitgroup_2 = b_9ER14PS_24.opencircuitgroup_2 = settings.value("9ER14PS_24/opencircuitgroup_2", "№ 2 - ").toString();
    b_9ER14PS_24_v2.opencircuitgroup_3 = b_9ER14PS_24.opencircuitgroup_3 = settings.value("9ER14PS_24/opencircuitgroup_3", "№ 3 - ").toString();
    b_9ER14PS_24_v2.opencircuitgroup_4 = b_9ER14PS_24.opencircuitgroup_4 = settings.value("9ER14PS_24/opencircuitgroup_4", "№ 4 - ").toString();
    b_9ER14PS_24_v2.opencircuitgroup_5 = b_9ER14PS_24.opencircuitgroup_5 = settings.value("9ER14PS_24/opencircuitgroup_5", "№ 5 - ").toString();
    b_9ER14PS_24_v2.opencircuitgroup_6 = b_9ER14PS_24.opencircuitgroup_6 = settings.value("9ER14PS_24/opencircuitgroup_6", "№ 6 - ").toString();
    b_9ER14PS_24_v2.opencircuitgroup_7 = b_9ER14PS_24.opencircuitgroup_7 = settings.value("9ER14PS_24/opencircuitgroup_7", "№ 7 - ").toString();
    b_9ER14PS_24_v2.opencircuitgroup_8 = b_9ER14PS_24.opencircuitgroup_8 = settings.value("9ER14PS_24/opencircuitgroup_8", "№ 8 - ").toString();
    b_9ER14PS_24_v2.opencircuitgroup_9 = b_9ER14PS_24.opencircuitgroup_9 = settings.value("9ER14PS_24/opencircuitgroup_9", "№ 9 - ").toString();
    b_9ER14PS_24_v2.opencircuitgroup_10 = b_9ER14PS_24.opencircuitgroup_10 = settings.value("9ER14PS_24/opencircuitgroup_10", "№ 10 - ").toString();
    b_9ER14PS_24_v2.opencircuitgroup_11 = b_9ER14PS_24.opencircuitgroup_11 = settings.value("9ER14PS_24/opencircuitgroup_11", "№ 11 - ").toString();
    b_9ER14PS_24_v2.opencircuitgroup_12 = b_9ER14PS_24.opencircuitgroup_12 = settings.value("9ER14PS_24/opencircuitgroup_12", "№ 12 - ").toString();
    b_9ER14PS_24_v2.opencircuitgroup_13 = b_9ER14PS_24.opencircuitgroup_13 = settings.value("9ER14PS_24/opencircuitgroup_13", "№ 13 - ").toString();
    b_9ER14PS_24_v2.opencircuitgroup_14 = b_9ER14PS_24.opencircuitgroup_14 = settings.value("9ER14PS_24/opencircuitgroup_14", "№ 14 - ").toString();
    b_9ER14PS_24_v2.opencircuitgroup_15 = b_9ER14PS_24.opencircuitgroup_15 = settings.value("9ER14PS_24/opencircuitgroup_15", "№ 15 - ").toString();
    b_9ER14PS_24_v2.opencircuitgroup_16 = b_9ER14PS_24.opencircuitgroup_16 = settings.value("9ER14PS_24/opencircuitgroup_16", "№ 16 - ").toString();
    b_9ER14PS_24_v2.opencircuitgroup_17 = b_9ER14PS_24.opencircuitgroup_17 = settings.value("9ER14PS_24/opencircuitgroup_17", "№ 17 - ").toString();
    b_9ER14PS_24_v2.opencircuitgroup_18 = b_9ER14PS_24.opencircuitgroup_18 = settings.value("9ER14PS_24/opencircuitgroup_18", "№ 18 - ").toString();
    b_9ER14PS_24_v2.opencircuitgroup_19 = b_9ER14PS_24.opencircuitgroup_19 = settings.value("9ER14PS_24/opencircuitgroup_19", "№ 19 - ").toString();
    b_9ER14PS_24_v2.opencircuitgroup_20 = b_9ER14PS_24.opencircuitgroup_20 = settings.value("9ER14PS_24/opencircuitgroup_20", "№ 20 - ").toString();
    b_9ER14PS_24_v2.opencircuitgroup_21 = b_9ER14PS_24.opencircuitgroup_21 = settings.value("9ER14PS_24/opencircuitgroup_21", "№ 21 - ").toString();
    b_9ER14PS_24_v2.opencircuitgroup_22 = b_9ER14PS_24.opencircuitgroup_22 = settings.value("9ER14PS_24/opencircuitgroup_22", "№ 22 - ").toString();
    b_9ER14PS_24_v2.opencircuitgroup_23 = b_9ER14PS_24.opencircuitgroup_23 = settings.value("9ER14PS_24/opencircuitgroup_23", "№ 23 - ").toString();
    b_9ER14PS_24_v2.opencircuitgroup_24 = b_9ER14PS_24.opencircuitgroup_24 = settings.value("9ER14PS_24/opencircuitgroup_24", "№ 24 - ").toString();

    b_9ER20P_28.opencircuitgroup_1 = settings.value("9ER20P_28/opencircuitgroup_1", "№ 1 - ").toString();
    b_9ER20P_28.opencircuitgroup_2 = settings.value("9ER20P_28/opencircuitgroup_2", "№ 2 - ").toString();
    b_9ER20P_28.opencircuitgroup_3 = settings.value("9ER20P_28/opencircuitgroup_3", "№ 3 - ").toString();
    b_9ER20P_28.opencircuitgroup_4 = settings.value("9ER20P_28/opencircuitgroup_4", "№ 4 - ").toString();
    b_9ER20P_28.opencircuitgroup_5 = settings.value("9ER20P_28/opencircuitgroup_5", "№ 5 - ").toString();
    b_9ER20P_28.opencircuitgroup_6 = settings.value("9ER20P_28/opencircuitgroup_6", "№ 6 - ").toString();
    b_9ER20P_28.opencircuitgroup_7 = settings.value("9ER20P_28/opencircuitgroup_7", "№ 7 - ").toString();
    b_9ER20P_28.opencircuitgroup_8 = settings.value("9ER20P_28/opencircuitgroup_8", "№ 8 - ").toString();
    b_9ER20P_28.opencircuitgroup_9 = settings.value("9ER20P_28/opencircuitgroup_9", "№ 9 - ").toString();
    b_9ER20P_28.opencircuitgroup_10 = settings.value("9ER20P_28/opencircuitgroup_10", "№ 10 - ").toString();
    b_9ER20P_28.opencircuitgroup_11 = settings.value("9ER20P_28/opencircuitgroup_11", "№ 11 - ").toString();
    b_9ER20P_28.opencircuitgroup_12 = settings.value("9ER20P_28/opencircuitgroup_12", "№ 12 - ").toString();
    b_9ER20P_28.opencircuitgroup_13 = settings.value("9ER20P_28/opencircuitgroup_13", "№ 13 - ").toString();
    b_9ER20P_28.opencircuitgroup_14 = settings.value("9ER20P_28/opencircuitgroup_14", "№ 14 - ").toString();
    b_9ER20P_28.opencircuitgroup_15 = settings.value("9ER20P_28/opencircuitgroup_15", "№ 15 - ").toString();
    b_9ER20P_28.opencircuitgroup_16 = settings.value("9ER20P_28/opencircuitgroup_16", "№ 16 - ").toString();
    b_9ER20P_28.opencircuitgroup_17 = settings.value("9ER20P_28/opencircuitgroup_17", "№ 17 - ").toString();
    b_9ER20P_28.opencircuitgroup_18 = settings.value("9ER20P_28/opencircuitgroup_18", "№ 18 - ").toString();
    b_9ER20P_28.opencircuitgroup_19 = settings.value("9ER20P_28/opencircuitgroup_19", "№ 19 - ").toString();
    b_9ER20P_28.opencircuitgroup_20 = settings.value("9ER20P_28/opencircuitgroup_20", "№ 20 - ").toString();
    b_9ER20P_28.opencircuitgroup_21 = settings.value("9ER20P_28/opencircuitgroup_21", "№ 21 - ").toString();
    b_9ER20P_28.opencircuitgroup_22 = settings.value("9ER20P_28/opencircuitgroup_22", "№ 22 - ").toString();
    b_9ER20P_28.opencircuitgroup_23 = settings.value("9ER20P_28/opencircuitgroup_23", "№ 23 - ").toString();
    b_9ER20P_28.opencircuitgroup_24 = settings.value("9ER20P_28/opencircuitgroup_24", "№ 24 - ").toString();
    b_9ER20P_28.opencircuitgroup_25 = settings.value("9ER20P_28/opencircuitgroup_25", "№ 25 - ").toString();
    b_9ER20P_28.opencircuitgroup_26 = settings.value("9ER20P_28/opencircuitgroup_26", "№ 26 - ").toString();
    b_9ER20P_28.opencircuitgroup_27 = settings.value("9ER20P_28/opencircuitgroup_27", "№ 27 - ").toString();
    b_9ER20P_28.opencircuitgroup_28 = settings.value("9ER20P_28/opencircuitgroup_28", "№ 28 - ").toString();

    b_9ER14P_24.opencircuitgroup_1 = settings.value("9ER14P_24/opencircuitgroup_1", "№ 1 - ").toString();
    b_9ER14P_24.opencircuitgroup_2 = settings.value("9ER14P_24/opencircuitgroup_2", "№ 2 - ").toString();
    b_9ER14P_24.opencircuitgroup_3 = settings.value("9ER14P_24/opencircuitgroup_3", "№ 3 - ").toString();
    b_9ER14P_24.opencircuitgroup_4 = settings.value("9ER14P_24/opencircuitgroup_4", "№ 4 - ").toString();
    b_9ER14P_24.opencircuitgroup_5 = settings.value("9ER14P_24/opencircuitgroup_5", "№ 5 - ").toString();
    b_9ER14P_24.opencircuitgroup_6 = settings.value("9ER14P_24/opencircuitgroup_6", "№ 6 - ").toString();
    b_9ER14P_24.opencircuitgroup_7 = settings.value("9ER14P_24/opencircuitgroup_7", "№ 7 - ").toString();
    b_9ER14P_24.opencircuitgroup_8 = settings.value("9ER14P_24/opencircuitgroup_8", "№ 8 - ").toString();
    b_9ER14P_24.opencircuitgroup_9 = settings.value("9ER14P_24/opencircuitgroup_9", "№ 9 - ").toString();
    b_9ER14P_24.opencircuitgroup_10 = settings.value("9ER14P_24/opencircuitgroup_10", "№ 10 - ").toString();
    b_9ER14P_24.opencircuitgroup_11 = settings.value("9ER14P_24/opencircuitgroup_11", "№ 11 - ").toString();
    b_9ER14P_24.opencircuitgroup_12 = settings.value("9ER14P_24/opencircuitgroup_12", "№ 12 - ").toString();
    b_9ER14P_24.opencircuitgroup_13 = settings.value("9ER14P_24/opencircuitgroup_13", "№ 13 - ").toString();
    b_9ER14P_24.opencircuitgroup_14 = settings.value("9ER14P_24/opencircuitgroup_14", "№ 14 - ").toString();
    b_9ER14P_24.opencircuitgroup_15 = settings.value("9ER14P_24/opencircuitgroup_15", "№ 15 - ").toString();
    b_9ER14P_24.opencircuitgroup_16 = settings.value("9ER14P_24/opencircuitgroup_16", "№ 16 - ").toString();
    b_9ER14P_24.opencircuitgroup_17 = settings.value("9ER14P_24/opencircuitgroup_17", "№ 17 - ").toString();
    b_9ER14P_24.opencircuitgroup_18 = settings.value("9ER14P_24/opencircuitgroup_18", "№ 18 - ").toString();
    b_9ER14P_24.opencircuitgroup_19 = settings.value("9ER14P_24/opencircuitgroup_19", "№ 19 - ").toString();
    b_9ER14P_24.opencircuitgroup_20 = settings.value("9ER14P_24/opencircuitgroup_20", "№ 20 - ").toString();
    b_9ER14P_24.opencircuitgroup_21 = settings.value("9ER14P_24/opencircuitgroup_21", "№ 21 - ").toString();
    b_9ER14P_24.opencircuitgroup_22 = settings.value("9ER14P_24/opencircuitgroup_22", "№ 22 - ").toString();
    b_9ER14P_24.opencircuitgroup_23 = settings.value("9ER14P_24/opencircuitgroup_23", "№ 23 - ").toString();
    b_9ER14P_24.opencircuitgroup_24 = settings.value("9ER14P_24/opencircuitgroup_24", "№ 24 - ").toString();

    // предельные напряжения разомкнутой цепи группы, вещественные, вольты
    // если есть ключ в конкретной секции, то берётся он. если его нет, то берётся такой же ключ из общей секции
    // если и его нету, то тогда значение по умолчанию.
    simulator.opencircuitgroup_limit_min = settings.value("simulator/opencircuitgroup_limit_min", settings.value("opencircuitgroup_limit_min", 32.3).toFloat()).toFloat();
    b_9ER20P_20.opencircuitgroup_limit_min = settings.value("9ER20P_20/opencircuitgroup_limit_min", settings.value("opencircuitgroup_limit_min", 32.3).toFloat()).toFloat();
    b_9ER20P_20_v2.opencircuitgroup_limit_min = settings.value("9ER20P_20_v2/opencircuitgroup_limit_min", settings.value("opencircuitgroup_limit_min", 32.3).toFloat()).toFloat();
    b_9ER14PS_24.opencircuitgroup_limit_min = settings.value("9ER14PS_24/opencircuitgroup_limit_min", settings.value("opencircuitgroup_limit_min", 32.3).toFloat()).toFloat();
    b_9ER14PS_24_v2.opencircuitgroup_limit_min = settings.value("9ER14PS_24_v2/opencircuitgroup_limit_min", settings.value("opencircuitgroup_limit_min", 32.3).toFloat()).toFloat();
    b_9ER20P_28.opencircuitgroup_limit_min = settings.value("9ER20P_28/opencircuitgroup_limit_min", settings.value("opencircuitgroup_limit_min", 32.3).toFloat()).toFloat();
    b_9ER14P_24.opencircuitgroup_limit_min = settings.value("9ER14P_24/opencircuitgroup_limit_min", settings.value("opencircuitgroup_limit_min", 32.3).toFloat()).toFloat();
    simulator.opencircuitgroup_limit_max = settings.value("simulator/opencircuitgroup_limit_max", settings.value("opencircuitgroup_limit_max", 33.3).toFloat()).toFloat();
    b_9ER20P_20.opencircuitgroup_limit_max = settings.value("9ER20P_20/opencircuitgroup_limit_max", settings.value("opencircuitgroup_limit_max", 33.3).toFloat()).toFloat();
    b_9ER20P_20_v2.opencircuitgroup_limit_max = settings.value("9ER20P_20_v2/opencircuitgroup_limit_max", settings.value("opencircuitgroup_limit_max", 33.3).toFloat()).toFloat();
    b_9ER14PS_24.opencircuitgroup_limit_max = settings.value("9ER14PS_24/opencircuitgroup_limit_max", settings.value("opencircuitgroup_limit_max", 33.3).toFloat()).toFloat();
    b_9ER14PS_24_v2.opencircuitgroup_limit_max = settings.value("9ER14PS_24_v2/opencircuitgroup_limit_max", settings.value("opencircuitgroup_limit_max", 33.3).toFloat()).toFloat();
    b_9ER20P_28.opencircuitgroup_limit_max = settings.value("9ER20P_28/opencircuitgroup_limit_max", settings.value("opencircuitgroup_limit_max", 33.3).toFloat()).toFloat();
    b_9ER14P_24.opencircuitgroup_limit_max = settings.value("9ER14P_24/opencircuitgroup_limit_max", settings.value("opencircuitgroup_limit_max", 33.3).toFloat()).toFloat();

    // предельные напряжения замкнутой цепи группы при токах 0.1, 0.5 и 1.0 ампер
    simulator.closecircuitgroup_limit_01a = settings.value("simulator/closecircuitgroup_limit_01a", 32.3).toFloat();
    b_9ER20P_20.closecircuitgroup_limit_01a = settings.value("9ER20P_20/closecircuitgroup_limit_01a", 32.3).toFloat();
    b_9ER20P_20_v2.closecircuitgroup_limit_01a = settings.value("9ER20P_20_v2/closecircuitgroup_limit_01a", 32.3).toFloat();
    b_9ER14PS_24.closecircuitgroup_limit_01a = settings.value("9ER14PS_24/closecircuitgroup_limit_01a", 32.3).toFloat();
    b_9ER14PS_24_v2.closecircuitgroup_limit_01a = settings.value("9ER14PS_24_v2/closecircuitgroup_limit_01a", 32.3).toFloat();
    b_9ER20P_28.closecircuitgroup_limit_01a = settings.value("9ER20P_28/closecircuitgroup_limit_01a", 32.3).toFloat();
    b_9ER14P_24.closecircuitgroup_limit_01a = settings.value("9ER14P_24/closecircuitgroup_limit_01a", 32.3).toFloat();
    simulator.closecircuitgroup_limit_05a = settings.value("simulator/closecircuitgroup_limit_05a", 33.3).toFloat();
    b_9ER20P_20.closecircuitgroup_limit_05a = settings.value("9ER20P_20/closecircuitgroup_limit_05a", 33.3).toFloat();
    b_9ER20P_20_v2.closecircuitgroup_limit_05a = settings.value("9ER20P_20_v2/closecircuitgroup_limit_05a", 33.3).toFloat();
    b_9ER14PS_24.closecircuitgroup_limit_05a = settings.value("9ER14PS_24/closecircuitgroup_limit_05a", 33.3).toFloat();
    b_9ER14PS_24_v2.closecircuitgroup_limit_05a = settings.value("9ER14PS_24_v2/closecircuitgroup_limit_05a", 33.3).toFloat();
    b_9ER20P_28.closecircuitgroup_limit_05a = settings.value("9ER20P_28/closecircuitgroup_limit_05a", 33.3).toFloat();
    b_9ER14P_24.closecircuitgroup_limit_05a = settings.value("9ER14P_24/closecircuitgroup_limit_05a", 33.3).toFloat();
    simulator.closecircuitgroup_limit_10a = settings.value("simulator/closecircuitgroup_limit_10a", 33.3).toFloat();
    b_9ER20P_20.closecircuitgroup_limit_10a = settings.value("9ER20P_20/closecircuitgroup_limit_10a", 33.3).toFloat();
    b_9ER20P_20_v2.closecircuitgroup_limit_10a = settings.value("9ER20P_20_v2/closecircuitgroup_limit_10a", 33.3).toFloat();
    b_9ER14PS_24.closecircuitgroup_limit_10a = settings.value("9ER14PS_24/closecircuitgroup_limit_10a", 33.3).toFloat();
    b_9ER14PS_24_v2.closecircuitgroup_limit_10a = settings.value("9ER14PS_24_v2/closecircuitgroup_limit_10a", 33.3).toFloat();
    b_9ER20P_28.closecircuitgroup_limit_10a = settings.value("9ER20P_28/closecircuitgroup_limit_10a", 33.3).toFloat();
    b_9ER14P_24.closecircuitgroup_limit_10a = settings.value("9ER14P_24/closecircuitgroup_limit_10a", 33.3).toFloat();

    // разомкнутые пределы - общие в дженерал секции.  а вот замкнутые таки разные
    // но это надо как-то придумать, чтобы такое же поле в конкретной секции перекрывало поле в общей.

    // кол-во ступеней проверки замкнутых цепей групп под нагрузкой
    simulator.number_discharge_stage = b_9ER20P_20.number_discharge_stage = b_9ER20P_20_v2.number_discharge_stage =
            b_9ER14PS_24.number_discharge_stage = b_9ER14PS_24_v2.number_discharge_stage = b_9ER14P_24.number_discharge_stage =
            b_9ER20P_28.number_discharge_stage = settings.value("number_discharge_stage",0).toInt();
    // токи нагрузки замкнутых цепей групп по ступеням, амперы
    simulator.disharge_current_1 = b_9ER20P_20.disharge_current_1 = b_9ER20P_20_v2.disharge_current_1 =
            b_9ER14PS_24.disharge_current_1 = b_9ER14PS_24_v2.disharge_current_1 = b_9ER14P_24.disharge_current_1 =
            b_9ER20P_28.disharge_current_1 = settings.value("disharge_current_1",0.1).toFloat();
    simulator.disharge_current_2 = b_9ER20P_20.disharge_current_2 = b_9ER20P_20_v2.disharge_current_2 =
            b_9ER14PS_24.disharge_current_2 = b_9ER14PS_24_v2.disharge_current_2 = b_9ER14P_24.disharge_current_2 =
            b_9ER20P_28.disharge_current_2 = settings.value("disharge_current_2",0.5).toFloat();
    simulator.disharge_current_3 = b_9ER20P_20.disharge_current_3 = b_9ER20P_20_v2.disharge_current_3 =
            b_9ER14PS_24.disharge_current_3 = b_9ER14PS_24_v2.disharge_current_3 = b_9ER14P_24.disharge_current_3 =
            b_9ER20P_28.disharge_current_3 = settings.value("disharge_current_3",1.0).toFloat();

    // время первой проверки на первой ступени, секунды
    simulator.time_uccg_1_1 = b_9ER20P_20.time_uccg_1_1 = b_9ER20P_20_v2.time_uccg_1_1 =
            b_9ER14PS_24.time_uccg_1_1 = b_9ER14PS_24_v2.time_uccg_1_1 = b_9ER14P_24.time_uccg_1_1 =
            b_9ER20P_28.time_uccg_1_1 = settings.value("time_uccg_1_1",30).toInt();
    // продолжительность первой ступени, секунды (15 минут = 60*15 = 900)
    simulator.time_uccg_1 = b_9ER20P_20.time_uccg_1 = b_9ER20P_20_v2.time_uccg_1 =
            b_9ER14PS_24.time_uccg_1 = b_9ER14PS_24_v2.time_uccg_1 = b_9ER14P_24.time_uccg_1 =
            b_9ER20P_28.time_uccg_1 = settings.value("time_uccg_1",900).toInt();
    // время первой проверки на второй ступени, секунды
    simulator.time_uccg_2_1 = b_9ER20P_20.time_uccg_2_1 = b_9ER20P_20_v2.time_uccg_2_1 =
            b_9ER14PS_24.time_uccg_2_1 = b_9ER14PS_24_v2.time_uccg_2_1 = b_9ER14P_24.time_uccg_2_1 =
            b_9ER20P_28.time_uccg_2_1 = settings.value("time_uccg_2_1",30).toInt();
    // продолжительность второй ступени, секунды (5 минут = 60*5 = 300)
    simulator.time_uccg_2 = b_9ER20P_20.time_uccg_2 = b_9ER20P_20_v2.time_uccg_2 =
            b_9ER14PS_24.time_uccg_2 = b_9ER14PS_24_v2.time_uccg_2 = b_9ER14P_24.time_uccg_2 =
            b_9ER20P_28.time_uccg_2 = settings.value("time_uccg_2",300).toInt();
    // время первой проверки на третьей ступени, секунды
    simulator.time_uccg_3_1 = b_9ER20P_20.time_uccg_3_1 = b_9ER20P_20_v2.time_uccg_3_1 =
            b_9ER14PS_24.time_uccg_3_1 = b_9ER14PS_24_v2.time_uccg_3_1 = b_9ER14P_24.time_uccg_3_1 =
            b_9ER20P_28.time_uccg_3_1 = settings.value("time_uccg_3_1",30).toInt();
    // продолжительность третьей  ступени, секунды (1 минута)
    simulator.time_uccg_3 = b_9ER20P_20.time_uccg_3 = b_9ER20P_20_v2.time_uccg_3 =
            b_9ER14PS_24.time_uccg_3 = b_9ER14PS_24_v2.time_uccg_3 = b_9ER14P_24.time_uccg_3 =
            b_9ER20P_28.time_uccg_3 = settings.value("time_uccg_3",300).toInt();

    // строки - точки измерения напряжения замкнутой цепи батареи
    simulator.str_closecircuitbattery=settings.value("simulator/str_closecircuitbattery","НЦЗб").toString();
    b_9ER20P_20.str_closecircuitbattery=settings.value("9ER20P_20/str_closecircuitbattery","НЦЗб").toString();
    b_9ER20P_20_v2.str_closecircuitbattery=settings.value("9ER20P_20_v2/str_closecircuitbattery","НЦЗб").toString();
    b_9ER14PS_24.str_closecircuitbattery=settings.value("9ER14PS_24/str_closecircuitbattery","НЦЗб").toString();
    b_9ER14PS_24_v2.str_closecircuitbattery=settings.value("9ER14PS_24_v2/str_closecircuitbattery","НЦЗб").toString();
    b_9ER20P_28.str_closecircuitbattery=settings.value("9ER20P_28/str_closecircuitbattery","НЦЗб").toString();
    b_9ER14P_24.str_closecircuitbattery=settings.value("9ER14P_24/str_closecircuitbattery","НЦЗб").toString();

    // ток проверки напряжения замкнутой цепи батареи, ампер
    simulator.discharge_current_battery = b_9ER20P_20.discharge_current_battery = b_9ER20P_20_v2.discharge_current_battery =
            b_9ER14PS_24.discharge_current_battery = b_9ER14PS_24_v2.discharge_current_battery = b_9ER14P_24.discharge_current_battery =
            b_9ER20P_28.discharge_current_battery = settings.value("discharge_current_battery",1.0).toFloat();
    // время проверки напряжения после начала подачи нагрузки, секунды
    simulator.time_uccb = b_9ER20P_20.time_uccb = b_9ER20P_20_v2.time_uccb =
            b_9ER14PS_24.time_uccb = b_9ER14PS_24_v2.time_uccb = b_9ER14P_24.time_uccb =
            b_9ER20P_28.time_uccb = settings.value("time_uccb",30).toInt();
    // предельное напряжение замкнутой цепи батареи, вольты
    simulator.closecircuitbattery_limit = b_9ER20P_20.closecircuitbattery_limit = b_9ER20P_20_v2.closecircuitbattery_limit =
            b_9ER14PS_24.closecircuitbattery_limit = b_9ER14PS_24_v2.closecircuitbattery_limit = b_9ER14P_24.closecircuitbattery_limit =
            b_9ER20P_28.closecircuitbattery_limit = settings.value("closecircuitbattery_limit",1.0).toFloat();

    // УУТББ
    // предельное сопротивление изоляции платы измерительной УУТББ, МОм
    b_9ER20P_20_v2.uutbb_isolation_resist_limit = settings.value("9ER20P_20_v2/uutbb_isolation_resist_limit", 5.0).toFloat();
    b_9ER14PS_24_v2.uutbb_isolation_resist_limit = settings.value("9ER14PS_24_v2/uutbb_isolation_resist_limit", 5.0).toFloat();
    // предельные напряжение разомкнутой цепи блока питания УУТББ, 7.05 +/- 0.15 вольт
    b_9ER20P_20_v2.opencircuitpower_limit_min = settings.value("9ER20P_20_v2/opencircuitpower_limit_min", 6.9).toFloat();
    b_9ER14PS_24_v2.opencircuitpower_limit_min = settings.value("9ER14PS_24_v2/opencircuitpower_limit_min", 6.9).toFloat();
    b_9ER20P_20_v2.opencircuitpower_limit_max = settings.value("9ER20P_20_v2/opencircuitpower_limit_max", 7.2).toFloat();
    b_9ER14PS_24_v2.opencircuitpower_limit_max = settings.value("9ER14PS_24_v2/opencircuitpower_limit_max", 7.2).toFloat();
    //предельное напряжение замкнутой цепи блока питания УУТББ, при токе 0.1А, вольт
    b_9ER20P_20_v2.closecircuitpower_limit = settings.value("9ER20P_20_v2/closecircuitpower_limit", 5.7).toFloat();
    b_9ER14PS_24_v2.closecircuitpower_limit = settings.value("9ER14PS_24_v2/closecircuitpower_limit", 5.7).toFloat();
    // время подключения нагрузки на БП УУТББ, секунды
    b_9ER20P_20_v2.time_ccp = settings.value("9ER20P_20_v2/time_ccp", 5.7).toInt();
    b_9ER14PS_24_v2.time_ccp = settings.value("9ER14PS_24_v2/time_ccp", 5.7).toInt();
}
