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
#define KDS_TIMEOUT           1           // код ошибки таймаут
#define KDS_INCORRECT_REPLY   2           // код ошибки неверный ответ

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
    QStandardItemModel *modelClosedCircuitVoltageGroup;
    QStandardItemModel *modelDepassivation;
    QStandardItemModel *modelOpenCircuitVoltageGroup;
    QStandardItemModel *modelInsulationResistanceUUTBB;
    int iBatteryIndex; ///< номер/индекс текущей батареи в массиве батарей.
    int iCurrentStep;
    int iMaxSteps;
    QList<int> imDepassivation;
    QList<double> dArrayVoltageOnTheHousing;
    QList<double> dArrayInsulationResistance;
    QList<double> dArrayOpenCircuitVoltageGroup;
    QList<double> dArrayOpenCircuitVoltageBattery;
    QList<double> dArrayDepassivation;
    QList<double> dArrayClosedCircuitVoltageGroup;
    QList<double> dArrayClosedCircuitVoltageBattery;
    QList<double> dArrayInsulationResistanceUUTBB;
    QList<double> dArrayOpenCircuitVoltagePowerSupply;
    QList<double> dArrayClosedCircuitVoltagePowerSupply;
    QString str;
    QString color;
    QString paramMsg;
    bool bState;
    void getCOMPorts();
    void comboxSetData();
    float param; /// для отладки, потом убрать
    double randMToN(double M, double N); /// для отладки, потом убрать
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
	
    /// Признак ручного режима
    bool bModeManual;

    //+++

public slots:
    void Log(QString message, QString color);
    void delay(int millisecondsToWait);
    void checkVoltageOnTheHousing();
    void checkInsulationResistance();
    void checkOpenCircuitVoltageGroup();
    void checkOpenCircuitVoltageBattery();
    void checkClosedCircuitVoltageGroup();
    void checkDepassivation();
    void checkClosedCircuitVoltageBattery();
    void checkInsulationResistanceUUTBB();
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
   void itemChangedOpenCircuitVoltageGroup(QStandardItem* itm);
   void itemChangedClosedCircuitVoltageGroup(QStandardItem* itm);
   void itemChangedDepassivation(QStandardItem* itm);
   void itemChangedInsulationResistanceUUTBB(QStandardItem* itm);
   void on_rbModeDiagnosticAuto_toggled(bool checked);
   void on_rbModeDiagnosticManual_toggled(bool checked);
   void on_rbVoltageOnTheHousing_toggled(bool checked);
   void on_rbInsulationResistance_toggled(bool checked);
   void on_rbOpenCircuitVoltageGroup_toggled(bool checked);
   void on_rbOpenCircuitVoltageBattery_toggled(bool checked);
   void on_rbClosedCircuitVoltageGroup_toggled(bool checked);
   void on_rbDepassivation_toggled(bool checked);
   void on_rbClosedCircuitVoltageBattery_toggled(bool checked);
   void on_rbInsulationResistanceUUTBB_toggled(bool checked);
   void on_rbOpenCircuitVoltagePowerSupply_toggled(bool checked);
   void on_rbClosedCircuitVoltagePowerSupply_toggled(bool checked);
   void on_cbIsUUTBB_toggled(bool checked);
   void on_btnClosedCircuitVoltageBattery_clicked();
   void on_btnOpenCircuitVoltagePowerSupply_clicked();
   void on_btnClosedCircuitVoltagePowerSupply_clicked();
   void on_btnDepassivation_clicked();

   //+++ Edward
   /// Приём массива из последовательного порта
   void recvSerialData(quint8 operation_code, const QByteArray data);

   /// Посылка подготовленного массива baSendArray в порт
   void sendSerialData();

   /// Срабатывание таймаута ответа
   void procTimeoutResponse();

   /// Послать пинг
   void sendPing();

   /// Переключение комбинацией клавиш Ctrl+D DeveloperState
   void triggerDeveloperState();// { bDeveloperState=!bDeveloperState; }
   //+++
   void on_btnCOMPortOpenClose_clicked();
   void on_btnCheckConnectedBattery_clicked();
   void on_btnVoltageOnTheHousing_clicked();
   void on_comboBoxBatteryList_currentIndexChanged(int index);
   void on_btnStartStopAutoModeDiagnostic_clicked();
   void on_cbParamsAutoMode_currentIndexChanged(int index);
   void on_btnInsulationResistance_clicked();
   void on_btnInsulationResistanceUUTBB_clicked();
   void on_btnOpenCircuitVoltageGroup_clicked();
   void on_btnOpenCircuitVoltageBattery_clicked();
   void on_btnClosedCircuitVoltageGroup_clicked();
   void on_actionCheckSave_triggered();
   void on_actionCheckLoad_triggered();
   void on_btnBuildReport_clicked();

protected:
    //virtual void showEvent(QShowEvent *e); // перегруз ф-ии для выпуска сигнала после отрисовки окна
    //перегруз события закрытия главного окна
    virtual void closeEvent(QCloseEvent *e);
};

#endif // MAINWINDOW_H
