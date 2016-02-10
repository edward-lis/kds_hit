#include <QDebug>
#include <QMessageBox>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "battery.h"

extern QVector<Battery> battery;

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

//QString type[2][3]={{"9ER20P-20 УУТББ", "9ER20P-20", "9ER20P-28"},{"9ЕR14PS-24 УУТББ", "9ЕR14PS-24 или 9ER14P-24", "???"}};
//QString type[2][3]={{battery[0].str_type_name+" УУТББ", battery[0].str_type_name, battery[3].str_type_name},{battery[1].str_type_name+" УУТББ", battery[1].str_type_name+" или "+battery[2].str_type_name, "???"}};
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

    stateCheckShowResult = new QState;
    stateCheckShowResult->setObjectName("stateCheckShowResult");
    connect(stateCheckShowResult, SIGNAL(entered()), this, SLOT(slotCheckBatteryDone()));

    // добавляем переходы состояний
    stateCheckPolarB->addTransition(ui->btnCOMPortOpenClose, SIGNAL(clicked(bool)), stateSerialPortClose); // при нажатии кнопки порта - закроем его
    stateCheckPolarB->addTransition(serialPort, SIGNAL(signalCriticalError()), stateSerialPortClose); // при критической ошибке в порту - закроем его
    stateCheckPolarB->addTransition(timeout, SIGNAL(timeout()), stateFirst); // по таймауту приёма вывалимся в начальное состояние
    stateCheckPolarB->addTransition(timerDelay, SIGNAL(timeout()), stateCheckPolarBPoll); // по таймауту задержки после режима - вывалимся в следующее состояние

    stateCheckPolarBPoll->addTransition(ui->btnCOMPortOpenClose, SIGNAL(clicked(bool)), stateSerialPortClose); // при нажатии кнопки порта - закроем его
    stateCheckPolarBPoll->addTransition(serialPort, SIGNAL(signalCriticalError()), stateSerialPortClose); // при критической ошибке в порту - закроем его
    stateCheckPolarBPoll->addTransition(timeout, SIGNAL(timeout()), stateFirst); // по таймауту приёма вывалимся в начальное состояние
    // в ф-ии обработки ответа опроса заведём разные таймеры в зависимости от полученных данных
    stateCheckPolarBPoll->addTransition(timerDelay0, SIGNAL(timeout()), stateCheckPolarBIdleTypeB); // по таймауту задержки после режима - вывалимся в следующее состояние
    stateCheckPolarBPoll->addTransition(timerDelay1, SIGNAL(timeout()), stateCheckPolarBIdleUocPB); // по таймауту задержки после режима - вывалимся в следующее состояние

    stateCheckPolarBIdleTypeB->addTransition(ui->btnCOMPortOpenClose, SIGNAL(clicked(bool)), stateSerialPortClose); // при нажатии кнопки порта - закроем его
    stateCheckPolarBIdleTypeB->addTransition(serialPort, SIGNAL(signalCriticalError()), stateSerialPortClose); // при критической ошибке в порту - закроем его
    stateCheckPolarBIdleTypeB->addTransition(timeout, SIGNAL(timeout()), stateFirst); // по таймауту приёма вывалимся в начальное состояние
    stateCheckPolarBIdleTypeB->addTransition(timerDelay, SIGNAL(timeout()), stateCheckTypeB); // по таймауту задержки после режима - вывалимся в следующее состояние

    stateCheckPolarBIdleUocPB->addTransition(ui->btnCOMPortOpenClose, SIGNAL(clicked(bool)), stateSerialPortClose); // при нажатии кнопки порта - закроем его
    stateCheckPolarBIdleUocPB->addTransition(serialPort, SIGNAL(signalCriticalError()), stateSerialPortClose); // при критической ошибке в порту - закроем его
    stateCheckPolarBIdleUocPB->addTransition(timeout, SIGNAL(timeout()), stateFirst); // по таймауту приёма вывалимся в начальное состояние
    stateCheckPolarBIdleUocPB->addTransition(timerDelay, SIGNAL(timeout()), stateCheckUocPB); // по таймауту задержки после режима - вывалимся в следующее состояние

    stateCheckTypeB->addTransition(ui->btnCOMPortOpenClose, SIGNAL(clicked(bool)), stateSerialPortClose); // при нажатии кнопки порта - закроем его
    stateCheckTypeB->addTransition(serialPort, SIGNAL(signalCriticalError()), stateSerialPortClose); // при критической ошибке в порту - закроем его
    stateCheckTypeB->addTransition(timeout, SIGNAL(timeout()), stateFirst); // по таймауту приёма вывалимся в начальное состояние
    stateCheckTypeB->addTransition(timerDelay, SIGNAL(timeout()), stateCheckTypeBPoll); // по таймауту задержки после режима - вывалимся в следующее состояние

    stateCheckTypeBPoll->addTransition(ui->btnCOMPortOpenClose, SIGNAL(clicked(bool)), stateSerialPortClose); // при нажатии кнопки порта - закроем его
    stateCheckTypeBPoll->addTransition(serialPort, SIGNAL(signalCriticalError()), stateSerialPortClose); // при критической ошибке в порту - закроем его
    stateCheckTypeBPoll->addTransition(timeout, SIGNAL(timeout()), stateFirst); // по таймауту приёма вывалимся в начальное состояние
    stateCheckTypeBPoll->addTransition(timerDelay0, SIGNAL(timeout()), stateCheckTypeBIdleUocPB); // по таймауту задержки после режима - вывалимся в следующее состояние
    stateCheckTypeBPoll->addTransition(timerDelay1, SIGNAL(timeout()), stateCheckShowResult); // по таймауту задержки после режима - вывалимся в следующее состояние

    stateCheckTypeBIdleUocPB->addTransition(ui->btnCOMPortOpenClose, SIGNAL(clicked(bool)), stateSerialPortClose); // при нажатии кнопки порта - закроем его
    stateCheckTypeBIdleUocPB->addTransition(serialPort, SIGNAL(signalCriticalError()), stateSerialPortClose); // при критической ошибке в порту - закроем его
    stateCheckTypeBIdleUocPB->addTransition(timeout, SIGNAL(timeout()), stateFirst); // по таймауту приёма вывалимся в начальное состояние
    stateCheckTypeBIdleUocPB->addTransition(timerDelay, SIGNAL(timeout()), stateCheckUocPB); // по таймауту задержки после режима - вывалимся в следующее состояние

    stateCheckUocPB->addTransition(ui->btnCOMPortOpenClose, SIGNAL(clicked(bool)), stateSerialPortClose); // при нажатии кнопки порта - закроем его
    stateCheckUocPB->addTransition(serialPort, SIGNAL(signalCriticalError()), stateSerialPortClose); // при критической ошибке в порту - закроем его
    stateCheckUocPB->addTransition(timeout, SIGNAL(timeout()), stateFirst); // по таймауту приёма вывалимся в начальное состояние
    stateCheckUocPB->addTransition(timerDelay, SIGNAL(timeout()), stateCheckUocPBPoll); // по таймауту задержки после режима - вывалимся в следующее состояние

    stateCheckUocPBPoll->addTransition(ui->btnCOMPortOpenClose, SIGNAL(clicked(bool)), stateSerialPortClose); // при нажатии кнопки порта - закроем его
    stateCheckUocPBPoll->addTransition(serialPort, SIGNAL(signalCriticalError()), stateSerialPortClose); // при критической ошибке в порту - закроем его
    stateCheckUocPBPoll->addTransition(timeout, SIGNAL(timeout()), stateFirst); // по таймауту приёма вывалимся в начальное состояние
    stateCheckUocPBPoll->addTransition(timerDelay, SIGNAL(timeout()), stateCheckShowResult); // по таймауту задержки после режима - вывалимся в следующее состояние

    stateCheckShowResult->addTransition(stateFirst); // из показа результатов вернёмся в первое состояние

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
    machine->addState(stateCheckShowResult);

    //connect(this, SIGNAL(signalCheckBatteryDone(int,int)), this, SLOT(slotCheckBatteryDone(int,int))); // показать месседжбокс по концу проверки
}

// подготовка и запуск режима проверки типа батареи. выполнится при входе в состояние stateCheckPolarB
void MainWindow::prepareCheckBatteryType()
{
    prepareSendCommand("Polar#", delay_command_after_start_before_request, &onstateCheckPolarB); // подготовка команды
    sendCommand(); // посылка команды
    ui->statusBar->showMessage(tr("Проверка типа подключенной батареи..."));
    // !!! тут бахнуть начало крутилочки прогресс-бара
    ui->progressBar->setMaximum(6);
    ui->progressBar->setValue(1);
    Log(tr("Проверка типа подключенной батареи"), "blue");
}

// режим собран, подготовка опроса полярности
void MainWindow::onstateCheckPolarB(QByteArray data)
{
    qDebug()<<"onstateCheckPolarB"<<data;
    // Здесь провести разбор строки!!!
    // и подготовить тут след режим
    prepareSendCommand("Polar?#", delay_command_after_request_before_next, &onstateCheckPolarBPoll);
    ui->progressBar->setValue(ui->progressBar->value()+1);
}

// Анализ определения полярности
void MainWindow::onstateCheckPolarBPoll(QByteArray data)
{
    qDebug()<<"onPolarBPoll" << data;
    // ниже времянка, патамушта байт QDataStream ds(data.left(2)); // Для преобразования первых двух байт в unsigned short
    QDataStream ds(data.left(1)); // Для преобразования первых двух байт в unsigned short
    quint16 i=0;
    ds>>i;
    qDebug()<<ds<<i;
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
    ui->progressBar->setValue(ui->progressBar->value()+1);
}

// завершение режима и сборка следующего
void MainWindow::onstateCheckPolarBIdleTypeB(QByteArray data)
{
    qDebug()<<"onstateCheckPolarBIdleTypeB"<<data;
    prepareSendCommand("TypeB 28#", delay_command_after_start_before_request, &onstateCheckTypeB);
    ui->progressBar->setValue(ui->progressBar->value()+1);
}

// завершение режима и сборка следующего
void MainWindow::onstateCheckPolarBIdleUocPB(QByteArray data)
{
    qDebug()<<"onstateCheckPolarBIdleUocPB"<<data;
    prepareSendCommand("UocPB#", delay_command_after_start_before_request, &onstateCheckUocPB);
    ui->progressBar->setValue(ui->progressBar->value()+1);
}

// Собран режим проверки подтипа батареи, подготовка опроса подтипа батареи
void MainWindow::onstateCheckTypeB(QByteArray data)
{
    qDebug()<<"onstateCheckTypeB" << data;
    prepareSendCommand("TypeB?#", delay_command_after_request_before_next, &onstateCheckTypeBPoll);
    ui->progressBar->setValue(ui->progressBar->value()+1);
}

// Анализ подтипа батареи
void MainWindow::onstateCheckTypeBPoll(QByteArray data)
{
    quint16 u=0, U=25.0/coefADC1; // приличное напряжение. Взять из ини-файла
    qDebug()<<"coef"<<coefADC1<<"U"<<(quint16)(25.0/coefADC1);
    QDataStream ds(data.left(2)); // Для преобразования первых двух байт в unsigned short
    ds>>u;
    qDebug()<<"onstateCheckTypeBPoll" << data<<u<<(float)(u*coefADC1)<<"U";
    if(u > U) // если есть приличное напряжение,
    {
        //qDebug()<<"Battery 9ER20P-28";
        ::y=2;
        //ui->statusBar->showMessage(::type[::x][::y]);
        // !!! где-то надо будет проверить соответствие подключенной батареи к выбранной, и остановить крутилочку прогресс-бара
        ui->progressBar->setValue(ui->progressBar->value()+1);
        //Log(::type[::x][::y], "blue");
        prepareSendIdleToFirstCommand(); // подготовить идле, перейти к слоту slotCheckBatteryDone
        timerDelay1->start(delay_command_after_request_before_next); // тут тоже можно сократить timerDelay1 до timerDelay. но оставим для ясности
    }
    else // отстаётся батарея 9ER20P-20
    {
        prepareSendCommand("IDLE#", delay_after_IDLE_before_other, &onstateCheckTypeBIdleUocPB);
        timerDelay0->start(delay_command_after_request_before_next);
    }
    ui->progressBar->setValue(ui->progressBar->value()+1);
}

// Завершение режима и сборка следующего, проверка напряжения БП УУТББ
void MainWindow::onstateCheckTypeBIdleUocPB(QByteArray data)
{
    qDebug()<<"onstateCheckTypeBIdleUocPB" << data;
    prepareSendCommand("UocPB#", delay_command_after_start_before_request, &onstateCheckUocPB);
    ui->progressBar->setValue(ui->progressBar->value()+1);
}

// режим собран, подготовка опроса
void MainWindow::onstateCheckUocPB(QByteArray data)
{
    qDebug()<<"onstateCheckUocPB"<<data;
    prepareSendCommand("UocPB?#", delay_command_after_request_before_next, &onstateCheckUocPBPoll);
    ui->progressBar->setValue(ui->progressBar->value()+1);
}

// Анализ наличия напряжения БП УУТББ
void MainWindow::onstateCheckUocPBPoll(QByteArray data)
{
    qDebug()<<"onstateCheckUocPBPoll"<<data;
    quint16 u=0, U=5.0/coefADC1; // напряжение БП. Взять из ини-файла
    qDebug()<<"coef"<<coefADC1<<"U"<<(quint16)(5.0/coefADC1);
    QDataStream ds(data.left(2)); // Для преобразования первых двух байт в unsigned short
    ds>>u;
    if(u > U) // если есть напряжение, то УУТББ
    {
        ::y=0;
    }
    else // иначе УУТББ нету
    {
        ::y=1;
    }
    prepareSendIdleToFirstCommand(); // подготовить идле, перейти к слоту slotCheckBatteryDone
    ui->progressBar->setValue(ui->progressBar->value()+1);
//    ui->statusBar->showMessage(::type[::x][::y]);
//    Log(::type[::x][::y], "blue");
    // проверить соответствие подключенной батареи к выбранной
    //signalCheckBatteryDone(::x, ::y);
}

// бахнуть месседжбокс об несоответствии подключенной и выбранной батарей
void MainWindow::slotCheckBatteryDone()
{
    sendCommand(); // посылка команды (IDLE)
    qDebug()<<"slotCheckBatteryDone";
    int x=::x;
    int y=::y;

    if((0==x) && (0==y)) // 9ER20P-20 УУТББ
    {
        if(!((ui->comboBoxBatteryList->currentIndex()==0) && ui->cbIsUUTBB->isChecked()))
        {
            Log("Подключена батарея "+battery[0].str_type_name+" УУТББ, но выбрана "+ui->comboBoxBatteryList->currentText()+(ui->cbIsUUTBB->isChecked()?" УУТББ":""), "red");
            QMessageBox::information(this, "Проверка подключенной батареи", "Подключенная батарея "+battery[0].str_type_name+" УУТББ не соответствует выбранной");
        }
        else
        {
            Log("Подключена батарея "+battery[0].str_type_name+" УУТББ", "blue");
        }
    }
    if((0==x) && (1==y)) // 9ER20P-20
    {
        if(!((ui->comboBoxBatteryList->currentIndex()==0) && !ui->cbIsUUTBB->isChecked()))
        {
            Log("Подключена батарея "+battery[0].str_type_name+", но выбрана "+ui->comboBoxBatteryList->currentText()+(ui->cbIsUUTBB->isChecked()?" УУТББ":""), "red");
            QMessageBox::information(this, "Проверка подключенной батареи", "Подключенная батарея "+battery[0].str_type_name+" не соответствует выбранной");
        }
        else
        {
            Log("Подключена батарея "+battery[0].str_type_name, "blue");
        }
    }
    if((1==x) && (0==y)) // 9ЕR14PS-24 УУТББ
    {
        if(!((ui->comboBoxBatteryList->currentIndex()==1) && ui->cbIsUUTBB->isChecked()))
        {
            Log("Подключена батарея "+battery[1].str_type_name+" УУТББ, но выбрана "+ui->comboBoxBatteryList->currentText()+(ui->cbIsUUTBB->isChecked()?" УУТББ":""), "red");
            QMessageBox::information(this, "Проверка подключенной батареи", "Подключенная батарея "+battery[1].str_type_name+" УУТББ не соответствует выбранной");
        }
        else
        {
            Log("Подключена батарея "+battery[1].str_type_name+" УУТББ", "blue");
        }
    }
    if((1==x) && (1==y)) //9ЕR14PS-24 или 9ER14P-24
    {
        if(!(((ui->comboBoxBatteryList->currentIndex()==1) || (ui->comboBoxBatteryList->currentIndex()==2)) && !ui->cbIsUUTBB->isChecked()))
        {
            Log("Подключена батарея "+battery[1].str_type_name+" или "+battery[2].str_type_name+", но выбрана "+ui->comboBoxBatteryList->currentText()+(ui->cbIsUUTBB->isChecked()?" УУТББ":""), "red");
            QMessageBox::information(this, "Проверка подключенной батареи", "Подключенная батарея "+battery[1].str_type_name+" или "+battery[2].str_type_name+" не соответствует выбранной");
        }
        else
        {
            Log("Подключена батарея "+battery[1].str_type_name+" или "+battery[2].str_type_name, "blue");
            QMessageBox::information(this, "Проверка подключенной батареи", "Подключена батарея "+battery[1].str_type_name+" или "+battery[2].str_type_name+"!");
        }
    }
    if(2==y)
    {
        if(ui->comboBoxBatteryList->currentIndex()!=3) // если выбранная батарея не соответствует подключенной
        {
            Log("Подключена батарея "+battery[3].str_type_name+", но выбрана "+ui->comboBoxBatteryList->currentText()+(ui->cbIsUUTBB->isChecked()?" УУТББ":""), "red");
            QMessageBox::information(this, "Проверка подключенной батареи", "Подключенная батарея "+battery[3].str_type_name+" не соответствует выбранной");
        }
        else
        {
            Log("Подключена батарея "+battery[3].str_type_name, "blue");
        }
    }
}
