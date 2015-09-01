#include "settings.h"
#include <QDebug>
#include <QTextCodec>
#include "battery.h"

extern Battery simulator, b_9ER20P_20, b_9ER20P_20_v2, b_9ER14PS_24, b_9ER14PS_24_v2, b_9ER20P_28, b_9ER14P_24;

Settings::Settings(QObject *parent) : QObject(parent)
{
    qDebug() << "App path : " << qApp->applicationDirPath(); // возвращает путь к папке с исполняемым файлом
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
    int size = settings.beginReadArray("batteries");
    settings.setArrayIndex(0);
    simulator.type_name = settings.value("name").toString();
    settings.setArrayIndex(1);
    b_9ER20P_20.type_name = settings.value("name").toString();
    settings.setArrayIndex(2);
    b_9ER20P_20_v2.type_name = settings.value("name").toString();
    settings.setArrayIndex(3);
    b_9ER14PS_24.type_name = settings.value("name").toString();
    settings.setArrayIndex(4);
    b_9ER14PS_24_v2.type_name = settings.value("name").toString();
    settings.setArrayIndex(5);
    b_9ER20P_28.type_name = settings.value("name").toString();
    settings.setArrayIndex(6);
    b_9ER14P_24.type_name = settings.value("name").toString();
    settings.endArray(); // end array, don't forget!

    // int - кол-во цепей/групп
    simulator.group_num=settings.value("simulator/group_num",0).toInt();
    b_9ER20P_20.group_num=settings.value("9ER20P_20/group_num",0).toInt();
    b_9ER20P_20_v2.group_num=settings.value("9ER20P_20_v2/group_num",0).toInt();
    b_9ER14PS_24.group_num=settings.value("9ER14PS_24/group_num",0).toInt();
    b_9ER14PS_24_v2.group_num=settings.value("9ER14PS_24_v2/group_num",0).toInt();
    b_9ER20P_28.group_num=settings.value("9ER20P_28/group_num",0).toInt();
    b_9ER14P_24.group_num=settings.value("9ER14P_24/group_num",0).toInt();
    // строки - точки измерения напряжения на корпусе
    simulator.voltage_corpus_1 = settings.value("simulator/voltage_corpus_1", "").toString();
    simulator.voltage_corpus_2 = settings.value("simulator/voltage_corpus_2", "").toString();
    b_9ER20P_20.voltage_corpus_1 = settings.value("9ER20P_20/voltage_corpus_1", "").toString();
    b_9ER20P_20.voltage_corpus_2 = settings.value("9ER20P_20/voltage_corpus_2", "").toString();
    b_9ER20P_20_v2.voltage_corpus_1 = settings.value("9ER20P_20_v2/voltage_corpus_1", "").toString();
    b_9ER20P_20_v2.voltage_corpus_2 = settings.value("9ER20P_20_v2/voltage_corpus_2", "").toString();
    b_9ER14PS_24.voltage_corpus_1 = settings.value("9ER14PS_24/voltage_corpus_1", "").toString();
    b_9ER14PS_24.voltage_corpus_2 = settings.value("9ER14PS_24/voltage_corpus_2", "").toString();
    b_9ER14PS_24_v2.voltage_corpus_1 = settings.value("9ER14PS_24_v2/voltage_corpus_1", "").toString();
    b_9ER14PS_24_v2.voltage_corpus_2 = settings.value("9ER14PS_24_v2/voltage_corpus_2", "").toString();
    b_9ER20P_28.voltage_corpus_1 = settings.value("9ER20P_28/voltage_corpus_1", "").toString();
    b_9ER20P_28.voltage_corpus_2 = settings.value("9ER20P_28/voltage_corpus_2", "").toString();
    b_9ER14P_24.voltage_corpus_1 = settings.value("9ER14P_24/voltage_corpus_1", "").toString();
    b_9ER14P_24.voltage_corpus_2 = settings.value("9ER14P_24/voltage_corpus_2", "").toString();

    // строки - точки измерения сопротивления изоляции
    simulator.isolation_resistance_1 = settings.value("simulator/isolation_resistance_1", "").toString();
    simulator.isolation_resistance_2 = settings.value("simulator/isolation_resistance_2", "").toString();
    simulator.isolation_resistance_3 = settings.value("simulator/isolation_resistance_3", "").toString();
    simulator.isolation_resistance_4 = settings.value("simulator/isolation_resistance_4", "").toString();
    b_9ER20P_20.isolation_resistance_1 = settings.value("9ER20P_20/isolation_resistance_1", "").toString();
    b_9ER20P_20.isolation_resistance_2 = settings.value("9ER20P_20/isolation_resistance_2", "").toString();
    b_9ER20P_20.isolation_resistance_3 = settings.value("9ER20P_20/isolation_resistance_3", "").toString();
    b_9ER20P_20.isolation_resistance_4 = settings.value("9ER20P_20/isolation_resistance_4", "").toString();
    b_9ER20P_20_v2.isolation_resistance_1 = settings.value("9ER20P_20_v2/isolation_resistance_1", "").toString();
    b_9ER20P_20_v2.isolation_resistance_2 = settings.value("9ER20P_20_v2/isolation_resistance_2", "").toString();
    b_9ER20P_20_v2.isolation_resistance_3 = settings.value("9ER20P_20_v2/isolation_resistance_3", "").toString();
    b_9ER20P_20_v2.isolation_resistance_4 = settings.value("9ER20P_20_v2/isolation_resistance_4", "").toString();
    b_9ER14PS_24.isolation_resistance_1 = settings.value("9ER14PS_24/isolation_resistance_1", "").toString();
    b_9ER14PS_24.isolation_resistance_2 = settings.value("9ER14PS_24/isolation_resistance_2", "").toString();
    b_9ER14PS_24.isolation_resistance_3 = settings.value("9ER14PS_24/isolation_resistance_3", "").toString();
    b_9ER14PS_24.isolation_resistance_4 = settings.value("9ER14PS_24/isolation_resistance_4", "").toString();
    b_9ER14PS_24_v2.isolation_resistance_1 = settings.value("9ER14PS_24_v2/isolation_resistance_1", "").toString();
    b_9ER14PS_24_v2.isolation_resistance_2 = settings.value("9ER14PS_24_v2/isolation_resistance_2", "").toString();
    b_9ER14PS_24_v2.isolation_resistance_3 = settings.value("9ER14PS_24_v2/isolation_resistance_3", "").toString();
    b_9ER14PS_24_v2.isolation_resistance_4 = settings.value("9ER14PS_24_v2/isolation_resistance_4", "").toString();
    b_9ER20P_28.isolation_resistance_1 = settings.value("9ER20P_28/isolation_resistance_1", "").toString();
    b_9ER20P_28.isolation_resistance_2 = settings.value("9ER20P_28/isolation_resistance_2", "").toString();
    b_9ER20P_28.isolation_resistance_3 = settings.value("9ER20P_28/isolation_resistance_3", "").toString();
    b_9ER20P_28.isolation_resistance_4 = settings.value("9ER20P_28/isolation_resistance_4", "").toString();
    b_9ER14P_24.isolation_resistance_1 = settings.value("9ER14P_24/isolation_resistance_1", "").toString();
    b_9ER14P_24.isolation_resistance_2 = settings.value("9ER14P_24/isolation_resistance_2", "").toString();
    b_9ER14P_24.isolation_resistance_3 = settings.value("9ER14P_24/isolation_resistance_3", "").toString();
    b_9ER14P_24.isolation_resistance_4 = settings.value("9ER14P_24/isolation_resistance_4", "").toString();
}
