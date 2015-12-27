#ifndef KDS_H
#define KDS_H

#include <QTextCodec>

/*
 * Объект текущей проверки одной конкретной батареи
 */

#include <QObject>

class Kds : public QObject
{
    Q_OBJECT
public:
    explicit Kds(QObject *parent = 0);

    QTextCodec *codec;              // кодек для строк для информационного обмена
    unsigned char nmc;              // номер-код проверяемого устройства (батарея, БИП, ...)

signals:
    void sendSerialData(const QByteArray &data); // сигнал передачи данных в последовательный порт.
    void sendSerialReceivedData(quint8 operation_code, QByteArray data); // посылает сигнал, когда получены данные из СОМ-порта

public slots:
    void getSerialDataReceived(QByteArray data); // ф-я, которая принимает данные из последовательного порта
    void send_request(quint8 operation_code, QByteArray text); // формирует запрос для посылки в СОМ-порт
};

#endif // KDS_H
