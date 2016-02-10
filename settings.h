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

    // здесь будут все общие переменные, которые настраиваются
    int num_batteries_types; ///< кол-во уникальных типов батарей
    float coefADC1; ///< коэффициент пересчёта кода АЦП в вольты
    float coefADC2; ///< коэффициент пересчёта кода АЦП в вольты

private:
    QString m_sSettingsFile; ///< полное имя ini-файла

signals:

public slots:
    void loadSettings(); ///< загрузить конфиг из ini-файла
};

#endif // SETTINGS_H
