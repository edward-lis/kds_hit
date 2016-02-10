#include <QDebug>
#include "mainwindow.h"
#include "ui_mainwindow.h"

/*
 * В начальном состоянии КА находится в состоянии закрытого порта. При нажатии на кнопку Открыть порт, переходит в состояние открытия порта.
 * Если порт не открылся, то возвращается в состояние закрытого порта.
 * Если порт открылся без ошибки, то переходит в состояние готовности №1 и начинает посылать в порт пинг.
 * В 1 состоянии, если нажата кнопка закрыть порт, то возвращается в начальное состояние закрытого порта.
 * Если произошла критическая ошибка в порту, то возвращается в начальное состояние закрытого порта.
 * Если пришёл ответ на пинг, то для начала разово посылается команда IDLE сброса коробочки, переходит в состояние посылки IDLE.
 * В состоянии посылки idle:
 * Если ответ (квитанция) на команду неправильный, то ...(ещё не доделано, подумать). Испускатся сигнал signalWrongReply.
 * Если ответа нет, то в 1 состояние, и продолжается попытка посылки idle.
 * Если ответ на команду нормальный, то разрешается кнопка проверки типа батареи и возвращается в 1 состояние.
 * Если 1 состоянии теперь нажата кнопка Проверки типа батареи, то переходит в состояние Проверка типа батареи.
 */

// Настроить КА
void MainWindow::setupMachine()
{
    // Конечный автомат
    machine = new QStateMachine(this);
    // добавляем состояния
    // исходное состояние.  последовательный порт закрыт, кнопка порта "открыть", остальные кнопки отключены.
    stateSerialPortClose = new QState();
    stateSerialPortClose->setObjectName("stateSerialPortClose");
    connect(stateSerialPortClose, SIGNAL(entered()), this, SLOT(enterStateSerialPortClose())); // закроем порт

    // состояние открытия порта.
    stateSerialPortOpening = new QState;
    stateSerialPortOpening->setObjectName("stateSerialPortOpening");
    connect(stateSerialPortOpening, SIGNAL(entered()), this, SLOT(enterStateSerialPortOpening())); // при в входе в состояние, выполним ф-ию открытия порта

    // порт открыт, посылается пинг для проверки связи. после установления связи один раз посылается команда сброса коробочки.
    stateFirst = new QState;
    stateFirst->setObjectName("stateFirst");
    connect(stateFirst, SIGNAL(entered()), this, SLOT(enter1State()));

    // состояние посылки команды IDLE.  анализируется ответ.
    stateIdleCommand = new QState;
    stateIdleCommand->setObjectName("stateIdleCommand");
    connect(stateIdleCommand, SIGNAL(entered()), this, SLOT(sendCommand()));

    // добавить в КА режим проверки типа подключенной батареи (checkbatterytype.cpp)
    machineAddCheckBatteryType();
    // добавить в КА режим проверки напряжения на корпусе (voltagecase.cpp)
    machineAddVoltageCase();

    // добавляем переходы состояний
    stateSerialPortClose->addTransition(ui->btnCOMPortOpenClose, SIGNAL(clicked(bool)), stateSerialPortOpening); // при нажатии на кнопку порта - открыть

    stateSerialPortOpening->addTransition(this, SIGNAL(signalSerialPortOpened()), stateFirst);  // если открылся, то перейти в первое состояние
    stateSerialPortOpening->addTransition(this, SIGNAL(signalSerialPortErrorOpened()), stateSerialPortClose); // если не открылся, то остаться в закрытом состоянии

    stateFirst->addTransition(ui->btnCOMPortOpenClose, SIGNAL(clicked(bool)), stateSerialPortClose); // при нажатии кнопки порта - закроем его
    stateFirst->addTransition(serialPort, SIGNAL(signalCriticalError()), stateSerialPortClose); // при критической ошибке в порту - закроем его
    stateFirst->addTransition(this, SIGNAL(workStart()), stateIdleCommand); // при первом пинге в начале работы послать ИДЛЕ для сброса коробочки
    stateFirst->addTransition(ui->btnCheckConnectedBattery, SIGNAL(clicked(bool)), stateCheckPolarB); // нажата кнопка проверки типа батареи
    stateFirst->addTransition(ui->btnVoltageOnTheHousing, SIGNAL(clicked(bool)), stateVoltageCase); // нажата кнопка проверки напряжения на корпусе

    stateIdleCommand->addTransition(ui->btnCOMPortOpenClose, SIGNAL(clicked(bool)), stateSerialPortClose); // при нажатии кнопки порта - закроем его
    stateIdleCommand->addTransition(serialPort, SIGNAL(signalCriticalError()), stateSerialPortClose); // при критической ошибке в порту - закроем его
    stateIdleCommand->addTransition(timeout, SIGNAL(timeout()), stateFirst); // по таймауту приёма вывалимся в начальное состояние
    stateIdleCommand->addTransition(timerDelay, SIGNAL(timeout()), stateFirst); // по таймауту задержки после режима - вывалимся в следующее состояние

    // добавляем состояния в КА
    machine->addState(stateSerialPortClose);
    machine->addState(stateSerialPortOpening);
    machine->addState(stateFirst);
    machine->addState(stateIdleCommand);
    // начальное состояние
    machine->setInitialState(stateSerialPortClose);
    // запуск КА
    machine->start();

}

// вошли в состояние закрытого порта
void MainWindow::enterStateSerialPortClose()
{
    if(serialPort)  // если есть объект последовательного порта,
        serialPort->closePort(); // то закрыть его
    ui->btnCOMPortOpenClose->setText(tr("Открыть")); // в этом состоянии напишем такие буквы на кнопке
    ui->comboBoxCOMPort->setEnabled(true); // и разрешим комбобокс выбора порта
    ui->statusBar->showMessage(tr("Порт закрыт"));
    Log(tr("[COM Порт] - Порт закрыт"), "red");
    ui->groupBoxDiagnosticMode->setDisabled(true);
    ui->groupBoxDiagnosticDevice->setDisabled(true);
    ui->groupBoxCheckParams->setDisabled(true);
    ui->btnStartAutoModeDiagnostic->setDisabled(true);
    //ui->btnCheckConnectedBattery->setEnabled(false); // запретим тыцкать в проверку батареи


    timerPing->stop(); // остановим таймеры.  пинга нет
    timeout->stop(); //
    start_work=true; // для первого ИДЛЕ признак
}

// вошли в состояние окрытия последовательного порта
void MainWindow::enterStateSerialPortOpening()
{
    if(ui->comboBoxCOMPort->currentText().isEmpty()) // последовательных портов в системе нет, состояние КА не меняется.
    {
        ui->statusBar->showMessage(tr("Нет последовательных портов"));
        Log(tr("[COM Порт] - Порт закрыт"), "red");
        ui->groupBoxDiagnosticDevice->setEnabled(false);
        ui->groupBoxDiagnosticMode->setEnabled(false);
        ui->groupBoxCheckParams->setEnabled(false);
        ui->btnStartAutoModeDiagnostic->setEnabled(false);
        return;
    }
    if(serialPort->openPort(ui->comboBoxCOMPort->currentText())) // открыть порт. если он открылся нормально
    {
        // порт открыт
        ui->statusBar->showMessage(tr("Порт %1 открыт").arg(serialPort->serial->portName()));
        Log(tr("[COM Порт] - Порт %1 открыт").arg(ui->comboBoxCOMPort->currentText()), "green");
        ui->btnCOMPortOpenClose->setText(tr("Закрыть")); // в этом состоянии напишем такие буквы на кнопке
        ui->comboBoxCOMPort->setEnabled(false); // и запретим выбор ком-порта
        ui->groupBoxDiagnosticDevice->setEnabled(true);
        ui->groupBoxDiagnosticMode->setEnabled(true);
        ui->groupBoxCheckParams->setEnabled(true);
        ui->btnStartAutoModeDiagnostic->setEnabled(true);
        emit signalSerialPortOpened(); // сигнал КА, что порт открылся
    }
    else // если порт не открылся
    {
        QMessageBox::critical(this, tr("Ошибка"), serialPort->serial->errorString()); // показать текст ошибки
        emit signalSerialPortErrorOpened(); // сигнал КА, что порт не смог открыться
    }
}

// Вошли в первое состояние (готовности к работе, порт открыт)
void MainWindow::enter1State()
{
    qDebug()<<"enter1State";
    timerPing->start(delay_timerPing); //при входе в режим ожидания действий, по окончанию таймера выполнится посылка пинга
}

// Коробочка после установления связи в первый раз сбросилась в состояние IDLE. типа callback, переходная ф-ия.
void MainWindow::onIdleOK(QByteArray data)
{
    qDebug()<<"onIdleOK" << data;
    if(start_work)
    {
        start_work=false; // один раз сбросили по началу работы коробочку в режим простоя, и хорош.
        ui->btnCheckConnectedBattery->setEnabled(true); // т.к. коробочка на связи и сбросилась в исходное, то разрешим кнопочку "Проверить батарею"
    }
}

