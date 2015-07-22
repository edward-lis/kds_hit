#ifndef SETTINGS_H
#define SETTINGS_H

#include <QObject>
#include <QSettings>
#include <QApplication>

#define INI_FILE_NAME   "kds_hit.ini" // короткое имя файла

class Settings : public QObject
{
    Q_OBJECT
public:
    explicit Settings(QObject *parent = 0);
    /**
     * @brief loadSettings загрузить конфиг из ini-файла
     */
    void loadSettings();
    void saveSettings();

    QString testString; // тестовая строка

    // здесь будут все переменные, которые настраиваются

private:
    QString m_sSettingsFile; // полное имя ini-файла

signals:

public slots:
};

#endif // SETTINGS_H
