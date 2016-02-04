#ifndef SERIALPORT_H
#define SERIALPORT_H

#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QTimer>

class SerialPort : public QObject
{
    Q_OBJECT
public:
    explicit SerialPort(QObject *parent = 0);
    //! Структура параметров последовательного порта
    struct Settings {
        QString name;
        qint32 baudRate;
        QString stringBaudRate;
        QSerialPort::DataBits dataBits;
        QString stringDataBits;
        QSerialPort::Parity parity;
        QString stringParity;
        QSerialPort::StopBits stopBits;
        QString stringStopBits;
        QSerialPort::FlowControl flowControl;
        QString stringFlowControl;
        bool localEchoEnabled;
    };

    //! Последовательный порт
    QSerialPort *serial;
    /// Текстовый кодек для строк для информационного обмена
    QTextCodec *codec;
    /// Таймер для тайм-аута приёма буфера
    QTimer *timerTimeOut;
    /// Приёмный буфер
    QByteArray rxBuffer;

    //! Открыть порт portName
    bool openPort(QString portName);
    /// Разобрать принятый пакет
    void disassembleReadMsg(QByteArray data);

private:
    //! Параметры последовательного порта
    //Settings currentSettings;

signals:
    /// Данные от коробочки приняты
    void readySerialData(quint8 operation_code, const QByteArray &data);
    /// Сигнал состояния - критическая ошибка
    void signalCriticalError();

public slots:
    //! Перехватчик критических ошибок
    void handleError(QSerialPort::SerialPortError error);

    /*! \brief Запись данных в последовательный порт.
     * Массив подготавливается в соответствии с протоколом информационного обмена, и пишется в порт
     * \param[in] operation_code Код операции
     * \param[in] data Тело сообщения
     */
    void writeSerialPort(quint8 operation_code, const QByteArray &data);
    /// Чтение данных из последовательного порта
    void readSerialPort();
    /// таймерная ф-ия
    void procTimerTimeOut();
    /// Закрыть последовательный порт
    void closePort();
};

#endif // SERIALPORT_H
