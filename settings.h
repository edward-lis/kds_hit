#ifndef SETTINGS_H
#define SETTINGS_H

#include <QObject>

class settings : public QObject
{
    Q_OBJECT
public:
    explicit settings(QObject *parent = 0);

signals:

public slots:
};

#endif // SETTINGS_H
