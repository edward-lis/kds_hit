#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QItemDelegate>
#include <QStandardItemModel>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QTime>
#include <QMessageBox>
#include <QDebug>
#include <QEventLoop>


//+++ Edward
#include <QTimer>
#include <QStateMachine>

#include "serialport.h"                     // ф-ии для работы с последовательным портом
#include "battery.h"                        // структура с данными по батареям
#include "settings.h"                       // загрузка конфигурации из ini-файла
#include "qcustomplot.h"

#define OFFLINE     "Нет связи"             // забить в ини-файл, сделать строкой
#define ONLINE      "Связь установлена"
// КОМАНДА PING доступна всегда, но лучше в пассивном/IDLE режиме устройства.
#define PING    "PING"  // некая строка пинга.  длина до 240 байт
//#define PING    "123456789012345678901234567890123456789012345678901234567890"  // некая строка пинга.  длина до 240 байт

// задержки после определённой команды и перед выдачей следующей команды, по протоколу
#define delay_command_after_start_before_request    400     //ms    // после команды пуска режима, перед первым запросом
#define delay_command_after_request_before_next     270     //ms    // в режиме, между запросами
#define delay_after_IDLE_before_other               150     //ms    // после IDLE перед следующим режимом

#define delay_timeOut                               500     //ms    // таймаут ответа на запрос
#define delay_timerPing                             1000     //ms    // пауза между пингами должна быть больше, чем таймаут!
//+++

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    QCustomPlot *customPlot;

private:
    Ui::MainWindow *ui;
    QStandardItemModel *model;
    int iStartCheck;
    int iBatteryIndex; ///< номер/индекс текущей батареи в массиве батарей.
    int iStep;
    int iAllSteps;
    int iStepVoltageOnTheHousing;
    int iStepInsulationResistance;
    int iStepOpenCircuitVoltageGroup;
    int iStepClosedCircuitVoltageGroup;
    int iStepDepassivation;
    int iStepClosedCircuitVoltageBattery;
    int iStepInsulationResistanceMeasuringBoardUUTBB;
    int iStepOpenCircuitVoltagePowerSupply;
    int iStepClosedCircuitVoltagePowerSupply;
    int iParamsNumberChecked;
    QVector<int> imDepassivation;
    QString str;
    QString paramMsg;
    bool bPause;
    bool bCheckCompleteVoltageOnTheHousing;
    bool bCheckCompleteInsulationResistance;
    bool bCheckCompleteOpenCircuitVoltageGroup;
    bool bCheckCompleteClosedCircuitVoltageGroup;
    bool bCheckCompleteClosedCircuitVoltageBattery;
    bool bCheckCompleteInsulationResistanceMeasuringBoardUUTBB;
    bool bCheckCompleteOpenCircuitVoltagePowerSupply;
    bool bCheckCompleteClosedCircuitVoltagePowerSupply;
    void getCOMPorts();
    float param;
    //+++ Edward
    /// Установки из ini-файла
    Settings settings;

    /// Экземпляр класса последовательный порт
    SerialPort *serialPort;

    /// Признак отрытого порта
    bool bPortOpen;

    /// Тайм-аут ответа коробочки при запросе
    QTimer *timeoutResponse;

    /// Таймер между пингами
    QTimer *timerPing;

    /// Пустой цикл для ожидания ответа от коробочки
    QEventLoop loop;

    /// Буфер с текущими принятым массивом из порта
    QByteArray baRecvArray;

    /// Буфер с текущим подготовленным массивом для передачи в порт
    QByteArray baSendArray;

    /// Буфер с текущей подготовленной командой для передачи в порт
    QByteArray baSendCommand;

    /// Получить из принятого массива данные опроса
    quint16 getRecvData(QByteArray baRecvArray);

    /// Первый принятый пинг - послать Idle
    bool bFirstPing;

    /// Режим разработчика
    bool bDeveloperState;
    //+++

public slots:
    void checkAutoModeDiagnostic();
    void setPause();
    void handleSelectionChangedBattery(int index);
    void Log(QString message, QString color);
    void delay(int millisecondsToWait);
    void progressBarSet(int iVal);
    void checkVoltageOnTheHousing();
    void checkInsulationResistance();
    void checkOpenCircuitVoltageGroup();
    void checkClosedCircuitVoltageGroup();
    void checkClosedCircuitVoltageBattery();
    void checkDepassivation();
    void checkInsulationResistanceMeasuringBoardUUTBB();
    void checkOpenCircuitVoltagePowerSupply();
    void checkClosedCircuitVoltagePowerSupply();

signals:
    //+++ Edward
    /*! Cигнал передачи массива в последовательный порт.
     * \param[in] operation_code Код операции
     * \param[in] data Тело сообщения
     */
   void signalSendSerialData(quint8 operation_code, const QByteArray &data);

   /// Сигнал готовности данных по приёму, выйти из пустого цикла ожидания
   void signalSerialDataReady();

   /// Сигнал срабатывания тайм-аута по приёму
   void signalTimeoutResponse();
   //+++

private slots:
   void itemChanged(QStandardItem*);
   void on_rbModeDiagnosticAuto_toggled(bool checked);
   void on_rbModeDiagnosticManual_toggled(bool checked);
   void on_rbVoltageOnTheHousing_toggled(bool checked);
   void on_rbInsulationResistance_toggled(bool checked);
   void on_rbOpenCircuitVoltageGroup_toggled(bool checked);
   void on_rbClosedCircuitVoltageGroup_toggled(bool checked);
   void on_rbDepassivation_toggled(bool checked);
   void on_rbClosedCircuitVoltageBattery_toggled(bool checked);
   void on_rbInsulationResistanceMeasuringBoardUUTBB_toggled(bool checked);
   void on_cbIsUUTBB_toggled(bool checked);
   void on_pushButton_clicked();
   void on_cbInsulationResistance_currentIndexChanged(const QString &arg1);

   //+++ Edward
   /// Приём массива из последовательного порта
   void recvSerialData(quint8 operation_code, const QByteArray data);

   /// Посылка подготовленного массива baSendArray в порт
   void sendSerialData();

   /// Срабатывание таймаута ответа
   void procTimeoutResponse();

   /// Послать пинг
   void sendPing();
   //+++
   void on_btnCOMPortOpenClose_clicked();
   void on_btnCheckConnectedBattery_clicked();
};

#endif // MAINWINDOW_H
