#include "settings.h"
#include <QDebug>
#include <QTextCodec>

Settings::Settings(QObject *parent) : QObject(parent)
{
    qDebug() << "App path : " << qApp->applicationDirPath(); // возвращает путь к папке с исполняемым файлом
    m_sSettingsFile = qApp->applicationDirPath() + "/" + INI_FILE_NAME;
    //QSettings settings(m_sSettingsFile, QSettings::NativeFormat);
    //Коментарий
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
    QTextCodec::setCodecForLocale(codec); // и устанавливаем его для локали, чтобы корректно показывал qDebug()

    QString sText = settings.value("battery/text", "").toString();
    testString = sText;
    qDebug() << "sText" << testString;
}
