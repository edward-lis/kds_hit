#include <QDebug>
#include "mainwindow.h"
#include "ui_mainwindow.h"

// !!! добавить в ини-файл и в настройки - какое ПРИЛИЧНОЕ напряжение должно быть? U=2500, например
// !!! отсутствие напряжения УУТББ - 0? или тоже некий порог?

/* Алгоритм определения типа подключенной батареи описан в файле протокола информационного обмена с коробочкой.
    Находясь в состоянии готовности, нажимается кнопка "проверка батареи". По этому событию КА переходит в состояние проверки типа
подключенной батареи, и при входе в это состояние выполняется слот,в котором подготавливается первая команда определения полярности подключенной
батареи, и она же посылается в порт.
При приёме ответа, выполняется ф-ия анализа принятых данных, по результатам подготавливается следующая команда, и КА переходит в
следующее нужное состояние.
При входе в следующее нужное состояние, происходит посылка подготовленной команды, затем принимается ответ,
анализ ответа, подготовка следующей команды и так далее, пока КА не выйдет из режима проверки типа подключенной батареи.

Результатом работы алгоритма является индекс подключенной батареи в матрице типов батарей - строка типа подключенной батареи.
В одном из случаев есть неопределённость, могут быть две батареи.


КА состоит из перечня всех возможных состояний, таблицы перехода и переходных ф-ий, которые анализируют полученный ответ от коробочки.

  В mainwindow.h добавить состояния.

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

Добавить ф-ию для описания КА (чтоб не забивать конструктор главного окна текстом)
    void machineAddCheckBatteryType(); ///< собрать КА для проверки типа подключенной батареи

В mainwindow.cpp (или в ф-ии настройки КА setupMachine() в файле finitestatemachine.cpp) :

Добавить в конструктор переход на эту проверку из первого состояния при нажатии на кнопку "проверка типа батареи"
    stateFirst->addTransition(ui->pushButton_CheckBattery, SIGNAL(clicked(bool)), stateCheckPolarB); // нажата кнопка проверки типа батареи
    (при переходе в stateCheckPolarB состояние выполнится ф-ия prepareCheckBatteryType(), где подготовится первая команда режима
    и посылка этой команды, см. начало комментария)

Добавить описание состояния этого режима в КА, до запуска самого КА, куда-нить ниже "stateFirst = new QState;",
но выше "stateFirst->addTransition,...", потому что там переход на stateFirst состояние.
    // добавить в КА режим проверки типа подключенной батареи
    machineAddCheckBatteryType();

Матрица типов подключенных батарей, в зависимости от электрических цепей:

                            Полярность прямая   обратная
Напряжение БП УУТББ есть    9ER20P-20 в2        9ЕR14PS-24 в2
Напряжение БП УУТББ нет     9ER20P-20           9ЕR14PS-24 или 9ER14P-24 - неоднозначность
Напряжение цепи 28 есть     9ER20P-28           х

*/

QString type[2][3]={{"9ER20P-20 УУТББ", "9ER20P-20", "9ER20P-28"},{"9ЕR14PS-24 УУТББ", "9ЕR14PS-24 или 9ER14P-24", "???"}};
static int x=0, y=0;

void MainWindow::machineAddCheckBatteryType()
{
    stateCheckPolarB = new QState;
    stateCheckPolarB->setObjectName("stateCheckPolarB");
    connect(stateCheckPolarB, SIGNAL(entered()), this, SLOT(prepareCheckBatteryType()));

    stateCheckPolarBPoll = new QState;
    stateCheckPolarBPoll->setObjectName("stateCheckPolarBPoll");
    connect(stateCheckPolarBPoll, SIGNAL(entered()), this, SLOT(sendCommand()));

    stateCheckPolarBIdleTypeB = new QState;
    stateCheckPolarBIdleTypeB->setObjectName("stateCheckPolarBIdleTypeB");
    connect(stateCheckPolarBIdleTypeB, SIGNAL(entered()), this, SLOT(sendCommand()));

    stateCheckPolarBIdleUocPB = new QState;
    stateCheckPolarBIdleUocPB->setObjectName("stateCheckPolarBIdleUocPB");
    connect(stateCheckPolarBIdleUocPB, SIGNAL(entered()), this, SLOT(sendCommand()));

    stateCheckTypeB = new QState;
    stateCheckTypeB->setObjectName("stateCheckTypeB");
    connect(stateCheckTypeB, SIGNAL(entered()), this, SLOT(sendCommand()));

    stateCheckTypeBPoll = new QState;
    stateCheckTypeBPoll->setObjectName("stateCheckTypeBPoll");
    connect(stateCheckTypeBPoll, SIGNAL(entered()), this, SLOT(sendCommand()));

    stateCheckTypeBIdleUocPB = new QState;
    stateCheckTypeBIdleUocPB->setObjectName("stateCheckTypeBIdleUocPB");
    connect(stateCheckTypeBIdleUocPB, SIGNAL(entered()), this, SLOT(sendCommand()));

    stateCheckUocPB = new QState;
    stateCheckUocPB->setObjectName("stateCheckUocPB");
    connect(stateCheckUocPB, SIGNAL(entered()), this, SLOT(sendCommand()));

    stateCheckUocPBPoll = new QState;
    stateCheckUocPBPoll->setObjectName("stateCheckUocPBPoll");
    connect(stateCheckUocPBPoll, SIGNAL(entered()), this, SLOT(sendCommand()));

    // добавляем переходы состояний
    stateCheckPolarB->addTransition(ui->btnCOMPortConnect, SIGNAL(clicked(bool)), stateSerialPortClose); // при нажатии кнопки порта - закроем его
    stateCheckPolarB->addTransition(serialPort, SIGNAL(signalCriticalError()), stateSerialPortClose); // при критической ошибке в порту - закроем его
    stateCheckPolarB->addTransition(timeout, SIGNAL(timeout()), stateFirst); // по таймауту приёма вывалимся в начальное состояние
    stateCheckPolarB->addTransition(timerDelay, SIGNAL(timeout()), stateCheckPolarBPoll); // по таймауту задержки после режима - вывалимся в следующее состояние

    stateCheckPolarBPoll->addTransition(ui->btnCOMPortConnect, SIGNAL(clicked(bool)), stateSerialPortClose); // при нажатии кнопки порта - закроем его
    stateCheckPolarBPoll->addTransition(serialPort, SIGNAL(signalCriticalError()), stateSerialPortClose); // при критической ошибке в порту - закроем его
    stateCheckPolarBPoll->addTransition(timeout, SIGNAL(timeout()), stateFirst); // по таймауту приёма вывалимся в начальное состояние
    // в ф-ии обработки ответа опроса заведём разные таймеры в зависимости от полученных данных
    stateCheckPolarBPoll->addTransition(timerDelay0, SIGNAL(timeout()), stateCheckPolarBIdleTypeB); // по таймауту задержки после режима - вывалимся в следующее состояние
    stateCheckPolarBPoll->addTransition(timerDelay1, SIGNAL(timeout()), stateCheckPolarBIdleUocPB); // по таймауту задержки после режима - вывалимся в следующее состояние

    stateCheckPolarBIdleTypeB->addTransition(ui->btnCOMPortConnect, SIGNAL(clicked(bool)), stateSerialPortClose); // при нажатии кнопки порта - закроем его
    stateCheckPolarBIdleTypeB->addTransition(serialPort, SIGNAL(signalCriticalError()), stateSerialPortClose); // при критической ошибке в порту - закроем его
    stateCheckPolarBIdleTypeB->addTransition(timeout, SIGNAL(timeout()), stateFirst); // по таймауту приёма вывалимся в начальное состояние
    stateCheckPolarBIdleTypeB->addTransition(timerDelay, SIGNAL(timeout()), stateCheckTypeB); // по таймауту задержки после режима - вывалимся в следующее состояние

    stateCheckPolarBIdleUocPB->addTransition(ui->btnCOMPortConnect, SIGNAL(clicked(bool)), stateSerialPortClose); // при нажатии кнопки порта - закроем его
    stateCheckPolarBIdleUocPB->addTransition(serialPort, SIGNAL(signalCriticalError()), stateSerialPortClose); // при критической ошибке в порту - закроем его
    stateCheckPolarBIdleUocPB->addTransition(timeout, SIGNAL(timeout()), stateFirst); // по таймауту приёма вывалимся в начальное состояние
    stateCheckPolarBIdleUocPB->addTransition(timerDelay, SIGNAL(timeout()), stateCheckUocPB); // по таймауту задержки после режима - вывалимся в следующее состояние

    stateCheckTypeB->addTransition(ui->btnCOMPortConnect, SIGNAL(clicked(bool)), stateSerialPortClose); // при нажатии кнопки порта - закроем его
    stateCheckTypeB->addTransition(serialPort, SIGNAL(signalCriticalError()), stateSerialPortClose); // при критической ошибке в порту - закроем его
    stateCheckTypeB->addTransition(timeout, SIGNAL(timeout()), stateFirst); // по таймауту приёма вывалимся в начальное состояние
    stateCheckTypeB->addTransition(timerDelay, SIGNAL(timeout()), stateCheckTypeBPoll); // по таймауту задержки после режима - вывалимся в следующее состояние

    stateCheckTypeBPoll->addTransition(ui->btnCOMPortConnect, SIGNAL(clicked(bool)), stateSerialPortClose); // при нажатии кнопки порта - закроем его
    stateCheckTypeBPoll->addTransition(serialPort, SIGNAL(signalCriticalError()), stateSerialPortClose); // при критической ошибке в порту - закроем его
    stateCheckTypeBPoll->addTransition(timeout, SIGNAL(timeout()), stateFirst); // по таймауту приёма вывалимся в начальное состояние
    stateCheckTypeBPoll->addTransition(timerDelay0, SIGNAL(timeout()), stateCheckTypeBIdleUocPB); // по таймауту задержки после режима - вывалимся в следующее состояние
    stateCheckTypeBPoll->addTransition(timerDelay1, SIGNAL(timeout()), stateIdleCommand); // по таймауту задержки после режима - вывалимся в следующее состояние

    stateCheckTypeBIdleUocPB->addTransition(ui->btnCOMPortConnect, SIGNAL(clicked(bool)), stateSerialPortClose); // при нажатии кнопки порта - закроем его
    stateCheckTypeBIdleUocPB->addTransition(serialPort, SIGNAL(signalCriticalError()), stateSerialPortClose); // при критической ошибке в порту - закроем его
    stateCheckTypeBIdleUocPB->addTransition(timeout, SIGNAL(timeout()), stateFirst); // по таймауту приёма вывалимся в начальное состояние
    stateCheckTypeBIdleUocPB->addTransition(timerDelay, SIGNAL(timeout()), stateCheckUocPB); // по таймауту задержки после режима - вывалимся в следующее состояние

    stateCheckUocPB->addTransition(ui->btnCOMPortConnect, SIGNAL(clicked(bool)), stateSerialPortClose); // при нажатии кнопки порта - закроем его
    stateCheckUocPB->addTransition(serialPort, SIGNAL(signalCriticalError()), stateSerialPortClose); // при критической ошибке в порту - закроем его
    stateCheckUocPB->addTransition(timeout, SIGNAL(timeout()), stateFirst); // по таймауту приёма вывалимся в начальное состояние
    stateCheckUocPB->addTransition(timerDelay, SIGNAL(timeout()), stateCheckUocPBPoll); // по таймауту задержки после режима - вывалимся в следующее состояние

    stateCheckUocPBPoll->addTransition(ui->btnCOMPortConnect, SIGNAL(clicked(bool)), stateSerialPortClose); // при нажатии кнопки порта - закроем его
    stateCheckUocPBPoll->addTransition(serialPort, SIGNAL(signalCriticalError()), stateSerialPortClose); // при критической ошибке в порту - закроем его
    stateCheckUocPBPoll->addTransition(timeout, SIGNAL(timeout()), stateFirst); // по таймауту приёма вывалимся в начальное состояние
    stateCheckUocPBPoll->addTransition(timerDelay, SIGNAL(timeout()), stateIdleCommand); // по таймауту задержки после режима - вывалимся в следующее состояние

    // добавляем состояния в КА
    machine->addState(stateCheckPolarB);
    machine->addState(stateCheckPolarBPoll);
    machine->addState(stateCheckPolarBIdleTypeB);
    machine->addState(stateCheckPolarBIdleUocPB);
    machine->addState(stateCheckTypeB);
    machine->addState(stateCheckTypeBPoll);
    machine->addState(stateCheckTypeBIdleUocPB);
    machine->addState(stateCheckUocPB);
    machine->addState(stateCheckUocPBPoll);
}

// подготовка и запуск режима проверки типа батареи. выполнится при входе в состояние stateCheckPolarB
void MainWindow::prepareCheckBatteryType()
{
    prepareSendCommand("PolarB#", delay_command_after_start_before_request, &onstateCheckPolarB); // подготовка команды
    sendCommand(); // посылка команды
    ui->statusBar->showMessage(tr("Проверка типа подключенной батареи..."));
    // !!! тут бахнуть начало крутилочки прогресс-бара
    Log(tr("Проверка типа подключенной батареи"), "blue");
}

// режим собран, подготовка опроса полярности
void MainWindow::onstateCheckPolarB(QByteArray data)
{
    qDebug()<<"onstateCheckPolarB"<<data;
    // Здесь провести разбор строки!!!
    // и подготовить тут след режим
    prepareSendCommand("PolarB?#", delay_command_after_request_before_next, &onstateCheckPolarBPoll);
}

// Анализ определения полярности
void MainWindow::onstateCheckPolarBPoll(QByteArray data)
{
    qDebug()<<"onPolarBPoll" << data;
    QDataStream ds(data); // Для преобразования первых двух байт в unsigned short
    quint16 i=0;
    ds>>i;
    if(!i) // полярность прямая
    {
        qDebug()<<"polar straight"<<i;
        ::x=0;
        prepareSendCommand("IDLE#", delay_after_IDLE_before_other, &onstateCheckPolarBIdleTypeB);
        timerDelay0->start(delay_command_after_request_before_next);
    }
    if(i == 1) // полярность обратная
    {
        qDebug()<<"polar revers"<<i;
        ::x=1;
        prepareSendCommand("IDLE#", delay_after_IDLE_before_other, &onstateCheckPolarBIdleUocPB);
        timerDelay1->start(delay_command_after_request_before_next);
    }
}

// завершение режима и сборка следующего
void MainWindow::onstateCheckPolarBIdleTypeB(QByteArray data)
{
    qDebug()<<"onstateCheckPolarBIdleTypeB"<<data;
    prepareSendCommand("TypeB 28#", delay_command_after_start_before_request, &onstateCheckTypeB);
}

// завершение режима и сборка следующего
void MainWindow::onstateCheckPolarBIdleUocPB(QByteArray data)
{
    qDebug()<<"onstateCheckPolarBIdleUocPB"<<data;
    prepareSendCommand("UocPB#", delay_command_after_start_before_request, &onstateCheckUocPB);
}

// Собран режим проверки подтипа батареи, подготовка опроса подтипа батареи
void MainWindow::onstateCheckTypeB(QByteArray data)
{
    qDebug()<<"onstateCheckTypeB" << data;
    prepareSendCommand("TypeB?#", delay_command_after_request_before_next, &onstateCheckTypeBPoll);
}

// Анализ подтипа батареи
void MainWindow::onstateCheckTypeBPoll(QByteArray data)
{
    quint16 u=0, U=2500; // приличное напряжение. Взять из ини-файла
    QDataStream ds(data); // Для преобразования первых двух байт в unsigned short
    ds>>u;
    qDebug()<<"onstateCheckTypeBPoll" << data<<u;
    if(u > U) // если есть приличное напряжение,
    {
        //qDebug()<<"Battery 9ER20P-28";
        ::y=2;
        ui->statusBar->showMessage(::type[::x][::y]);
        // !!! где-то надо будет проверить соответствие подключенной батареи к выбранной, и остановить крутилочку прогресс-бара
        Log(::type[::x][::y], "blue");
        prepareSendIdleToFirstCommand(); // выход в первое состояние, конец алгоритма
        timerDelay1->start(delay_command_after_request_before_next); // тут тоже можно сократить timerDelay1 до timerDelay. но оставим для ясности
    }
    else // отстаётся батарея 9ER20P-20
    {
        prepareSendCommand("IDLE#", delay_after_IDLE_before_other, &onstateCheckTypeBIdleUocPB);
        timerDelay0->start(delay_command_after_request_before_next);
    }
}

// Завершение режима и сборка следующего, проверка напряжения БП УУТББ
void MainWindow::onstateCheckTypeBIdleUocPB(QByteArray data)
{
    qDebug()<<"onstateCheckTypeBIdleUocPB" << data;
    prepareSendCommand("UocPB#", delay_command_after_start_before_request, &onstateCheckUocPB);
}

// режим собран, подготовка опроса
void MainWindow::onstateCheckUocPB(QByteArray data)
{
    qDebug()<<"onstateCheckUocPB"<<data;
    prepareSendCommand("UocPB?#", delay_command_after_request_before_next, &onstateCheckUocPBPoll);
}

// Анализ наличия напряжения БП УУТББ
void MainWindow::onstateCheckUocPBPoll(QByteArray data)
{
    qDebug()<<"onstateCheckUocPBPoll"<<data;
    QDataStream ds(data); // Для преобразования первых двух байт в unsigned short
    quint16 i=0;
    ds>>i;
    if(i > 0) // если есть напряжение, то УУТББ
    {
        ::y=0;
    }
    else // иначе УУТББ нету
    {
        ::y=1;
    }
    prepareSendIdleToFirstCommand(); // выход в первое состояние, конец алгоритма
    ui->statusBar->showMessage(::type[::x][::y]);
    Log(::type[::x][::y], "blue");
    // !!! где-то надо будет проверить соответствие подключенной батареи к выбранной, и остановить крутилочку прогресс-бара
}
