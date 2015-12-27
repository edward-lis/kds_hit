#ifndef COMPORTWIDGET_H
#define COMPORTWIDGET_H

/*
 * Объект окна выбора и настройки последовательного порта и ввода/вывода
 */

#include <QWidget>
#include <QtCore/QDateTime>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>

namespace Ui {
    class ComPortWidget;
}


class ComPortWidget : public QWidget
{
    Q_OBJECT

public:
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

    explicit ComPortWidget(QWidget *parent = 0);
    ~ComPortWidget();

    Settings settings() const;

private:

    void fillPortsParameters();
    void fillPortsInfo();
    void updateSettings();

    void initComPortWidgetCloseState();
    void initComPortWidgetOpenState();
    void initButtonConnections();
    void initTimer();
    void deinitTimer();

private:
    Ui::ComPortWidget *ui;
    Settings currentSettings;
    QSerialPort *serial;
    QTimer *timer;
    QByteArray rxBuffer;

private slots:
    void procSerialDataReceive();
    void procSerialDataTransfer(const QByteArray &data);
    void procControlButtonClick();
    void procTimerOut();
    void handleError(QSerialPort::SerialPortError error);

signals:
    void sendSerialReceivedData(QByteArray data);
};

#endif // COMPORTWIDGET_H
