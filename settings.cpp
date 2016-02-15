#include "settings.h"
#include <QDebug>
#include <QTextCodec>
#include <QApplication>
#include <QSettings>
#include <QMessageBox>

#include "battery.h"
#include "mainwindow.h"

extern QVector<Battery> battery;
//extern MainWindow *w;

Settings::Settings(QObject *parent) : QObject(parent)
{
    //qDebug() << "App path : " << qApp->applicationDirPath() << "/" << INI_FILE_NAME; // возвращает путь к папке с исполняемым файлом и ini-файлу
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
    int i;

    QSettings settings(m_sSettingsFile, QSettings::IniFormat);
    settings.setIniCodec("Windows-1251"); // т.к. мы в винде, то устанавливаем кодек для ini-файла
    QTextCodec *codec = QTextCodec::codecForName("cp866"); // берём кодек
    QTextCodec::setCodecForLocale(codec); // и устанавливаем его для локали, чтобы корректно показывал qDebug() русский ANSI шрифт из 1251 файла в 866 локали консоли

    //QString sText = settings.value("battery/text", "").toString();
    //testString = sText;
    //qDebug() << "sText" << testString;

    if(!QFile::exists(m_sSettingsFile))
    {
        QMessageBox::critical(NULL, tr("Критическая ошибка КДС ХИТ"), tr("Отсутствует файл конфигурации, запуск системы невозможен"));
        //qApp->quit();
        exit(0);
    }
    // !!! сделать в релизе проверку на целостность конф.файла

    QString str;
    bool ok;
    // Для чтения шестнадцатиричных данных
    str = settings.value("k1_code", 1).toString();
    coefADC1 = settings.value("k1_volt", 0).toFloat()/str.toUInt(&ok, 16);
    //qDebug()<<"coef"<<QString::number(coefADC1)<<settings.value("k1_volt", 0).toFloat()<<str;

    coefADC2 = settings.value("k2_volt", 0).toFloat()/settings.value("k2_code", 1).toString().toUInt(&ok, 16);
    //qDebug()<<"coef"<<QString::number(coefADC2)<<settings.value("k2_volt", 0).toFloat()
           //<<qPrintable(QString::number(settings.value("k2_code", 1).toString().toUInt(&ok, 16), 16));// прочитать ключ как строку, преобразовать её в 16, напечатать как номер, преобразованный в 16, без кавычек
    voltage_circuit_type = settings.value("voltage_circuit_type", 25.0).toFloat();
    voltage_power_uutbb = settings.value("voltage_power_uutbb", 5.0).toFloat();

    // get battaries names, as array
    num_batteries_types = settings.beginReadArray("batteries"); // добавляет префикс к текущей группе, и начинает чтение из массива. возвращает размер массива
    if(num_batteries_types > 0)
    {
        battery.resize(num_batteries_types);
        for(i=0; i<num_batteries_types; i++)
        {
            settings.setArrayIndex(i);
            battery[i].str_type_name = settings.value("name").toString(); // считываем наименования батарей
        }
        settings.endArray(); // end array, don't forget!
    }

    // value(,): первый параметр - ключ. второй параметр - параметр по умолчанию (допустим, если нет запрашиваемого ключа в тексте файла)
    // ключ - секция/ключ.   если без секции, то секция по умолчанию [General]

    // int - кол-во цепей/групп
    battery[0].group_num = settings.value("9ER20P_20/group_num", 20).toInt();
    battery[1].group_num = settings.value("9ER14PS_24/group_num", 24).toInt();
    battery[2].group_num = settings.value("9ER14P_24/group_num", 24).toInt();
    battery[3].group_num = settings.value("9ER20P_28/group_num", 28).toInt();
    // строки - точки измерения напряжения на корпусе
    battery[0].str_voltage_corpus[0] = settings.value("9ER20P_20/voltage_corpus_1", "").toString();
    battery[0].str_voltage_corpus[1] = settings.value("9ER20P_20/voltage_corpus_2", "").toString();
    battery[1].str_voltage_corpus[0] = settings.value("9ER14PS_24/voltage_corpus_1", "").toString();
    battery[1].str_voltage_corpus[1] = settings.value("9ER14PS_24/voltage_corpus_2", "").toString();
    battery[2].str_voltage_corpus[0] = settings.value("9ER14P_24/voltage_corpus_1", "").toString();
    battery[2].str_voltage_corpus[1] = settings.value("9ER14P_24/voltage_corpus_2", "").toString();
    battery[3].str_voltage_corpus[0] = settings.value("9ER20P_28/voltage_corpus_1", "").toString();
    battery[3].str_voltage_corpus[1] = settings.value("9ER20P_28/voltage_corpus_2", "").toString();
    // вещественные предельное напряжение на корпусе батареи
    voltage_corpus_limit = settings.value("voltage_corpus_limit", 1.0).toFloat();

    // строки - точки измерения сопротивления изоляции
    battery[0].i_isolation_resistance_num=settings.beginReadArray("isolation_resistance_9ER20P_20");
    battery[0].str_isolation_resistance.resize(battery[0].i_isolation_resistance_num); // зарезервируем место под строки
    for(i=0; i<battery[0].i_isolation_resistance_num; i++)
    {
        settings.setArrayIndex(i);
        battery[0].str_isolation_resistance[i] = settings.value("isolation_resistance", "").toString(); // считываем наименования точек измерения сопротивления изоляции
    }
    settings.endArray();

    battery[1].i_isolation_resistance_num=settings.beginReadArray("isolation_resistance_9ER14PS_24");
    battery[1].str_isolation_resistance.resize(battery[1].i_isolation_resistance_num); // зарезервируем место под строки
    for(i=0; i<battery[1].i_isolation_resistance_num; i++)
    {
        settings.setArrayIndex(i);
        battery[1].str_isolation_resistance[i] = settings.value("isolation_resistance", "").toString(); // считываем наименования точек измерения сопротивления изоляции
    }
    settings.endArray();

    battery[2].i_isolation_resistance_num=settings.beginReadArray("isolation_resistance_9ER14P_24");
    battery[2].str_isolation_resistance.resize(battery[2].i_isolation_resistance_num); // зарезервируем место под строки
    for(i=0; i<battery[2].i_isolation_resistance_num; i++)
    {
        settings.setArrayIndex(i);
        battery[2].str_isolation_resistance[i] = settings.value("isolation_resistance", "").toString(); // считываем наименования точек измерения сопротивления изоляции
    }
    settings.endArray();

    battery[3].i_isolation_resistance_num=settings.beginReadArray("isolation_resistance_9ER20P_28");
    battery[3].str_isolation_resistance.resize(battery[3].i_isolation_resistance_num); // зарезервируем место под строки
    for(i=0; i<battery[3].i_isolation_resistance_num; i++)
    {
        settings.setArrayIndex(i);
        battery[3].str_isolation_resistance[i] = settings.value("isolation_resistance", "").toString(); // считываем наименования точек измерения сопротивления изоляции
    }
    settings.endArray();

    // вещественные предельное сопротивление изоляции батареи
    isolation_resistance_limit = settings.value("isolation_resistance_limit", 20).toFloat();

    // строки - точки измерения напряжения цепей групп
    settings.beginReadArray("circuitgroup_9ER20P_20");
    for(i=0; i<battery[0].group_num; i++)
    {
        settings.setArrayIndex(i);
        battery[0].circuitgroup[i] = settings.value("circuitgroup", "").toString(); // считываем наименования точек измерения групп цепей
    }
    settings.endArray();
    settings.beginReadArray("circuitgroup_9ER14PS_24");
    for(i=0; i<battery[1].group_num; i++)
    {
        settings.setArrayIndex(i);
        battery[1].circuitgroup[i] = settings.value("circuitgroup", "").toString(); // считываем наименования точек измерения групп цепей
    }
    settings.endArray();
    settings.beginReadArray("circuitgroup_9ER14P_24");
    for(i=0; i<battery[2].group_num; i++)
    {
        settings.setArrayIndex(i);
        battery[2].circuitgroup[i] = settings.value("circuitgroup", "").toString(); // считываем наименования точек измерения групп цепей
    }
    settings.endArray();
    settings.beginReadArray("circuitgroup_9ER20P_28");
    for(i=0; i<battery[3].group_num; i++)
    {
        settings.setArrayIndex(i);
        battery[3].circuitgroup[i] = settings.value("circuitgroup", "").toString(); // считываем наименования точек измерения групп цепей
    }
    settings.endArray();

    // предельные напряжения разомкнутой цепи группы, вещественные, вольты
    opencircuitgroup_limit_min = settings.value("opencircuitgroup_limit_min", 32.3).toFloat();
    opencircuitgroup_limit_max = settings.value("opencircuitgroup_limit_max", 33.3).toFloat();

    // строки - точки измерения напряжения цепи батареи
    battery[0].circuitbattery = settings.value("9ER20P_20/circuitbattery", "").toString();
    battery[1].circuitbattery = settings.value("9ER14PS_24/circuitbattery", "").toString();
    battery[2].circuitbattery = settings.value("9ER14P_24/circuitbattery", "").toString();
    battery[3].circuitbattery = settings.value("9ER20P_28/circuitbattery", "").toString();
    //предельное напряжение разомкнутой цепи батареи, вольты
    opencircuitbattery_limit = settings.value("opencircuitbattery_limit_min", 32.3).toFloat();
    // предельное напряжение замкнутой цепи группы, вольт
    closecircuitgroup_limit = settings.value("closecircuitgroup_limit", 27.0).toFloat();
    // предельное напряжение замкнутой цепи батареи, вольты
    closecircuitbattery_limit = settings.value("closecircuitbattery_limit", 30.0).toFloat();

    // кол-во ступеней проверки замкнутых цепей групп под нагрузкой
    number_depassivation_stage = settings.value("number_depassivation_stage", 3).toInt();
    // токи распассивации замкнутых цепей групп по ступеням, амперы
    depassivation_current[0] = settings.value("depassivation_current_1", 0.25).toFloat();
    depassivation_current[1] = settings.value("depassivation_current_2", 0.5).toFloat();
    depassivation_current[2] = settings.value("depassivation_current_3", 1.0).toFloat();
    // продолжительность ступеней распассивации, секунды
    time_depassivation[0] = settings.value("time_depassivation_1", 900).toInt();
    time_depassivation[1] = settings.value("time_depassivation_2", 300).toInt();
    time_depassivation[2] = settings.value("time_depassivation_3", 60).toInt();

    // предельное сопротивление изоляции платы измерительной УУТББ, МОм
    uutbb_isolation_resist_limit = settings.value("uutbb_isolation_resist_limit", 5.0).toFloat();
    // предельные напряжение разомкнутой цепи блока питания УУТББ, 7.05 +/- 0.15 вольт
    uutbb_opencircuitpower_limit_min = settings.value("uutbb_opencircuitpower_limit_min", 6.9).toFloat();
    uutbb_opencircuitpower_limit_max = settings.value("uutbb_opencircuitpower_limit_max", 7.2).toFloat();
    // предельное напряжение замкнутой цепи блока питания УУТББ, при токе 0.1А, вольт
    uutbb_closecircuitpower_limit = settings.value("uutbb_opencircuitpower_limit_max", 5.7).toFloat();
    // время подключения нагрузки на БП УУТББ, секунды
    uutbb_time_ccp = settings.value("uutbb_time_ccp", 10).toInt();

    // строки - точки измерения сопротивления изоляции УУТББ
    battery[0].i_uutbb_resist_num=settings.beginReadArray("uutbb_resist_9ER20P_20");
    battery[0].uutbb_resist.resize(battery[0].i_uutbb_resist_num); // зарезервируем место под строки
    for(i=0; i<battery[0].i_uutbb_resist_num; i++)
    {
        settings.setArrayIndex(i);
        battery[0].uutbb_resist[i] = settings.value("uutbb_resist", "").toString(); // считываем наименования точек измерения сопротивления изоляции
    }
    settings.endArray();
    battery[1].i_uutbb_resist_num=settings.beginReadArray("uutbb_resist_9ER14PS_24");
    battery[1].uutbb_resist.resize(battery[1].i_uutbb_resist_num); // зарезервируем место под строки
    for(i=0; i<battery[1].i_uutbb_resist_num; i++)
    {
        settings.setArrayIndex(i);
        battery[1].uutbb_resist[i] = settings.value("uutbb_resist", "").toString(); // считываем наименования точек измерения сопротивления изоляции
    }
    settings.endArray();

    // Функция сопротивления/кода АЦП
    functionResist.resize(settings.beginReadArray("resist_function"));
    for(i=0; i<functionResist.size(); i++)
    {
        settings.setArrayIndex(i);
        functionResist[i].resist = settings.value("resist", 0).toInt(); // dec
        functionResist[i].codeADC = settings.value("codeADC", 0).toString().toUInt(&ok, 16); // hex
    }


    printSettings();

#if 0
    // если есть ключ в конкретной секции, то берётся он. если его нет, то берётся такой же ключ из общей секции
    // если и его нету, то тогда значение по умолчанию.
    b_9ER20P_20.opencircuitgroup_limit_min = settings.value("9ER20P_20/opencircuitgroup_limit_min", settings.value("opencircuitgroup_limit_min", 32.3).toFloat()).toFloat();
#endif
}

void Settings::printSettings()
{
    int i,j;
    for(i=0; i<num_batteries_types; i++)
    {
        qDebug()<<"Battery"<<battery[i].str_type_name<<"==================================================";
        qDebug()<<tr("Сопротивление изоляции :");
        for(j=0; j<battery[i].i_isolation_resistance_num; j++)
        {
            qDebug()<<battery[i].str_isolation_resistance[j];
        }
        qDebug()<<tr("Сопротивление изоляции УУТББ:");
        for(j=0; j<battery[i].i_uutbb_resist_num; j++)
        {
            qDebug()<<j<<battery[i].uutbb_resist[j];
        }
    }
    qDebug()<<"Resistance function:";
    for(i=0; i<functionResist.size(); i++)
    {
        qDebug()<<i<<" R= "<<qPrintable(QString::number(functionResist[i].resist))<<", codeADC= 0x"<<qPrintable(QString::number(functionResist[i].codeADC, 16));
    }
}
