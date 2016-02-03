#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QItemDelegate>
#include <QStandardItemModel>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QTime>
#include <QMessageBox>


//+++ Edward
#include <QTimer>
#include <QStateMachine>

#include "serialport.h"

#define OFFLINE     "Нет связи"             // забить в ини-файл, сделать строкой
#define ONLINE      "Связь установлена"
// КОМАНДА PING доступна всегда, но лучше в пассивном/IDLE режиме устройства.
#define PING    "PING"  // некая строка пинга.  длина до 240 байт
//#define PING    "123456789012345678901234567890123456789012345678901234567890"  // некая строка пинга.  длина до 240 байт

// задержки после определённой команды и перед выдачей следующей команды, по протоколу
#define delay_command_after_start_before_request    400     //ms    // после команды пуска режима, перед первым запросом
#define delay_command_after_request_before_next     270     //ms    // в режиме, между запросами
#define delay_after_IDLE_before_other               150     //ms    // после IDLE перед следующим режимом

#define delay_timeOut                               450     //ms    // таймаут ответа на запрос
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

    //+++ Edward
    /// Конечный автомат
    QStateMachine *machine;
    /// Состояния КА
    QState *stateSerialPortClose; ///< Порт закрыт
    QState *stateSerialPortOpening; ///< Открытие порта
    QState *stateFirst; ///< первое состояние
    QState *stateIdleCommand; ///< Состояние посылки первой после окрытия порта команды Idle

    /// Состояния режима проверки типа подключенной батареи
    QState *stateCheckPolarB; ///< собрать режим полярности
    QState *stateCheckPolarBPoll; ///< запрос полярности
    QState *stateCheckPolarBIdleTypeB; ///< разобрать режим полярности, полярность прямая = 0
    QState *stateCheckPolarBIdleUocPB; ///< разобрать режим полярности, полярность обратная = 1
    QState *stateCheckTypeB; ///< собрать режим подтипа батареи
    QState *stateCheckTypeBPoll; ///< запрос подтипа батареи
    QState *stateCheckTypeBIdleUocPB; ///< разобрать режим подтипа батареи, тип хххР-20
    QState *stateCheckUocPB; ///< собрать режим проверки напряжения БП УТБ
    QState *stateCheckUocPBPoll; ///< опрос напряжения БП УТБ

    void setupMachine(); ///< настроить КА (finitestatemachine.cpp)
    void machineAddCheckBatteryType(); ///< собрать КА для проверки типа подключенной батареи (checkbatterytype.cpp)
    // для режима проверки типа батареи, callback, ф-ии анализа ответа, переходные ф-ии
    void onstateCheckPolarB(QByteArray data);
    void onstateCheckPolarBPoll(QByteArray data);
    void onstateCheckPolarBIdleTypeB(QByteArray data);
    void onstateCheckPolarBIdleUocPB(QByteArray data);
    void onstateCheckTypeB(QByteArray data);
    void onstateCheckTypeBPoll(QByteArray data);
    void onstateCheckTypeBIdleUocPB(QByteArray data);
    void onstateCheckUocPB(QByteArray data);
    void onstateCheckUocPBPoll(QByteArray data);

    /// Ф-ия подготовки и последующей посылки команды cS
    void prepareSendCommand(QString cS, int dT, void (MainWindow::*fCA)(QByteArray));
    /// Ф-ия подготовки и последующей посылки команды "IDLE#" с возвратом в первое состояние
    void prepareSendIdleToFirstCommand();
//+++

private:
    Ui::MainWindow *ui;
    int iStartCheck;
    int iBatteryIndex;
    int iStep;
    int iAllSteps;
    int iStepVoltageOnTheHousing;
    int iStepInsulationResistance;
    int iStepOpenCircuitVoltageGroup;
    int iStepClosedCircuitVoltageGroup;
    int iStepClosedCircuitVoltageBattery;
    int iStepInsulationResistanceMeasuringBoardUUTBB;
    int iStepOpenCircuitVoltagePowerSupply;
    int iStepClosedCircuitVoltagePowerSupply;
    //int iDiagnosticModeCurrentIndex;
    int iParamsNumberChecked;
    QString color;
    QString paramMsg;
    QSerialPort *com;
    //bool bStop;
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

    float paramVoltageOnTheHousing1;
    float paramVoltageOnTheHousing2;
    float paramInsulationResistance1;
    float paramInsulationResistance2;
    float paramInsulationResistance3;
    float paramInsulationResistance4;
    float paramOpenCircuitVoltageGroup1;
    //float paramClosedCircuitVoltage;
    //float paramClosedCircuitVoltage;
    float paramClosedCircuitVoltage;

    //+++ Edward
    /// Экземпляр класса последовательный порт
    SerialPort *serialPort;
    /// Признак состояния начала работы с коробком после установления связи. Сбросить систему в первоначальное состояние
    bool start_work;
    /// Тайм-аут ответа коробочки при запросе
    QTimer *timeout;
    /// Таймер между пингами
    QTimer *timerPing;
    /// Таймер задержки выдачи следующей команды после выдачи ИДЛЕ, команд, опрос/запрос. Если переход один.
    QTimer *timerDelay;
    /// Таймер задержки выдачи следующей команды после выдачи ИДЛЕ, команд, опрос/запрос. Если переход с состоянием 0
    QTimer *timerDelay0;
    /// Таймер задержки выдачи следующей команды после выдачи ИДЛЕ, команд, опрос/запрос. Если переход с состоянием 1
    QTimer *timerDelay1;
    /// Время задержки выдачи следующей команды после выдачи ИДЛЕ, команд, опрос/запрос
    int delayTime;
    /// Текущая команда для посылки в порт
    QString commandString;

    /// Функция анализа принятого ответа на команду (переходная ф-ия)(типа callback)
    void (MainWindow::*funcCommandAnswer)(QByteArray); ///< указатель на ф-ию
    /// конкретная функция "funcCommandAnswer", которая будет подставляться и выполнятся для анализа ответа на посылку первого IDLE сброса коробка в исходное
    void onIdleOK(QByteArray data);
    //+++

public slots:
    void openCOMPort();
    void closeCOMPort();
    void writeCOMPortData();
    void readCOMPortData();
    void checkAutoModeDiagnostic();
    void resetCheck();
    void isUUTBB();
    void setPause();
    //void clickModeDiagnostic();
    //void paramCheck();
    void handleSelectionChangedBattery(int index);
    void Log(QString message, QString color);
    //void setEnabled(bool flag);
    void delay(int millisecondsToWait);
    void progressBarSet(int iVal);
    //void progressBarSetMaximum();
    //void checkVoltageOnTheHousing(int iBatteryCurrentIndex, int iCurrentStep);
    void checkVoltageOnTheHousing();
    void checkInsulationResistance();
    void checkOpenCircuitVoltageGroup();
    void checkClosedCircuitVoltageGroup();
    void checkClosedCircuitVoltageBattery();
    void checkInsulationResistanceMeasuringBoardUUTBB();
    void checkOpenCircuitVoltagePowerSupply();
    void checkClosedCircuitVoltagePowerSupply();

    //+++ Edward
    /// Приём данных от последовательного порта
    void recvSerialData(quint8 operation_code, const QByteArray data);
    /// Срабатывание таймаута
    void procTimeout();
    /// Послать пинг
    void sendPing();

    /// ф-ия состояния Порт закрыт
    void enterStateSerialPortClose();
    /// ф-ия состояния Открытие порта
    void enterStateSerialPortOpening();
    /// ф-ия состояния Порт открыт, готовность к работе
    void enter1State();
    /// ф-ия посылки команды
    void sendCommand();

    // Для режима проверки типа подключенной батареи
    void prepareCheckBatteryType(); ///< подготовка и запуск режима проверки типа батареи
    //+++

signals:
    //+++ Edward
    /*! Cигнал передачи данных в последовательный порт.
     * \param[in] operation_code Код операции
     * \param[in] data Тело сообщения
     */
   void sendSerialData(quint8 operation_code, const QByteArray &data);
   /// Сигнал события, что порт открылся нормально
   void signalSerialPortOpened();
   /// Сигнал события, что порт не открылся. Ошибка при открытии порта.
   void signalSerialPortErrorOpened();
   /// Сигнал таймера пинга
   // void signalTimerPing();
   /// Сигнал наступления события сброса коробочки в начале работы после установления связи по последовательному порту
   void workStart();
   /// Сигнал - ответ от коробочки не такой, какой ожидалось
   //void signalWrongReply();
   //+++
};

#endif // MAINWINDOW_H
