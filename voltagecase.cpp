/*
 * КА характеризуется тем, что у него есть переходная ф-ия (слот), которая выполняется при входе (SIGNAL(entered())) в новое состояние.
 * Этой переходной функцией как правило является слот SLOT(sendCommand()), который отправляет команду в порт и взводит таймер таймаута ответа.
 *
 * При приёме ответной посылки из порта выполняется ф-ия "(this->*funcCommandAnswer)(data)", которая занимается разбором и анализом принятой инфы.
 * Этому указателю на ф-ию каждый раз подставляется некая конкретная ф-ия, в которой и решается, что делать дальше. Обычно там подготавливается
 * команда для следующей посылки ф-ией sendCommand(), которая выполнится при входе в следующее состояние КА.
 *
 * При входе в какой-нить режим, первая переходная ф-ия немного другая - подготавливает команду сбора режима и отсылает её.
 *
 * В mainwindow.h добавить слот, который выполнится при вхождении в сбор режима проверки/измерения:
 *
    // Для режима проверки напряжения на корпусе. Выполнится при нажатии на кнопку Старт проверки (1)
    void prepareVoltageCase(); ///< подготовка и запуск режима проверки напряжения на корпусе
  *
  * Добавить состояния проверок:
    // Проверка напряжения на корпусе
    /// Состояния КА проверки напряжения на корпусе
    QState *stateVoltageCase; ///< Посылка команды и ожидание окончания задержки после отсылки (для сбора режима в коробочке) "UcaseX#
    QState *stateVoltageCasePoll; ///< После приёма ответа подтверждения от коробочки о корректном сборе режима "UcaseX#OK - посылка опроса "Ucase?#"
    QState *stateVoltageCaseIdle; ///< Разобрать режим

  * Добавить ф-ию для добавления новых состояний в КА
    void machineAddVoltageCase(); ///< собрать КА для проверки напряжения на корпусе (voltagecase.cpp)
  * В этой ф-ии написать состояния, переходы и добавление состояний в КА
  *
  * Добавить ф-ии разбора принятой инфы
    void onstateVoltageCase(QByteArray data);
    void onstateVoltageCasePoll(QByteArray data);
    void onstateVoltageCaseIdle(QByteArray data);

  */

#include <QDebug>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "battery.h"

extern QVector<Battery> battery;

void MainWindow::machineAddVoltageCase()
{
    // Добавить в КА состояния
    stateVoltageCase = new QState;
    stateVoltageCase->setObjectName("stateVoltageCase");
    connect(stateVoltageCase, SIGNAL(entered()), this, SLOT(prepareVoltageCase()));

    stateVoltageCasePoll = new QState;
    stateVoltageCasePoll->setObjectName("stateVoltageCasePoll");
    connect(stateVoltageCasePoll, SIGNAL(entered()), this, SLOT(sendCommand()));

    stateVoltageCaseIdle = new QState;
    stateVoltageCaseIdle->setObjectName("stateVoltageCaseIdle");
    connect(stateVoltageCaseIdle, SIGNAL(entered()), this, SLOT(sendCommand()));

    // добавляем переходы состояний
    stateVoltageCase->addTransition(ui->btnCOMPortOpenClose, SIGNAL(clicked(bool)), stateSerialPortClose); // при нажатии кнопки порта - закроем его
    stateVoltageCase->addTransition(serialPort, SIGNAL(signalCriticalError()), stateSerialPortClose); // при критической ошибке в порту - закроем его
    stateVoltageCase->addTransition(timeout, SIGNAL(timeout()), stateFirst); // по таймауту приёма вывалимся в начальное состояние
    stateVoltageCase->addTransition(timerDelay, SIGNAL(timeout()), stateVoltageCasePoll); // по таймауту задержки после режима - вывалимся в следующее состояние

    stateVoltageCasePoll->addTransition(ui->btnCOMPortOpenClose, SIGNAL(clicked(bool)), stateSerialPortClose); // при нажатии кнопки порта - закроем его
    stateVoltageCasePoll->addTransition(serialPort, SIGNAL(signalCriticalError()), stateSerialPortClose); // при критической ошибке в порту - закроем его
    stateVoltageCasePoll->addTransition(timeout, SIGNAL(timeout()), stateFirst); // по таймауту приёма вывалимся в начальное состояние
    stateVoltageCasePoll->addTransition(timerDelay, SIGNAL(timeout()), stateVoltageCaseIdle); // по таймауту задержки после режима - вывалимся в следующее состояние

    // в ф-ии обработки ответа опроса заведём разные таймеры в зависимости от полученных данных
    //stateVoltageCasePoll->addTransition(timerDelay0, SIGNAL(timeout()), stateVoltageCaseIdle); // по таймауту задержки после режима - вывалимся в следующее состояние
    //stateVoltageCasePoll->addTransition(timerDelay1, SIGNAL(timeout()), stateVoltageCaseIdle); // по таймауту задержки после режима - вывалимся в следующее состояние

    stateVoltageCaseIdle->addTransition(ui->btnCOMPortOpenClose, SIGNAL(clicked(bool)), stateSerialPortClose); // при нажатии кнопки порта - закроем его
    stateVoltageCaseIdle->addTransition(serialPort, SIGNAL(signalCriticalError()), stateSerialPortClose); // при критической ошибке в порту - закроем его
    stateVoltageCaseIdle->addTransition(timeout, SIGNAL(timeout()), stateFirst); // по таймауту приёма вывалимся в начальное состояние
    //stateVoltageCaseIdle->addTransition(timerDelay, SIGNAL(timeout()), stateIdleCommand); // по таймауту задержки после режима - вывалимся в следующее состояние
    stateVoltageCaseIdle->addTransition(stateFirst);

    // добавляем состояния в КА
    machine->addState(stateVoltageCase);
    machine->addState(stateVoltageCasePoll);
    machine->addState(stateVoltageCaseIdle);

}

QString command;
// выполнится при входе в состояние stateVoltageCase
void MainWindow::prepareVoltageCase()
{
    qDebug()<<"prepareVoltageCase";
    qDebug()<<ui->rbModeDiagnosticAuto->isChecked()<<ui->rbModeDiagnosticManual->isChecked()<<ui->cbVoltageOnTheHousing->currentIndex();
    if(ui->rbModeDiagnosticManual->isChecked())// если в ручном режиме
    {
        if(ui->cbVoltageOnTheHousing->currentIndex() == 1) // если выбрана в комбобоксе такая цепь
        {
            command = "UcaseM#";
            prepareSendCommand(command, delay_command_after_start_before_request, &onstateVoltageCase); // подготовка команды, установка
        }
        else
        {
            command = "UcaseP#";
            prepareSendCommand(command, delay_command_after_start_before_request, &onstateVoltageCase); // подготовка команды
        }
        sendCommand(); // посылка команды
        ui->statusBar->showMessage(tr("Проверка напряжения на корпусе ..."));
        ui->progressBar->setValue(ui->progressBar->value()+1);
        Log(tr("Проверка напряжения на корпусе"), "blue");
    }
}

void MainWindow::onstateVoltageCase(QByteArray data)
{
    qDebug()<<"onstateVoltageCase"<<data;
    prepareSendCommand("Ucase?#", delay_command_after_request_before_next, &onstateVoltageCasePoll);
    ui->progressBar->setValue(ui->progressBar->value()+1);
}

quint16 u=0;

// Анализ напряжения
void MainWindow::onstateVoltageCasePoll(QByteArray data)
{
    qDebug()<<"onstateVoltageCasePoll" << data;
    QDataStream ds(data.left(2)); // Для преобразования первых двух байт в unsigned short
    ds>>u;
    qDebug()<<"onstateVoltageCasePoll" << data<<u<<(float)(u*coefADC2)<<"U"; // преобразовать код АЦП в напряжение.
    prepareSendCommand("IDLE#", delay_after_IDLE_before_other, &onstateVoltageCaseIdle);
    ui->progressBar->setValue(ui->progressBar->value()+1);
}

// завершение режима и сборка следующего
void MainWindow::onstateVoltageCaseIdle(QByteArray data)
{
    qDebug()<<"onstateVoltageCaseIdle"<<data;
    quint16 Ucase=1.0/coefADC2; // пороговое напряжение. Взять из ини-файла;
    //prepareSendCommand("xxx#", delay_command_after_start_before_request, &onstateCheckTypeB);
    ui->progressBar->setValue(ui->progressBar->value()+1);
    if(u > Ucase) // напряжение больше
    {
        qDebug()<<command<<" > "<<u;
        //prepareSendCommand("IDLE#", delay_after_IDLE_before_other, &onstateVoltageCaseIdle);
        //timerDelay0->start(delay_command_after_request_before_next);
        Log("Напряжение цепи "+battery[iBatteryIndex].str_voltage_corpus[ui->cbVoltageOnTheHousing->currentIndex()]+" = "+QString::number((float)(u*coefADC2))+" В. Не норма!", "red"); // !!! отформатировать вывод напряжения!
        if(ui->rbModeDiagnosticAuto->isChecked())// если в автоматическом режиме
        {
            // !!! переход в ручной режим
            QMessageBox::critical(this, "Не норма!", "Напряжение цепи "+battery[iBatteryIndex].str_voltage_corpus[ui->cbVoltageOnTheHousing->currentIndex()]+" = "+QString::number((float)(u*coefADC2))+" В больше нормы");// !!!
        }
    }
    if(u <= Ucase) // напряжение в норме
    {
        qDebug()<<command<<" norm"<<u;
        Log("Напряжение цепи "+battery[iBatteryIndex].str_voltage_corpus[ui->cbVoltageOnTheHousing->currentIndex()]+" = "+QString::number((float)(u*coefADC2))+" В.  Норма.", "blue"); // !!!
        //prepareSendIdleToFirstCommand(); // подготовить идле
    }
}
