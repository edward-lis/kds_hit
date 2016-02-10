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

    float coefADC1; ///< коэффициент пересчёта кода АЦП в вольты
    float coefADC2; ///< коэффициент пересчёта кода АЦП в вольты

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
    QState *stateCheckShowResult; ///< показать результаты измерений

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

    // Проверка напряжения на корпусе
    /// Состояния КА проверки напряжения на корпусе
    QState *stateVoltageCase; ///< Посылка команды и ожидание окончания задержки после отсылки (для сбора режима в коробочке) "UcaseX#
    QState *stateVoltageCasePoll; ///< После приёма ответа подтверждения от коробочки о корректном сборе режима "UcaseX#OK - посылка опроса "Ucase?#"
    QState *stateVoltageCaseIdle; ///< Разобрать режим
    //QState *stateVoltageCaseIdleOk; ///< Разобрать режим
    void onstateVoltageCase(QByteArray data);
    void onstateVoltageCasePoll(QByteArray data);
    void onstateVoltageCaseIdle(QByteArray data);
    void machineAddVoltageCase(); ///< собрать КА для проверки напряжения на корпусе (voltagecase.cpp)

//+++

private:
    Ui::MainWindow *ui;
    QStandardItemModel *model;
    int iStartCheck;
    int iBatteryIndex;
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
    Settings settings;
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

    //+++ Edward
    /// Приём данных от последовательного порта
    void recvSerialData(quint8 operation_code, const QByteArray data);
    /// Срабатывание таймаута
    void procTimeout();
    /// Послать пинг
    void sendPing();
    /// показать месседжбокс о несоответствии подключенной и выбранной батарей
    void slotCheckBatteryDone();

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
    // Для режима проверки напряжения на корпусе. Выполнится при нажатии на кнопку Старт проверки (1)
    void prepareVoltageCase(); ///< подготовка и запуск режима проверки напряжения на корпусе
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
   /// Сигнал - показать месседжбокс о несоответствии подключенной и выбранной батарей
   void signalCheckBatteryDone(int x, int y);
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
};

#endif // MAINWINDOW_H
