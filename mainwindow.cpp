#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    iBatteryCurrentIndex = 0;
    iDiagnosticModeCurrentIndex = 0;
    ResetCheck();
    fillPortsInfo();
    com = new QSerialPort(this);
    connect(ui->btnCOMPortConnect, SIGNAL(clicked(bool)), this, SLOT(openCOMPort()));
    connect(ui->btnCOMPortDisconnect, SIGNAL(clicked(bool)), this, SLOT(closeCOMPort()));
    connect(ui->btnCOMPortReadData, SIGNAL(clicked(bool)), this, SLOT(readData()));
    connect(ui->btnCOMPortSendCommand, SIGNAL(clicked(bool)), this, SLOT(writeData()));
    connect(ui->btnStartCheck, SIGNAL(clicked()), this, SLOT(CheckBattery()));
    connect(ui->comboBoxBatteryList, SIGNAL(currentIndexChanged(int)), this , SLOT(handleSelectionChangedBattery(int)));
    connect(ui->comboBoxDiagnosticModeList, SIGNAL(currentIndexChanged(int)), this , SLOT(handleSelectionChangedDiagnosticMode(int)));
    connect(ui->menuReset, SIGNAL(triggered(bool)), this , SLOT(ResetCheck()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

/*
 * Проверка проверяемых параметров
 */
void MainWindow::paramCheck()
{
    iParamsNumberChecked = 0;
    paramMsg = "";
    if (ui->checkBoxVoltageOnTheHousing->isChecked())
        iParamsNumberChecked = 1;
    if (ui->checkBoxInsulationResistance->isChecked())
        iParamsNumberChecked = 2;
    if (ui->checkBoxBatteryOpenCircuitVoltageGroup->isChecked())
        iParamsNumberChecked = 3;
    if (ui->checkBoxBatteryClosedCircuitVoltageGroup->isChecked())
        iParamsNumberChecked = 4;
    if (ui->checkBoxClosedCircuitVoltage->isChecked())
        iParamsNumberChecked = 5;
    if (ui->checkBoxBatteryInsulationResistanceMeasuringBoardUUTBB->isChecked())
        iParamsNumberChecked = 6;
    if (ui->checkBoxBatteryOpenCircuitVoltagePowerSupply->isChecked())
        iParamsNumberChecked = 7;
    if (ui->checkBoxBatteryClosedCircuitVoltagePowerSupply->isChecked())
        iParamsNumberChecked = 8;

    if (iParamsNumberChecked > 0) {
        for (int i = 0; 0 < iParamsNumberChecked; iParamsNumberChecked--) {
            switch (i++) {
            case 1:
                if (!ui->checkBoxVoltageOnTheHousing->isChecked())
                    paramMsg += " - Напряжение на корпусе\n";
                break;
            case 2:
                if (!ui->checkBoxInsulationResistance->isChecked())
                    paramMsg += " - Сопротивление изоляции\n";
                break;
            case 3:
                if (!ui->checkBoxBatteryOpenCircuitVoltageGroup->isChecked())
                    paramMsg += " - Напряжение разомкнутой цепи группы\n";
                break;
            case 4:
                if (!ui->checkBoxBatteryClosedCircuitVoltageGroup->isChecked())
                    paramMsg += " - Напряжение замкнутой цепи группы\n";
                break;
            case 5:
                if (!ui->checkBoxClosedCircuitVoltage->isChecked())
                    paramMsg += " - Напряжение замкнутой цепи батареи\n";
                break;
            case 6:
                if (!ui->checkBoxBatteryInsulationResistanceMeasuringBoardUUTBB->isChecked())
                    paramMsg += " - Сопротивление изоляции платы измерительной УУТББ\n";
                break;
            case 7:
                if (!ui->checkBoxBatteryOpenCircuitVoltagePowerSupply->isChecked())
                    paramMsg += " - Напряжение разомкнутой цепи блока питания\n";
                break;
            case 8:
                if (!ui->checkBoxBatteryClosedCircuitVoltagePowerSupply->isChecked())
                    paramMsg += " - Напряжение замкнутой цепи блока питания";
                break;
            default:
                break;
            }
        }
        if (paramMsg.length() > 0) {
            iStartCheck = QMessageBox::question(this, "Внимание", "Вы уверны что хотите пропустить следующие этапы проверки?\n"+paramMsg, tr("Да"), tr("Нет"));
        }
    } else {
        iStartCheck = QMessageBox::information(this, "Внимание", "Вы должны выбрать проверяемый параметр.");
    }
}

/*
 * Прогресс бар шаг вперед
 */
void MainWindow::progressBarSet(int iVal)
{
    ui->progressBar->setValue(ui->progressBar->value()+iVal);
}

/*
 * Прогресс бар установка максимума
 */
void MainWindow::progressBarSetMaximum()
{
    ui->progressBar->setValue(0);
    iProgressBarAllSteps = 0;
    switch (iBatteryCurrentIndex) {
    case 1: //Самодиагностика с помощью имитатора батареи
        if (ui->checkBoxVoltageOnTheHousing->isChecked())
            iProgressBarAllSteps += 1;
        if (ui->checkBoxInsulationResistance->isChecked())
            iProgressBarAllSteps += 1;
        if (ui->checkBoxBatteryOpenCircuitVoltageGroup->isChecked())
            iProgressBarAllSteps += 1;
        if (ui->checkBoxBatteryClosedCircuitVoltageGroup->isChecked())
            iProgressBarAllSteps += 1;
        if (ui->checkBoxClosedCircuitVoltage->isChecked())
            iProgressBarAllSteps += 1;
        if (ui->checkBoxBatteryInsulationResistanceMeasuringBoardUUTBB->isChecked())
            iProgressBarAllSteps += 1;
        if (ui->checkBoxBatteryOpenCircuitVoltagePowerSupply->isChecked())
            iProgressBarAllSteps += 1;
        if (ui->checkBoxBatteryClosedCircuitVoltagePowerSupply->isChecked())
            iProgressBarAllSteps += 1;
        break;
    case 2: //9ER20P-20
        if (ui->checkBoxVoltageOnTheHousing->isChecked())
            iProgressBarAllSteps += 2;
        if (ui->checkBoxInsulationResistance->isChecked())
            iProgressBarAllSteps += 4;
        if (ui->checkBoxBatteryOpenCircuitVoltageGroup->isChecked())
            iProgressBarAllSteps += 20;
        if (ui->checkBoxBatteryClosedCircuitVoltageGroup->isChecked())
            iProgressBarAllSteps += 60;
        if (ui->checkBoxClosedCircuitVoltage->isChecked())
            iProgressBarAllSteps += 1;
        break;
    case 3: //9ER20P-20 (УУТББ)
        if (ui->checkBoxVoltageOnTheHousing->isChecked())
            iProgressBarAllSteps += 2;
        if (ui->checkBoxInsulationResistance->isChecked())
            iProgressBarAllSteps += 4;
        if (ui->checkBoxBatteryOpenCircuitVoltageGroup->isChecked())
            iProgressBarAllSteps += 20;
        if (ui->checkBoxBatteryClosedCircuitVoltageGroup->isChecked())
            iProgressBarAllSteps += 60;
        if (ui->checkBoxClosedCircuitVoltage->isChecked())
            iProgressBarAllSteps += 1;
        if (ui->checkBoxBatteryInsulationResistanceMeasuringBoardUUTBB->isChecked())
            iProgressBarAllSteps += 1;
        if (ui->checkBoxBatteryOpenCircuitVoltagePowerSupply->isChecked())
            iProgressBarAllSteps += 1;
        if (ui->checkBoxBatteryClosedCircuitVoltagePowerSupply->isChecked())
            iProgressBarAllSteps += 1;
        break;
    case 4: //9ER20P-28
        if (ui->checkBoxVoltageOnTheHousing->isChecked())
            iProgressBarAllSteps += 2;
        if (ui->checkBoxInsulationResistance->isChecked())
            iProgressBarAllSteps += 4;
        if (ui->checkBoxBatteryOpenCircuitVoltageGroup->isChecked())
            iProgressBarAllSteps += 28;
        if (ui->checkBoxBatteryClosedCircuitVoltageGroup->isChecked())
            iProgressBarAllSteps += 84;
        if (ui->checkBoxClosedCircuitVoltage->isChecked())
            iProgressBarAllSteps += 1;
        break;
    case 5: //9ER14P-24
        if (ui->checkBoxVoltageOnTheHousing->isChecked())
            iProgressBarAllSteps += 2;
        if (ui->checkBoxInsulationResistance->isChecked())
            iProgressBarAllSteps += 2;
        if (ui->checkBoxBatteryOpenCircuitVoltageGroup->isChecked())
            iProgressBarAllSteps += 24;
        if (ui->checkBoxBatteryClosedCircuitVoltageGroup->isChecked())
            iProgressBarAllSteps += 72;
        if (ui->checkBoxClosedCircuitVoltage->isChecked())
            iProgressBarAllSteps += 1;
        break;
    case 6: //9ER14PS-24
        if (ui->checkBoxVoltageOnTheHousing->isChecked())
            iProgressBarAllSteps += 2;
        if (ui->checkBoxInsulationResistance->isChecked())
            iProgressBarAllSteps += 2;
        if (ui->checkBoxBatteryOpenCircuitVoltageGroup->isChecked())
            iProgressBarAllSteps += 24;
        if (ui->checkBoxBatteryClosedCircuitVoltageGroup->isChecked())
            iProgressBarAllSteps += 72;
        if (ui->checkBoxClosedCircuitVoltage->isChecked())
            iProgressBarAllSteps += 1;
        break;
    case 7: //9ER14PS-24 (УУТББ)
        if (ui->checkBoxVoltageOnTheHousing->isChecked())
            iProgressBarAllSteps += 2;
        if (ui->checkBoxInsulationResistance->isChecked())
            iProgressBarAllSteps += 2;
        if (ui->checkBoxBatteryOpenCircuitVoltageGroup->isChecked())
            iProgressBarAllSteps += 24;
        if (ui->checkBoxBatteryClosedCircuitVoltageGroup->isChecked())
            iProgressBarAllSteps += 72;
        if (ui->checkBoxClosedCircuitVoltage->isChecked())
            iProgressBarAllSteps += 1;
        if (ui->checkBoxBatteryInsulationResistanceMeasuringBoardUUTBB->isChecked())
            iProgressBarAllSteps += 1;
        if (ui->checkBoxBatteryOpenCircuitVoltagePowerSupply->isChecked())
            iProgressBarAllSteps += 1;
        if (ui->checkBoxBatteryClosedCircuitVoltagePowerSupply->isChecked())
            iProgressBarAllSteps += 1;
        break;
    default:
        break;
    }
    ui->progressBar->setMaximum(iProgressBarAllSteps);
}

/*
 * COM Порт получения списка портов
 */
void MainWindow::fillPortsInfo()
{
    ui->comboBoxCOMPort->clear();
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        QStringList list;
        list << info.portName();
        ui->comboBoxCOMPort->addItems(list);
    }
}

/*
 * COM Порт установка соединения
 */
void MainWindow::openCOMPort()
{
    com->setPortName(ui->comboBoxCOMPort->currentText());
    com->setBaudRate(QSerialPort::Baud115200);
    com->setDataBits(QSerialPort::Data8);
    com->setParity(QSerialPort::NoParity);
    com->setStopBits(QSerialPort::OneStop);
    com->setFlowControl(QSerialPort::NoFlowControl);
    if (com->open(QIODevice::ReadWrite)) {
        setEnabled(true);
        Log(tr("[COM Порт] - Соединение установлено на порт %1.").arg(ui->comboBoxCOMPort->currentText()), "green");
        foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
            if (info.portName() == ui->comboBoxCOMPort->currentText()) {
                Log(tr("[COM Порт] - Описание: %1.").arg(info.description()), "blue");
            }
        }
    } else {
        Log("[COM Порт] - Ошибка устройства: "+com->errorString(), "red");
    }
}

/*
 * COM Порт запись данных
 */
void MainWindow::writeData()
{
    QByteArray data;
    data.append(ui->lineEditCOMPortCommand->text());
    com->write(data);
    Log("[COM Порт] - Отправка: "+ui->lineEditCOMPortCommand->text(), "green");
}

/*
 * COM Порт чтение данных
 */
void MainWindow::readData()
{
    QByteArray data = com->readAll();
    Log("[COM Порт] - Получение: "+QString(data), "blue");
}

/*
 * COM Порт отключение
 */
void MainWindow::closeCOMPort()
{
    if (com->isOpen())
        com->close();
    setEnabled(false);
    Log("[COM Порт] - Соединение разъединено.", "red");
}

/*
 * Включение отключение элементов
 */
void MainWindow::setEnabled(bool flag)
{
    if(com->isOpen()) {
        ui->btnCOMPortDisconnect->setEnabled(true);
        ui->btnCOMPortConnect->setEnabled(false);
        ui->comboBoxCOMPort->setEnabled(false);
    } else {
        ui->btnCOMPortDisconnect->setEnabled(false);
        ui->btnCOMPortConnect->setEnabled(true);
        ui->comboBoxCOMPort->setEnabled(true);
    }

    ui->comboBoxBatteryList->setEnabled(flag);
    if(iBatteryCurrentIndex > 0) {
        ui->dateEditBatteryBuild->setEnabled(flag);
        ui->lineEditBatteryNumber->setEnabled(flag);
        ui->comboBoxDiagnosticModeList->setEnabled(flag);
        if(iDiagnosticModeCurrentIndex > 0) {
            ui->checkBoxVoltageOnTheHousing->setEnabled(flag);
            ui->checkBoxInsulationResistance->setEnabled(flag);
            ui->checkBoxBatteryOpenCircuitVoltageGroup->setEnabled(flag);
            ui->checkBoxBatteryClosedCircuitVoltageGroup->setEnabled(flag);
            ui->checkBoxClosedCircuitVoltage->setEnabled(flag);
            ui->checkBoxBatteryInsulationResistanceMeasuringBoardUUTBB->setEnabled(flag);
            ui->checkBoxBatteryOpenCircuitVoltagePowerSupply->setEnabled(flag);
            ui->checkBoxBatteryClosedCircuitVoltagePowerSupply->setEnabled(flag);
        }
        ui->btnStartCheck->setEnabled(flag);
    }
}

/*
 * Сброс параметров проверки
 */
void MainWindow::ResetCheck()
{
    ui->labelVoltageOnTheHousing1->clear();
    ui->labelVoltageOnTheHousing2->clear();
    ui->labelInsulationResistance1->clear();
    ui->labelInsulationResistance2->clear();
    ui->labelInsulationResistance3->clear();
    ui->labelInsulationResistance4->clear();
    ui->labelClosedCircuitVoltage->clear();
}

/*
 * Выбор батареи
 */
void MainWindow::handleSelectionChangedBattery(int index)
{
    if (index > 0) {
        ui->btnStartCheck->setEnabled(true);
        ui->dateEditBatteryBuild->setEnabled(true);
        ui->lineEditBatteryNumber->setEnabled(true);
        ui->comboBoxDiagnosticModeList->setEnabled(true);
        if (index == 1 or index == 3 or index == 7) {
            ui->checkBoxBatteryInsulationResistanceMeasuringBoardUUTBB->show();
            ui->checkBoxBatteryOpenCircuitVoltagePowerSupply->show();
            ui->checkBoxBatteryClosedCircuitVoltagePowerSupply->show();
        } else {
            ui->checkBoxBatteryInsulationResistanceMeasuringBoardUUTBB->hide();
            ui->checkBoxBatteryOpenCircuitVoltagePowerSupply->hide();
            ui->checkBoxBatteryClosedCircuitVoltagePowerSupply->hide();
        }
    } else {
        ui->btnStartCheck->setEnabled(false);
        ui->dateEditBatteryBuild->setEnabled(false);
        ui->lineEditBatteryNumber->setEnabled(false);
        ui->comboBoxDiagnosticModeList->setEditable(false);
    }

    iBatteryCurrentIndex = QString::number(index).toInt();
}

/*
 * Выбор режима диагностики
 */
void MainWindow::handleSelectionChangedDiagnosticMode(int index)
{
    if (index > 0) {
        ui->checkBoxVoltageOnTheHousing->setEnabled(true);
        ui->checkBoxInsulationResistance->setEnabled(true);
        ui->checkBoxBatteryOpenCircuitVoltageGroup->setEnabled(true);
        ui->checkBoxBatteryClosedCircuitVoltageGroup->setEnabled(true);
        ui->checkBoxClosedCircuitVoltage->setEnabled(true);
        ui->checkBoxBatteryInsulationResistanceMeasuringBoardUUTBB->setEnabled(true);
        ui->checkBoxBatteryOpenCircuitVoltagePowerSupply->setEnabled(true);
        ui->checkBoxBatteryClosedCircuitVoltagePowerSupply->setEnabled(true);
    } else {
        ui->checkBoxVoltageOnTheHousing->setChecked(true);
        ui->checkBoxVoltageOnTheHousing->setEnabled(false);
        ui->checkBoxInsulationResistance->setChecked(true);
        ui->checkBoxInsulationResistance->setEnabled(false);
        ui->checkBoxBatteryOpenCircuitVoltageGroup->setChecked(true);
        ui->checkBoxBatteryOpenCircuitVoltageGroup->setEnabled(false);
        ui->checkBoxBatteryClosedCircuitVoltageGroup->setChecked(true);
        ui->checkBoxBatteryClosedCircuitVoltageGroup->setEnabled(false);
        ui->checkBoxClosedCircuitVoltage->setChecked(true);
        ui->checkBoxClosedCircuitVoltage->setEnabled(false);
        ui->checkBoxBatteryInsulationResistanceMeasuringBoardUUTBB->setChecked(true);
        ui->checkBoxBatteryInsulationResistanceMeasuringBoardUUTBB->setEnabled(false);
        ui->checkBoxBatteryOpenCircuitVoltagePowerSupply->setChecked(true);
        ui->checkBoxBatteryOpenCircuitVoltagePowerSupply->setEnabled(false);
        ui->checkBoxBatteryClosedCircuitVoltagePowerSupply->setChecked(true);
        ui->checkBoxBatteryClosedCircuitVoltagePowerSupply->setEnabled(false);
    }
    iDiagnosticModeCurrentIndex = QString::number(index).toInt();
}

/*
 * Запись событий в журнал
 */
void MainWindow::Log(QString message, QString color)
{
    QTime time = QTime::currentTime();
    QString text = time.toString("hh:mm:ss.zzz") + " ";
    text = (color == "green" or color == "red" or color == "blue") ? text + "<font color=\""+color+"\">"+message+"</font>" : text + message;
    ui->EventLog->appendHtml(tr("%1").arg(text));
}

/*
 * Общая проверка батареи
 */
void MainWindow::CheckBattery()
{
    paramCheck();
    if (iStartCheck == 0) {
        Log("Начало проверки батареи: \"<b>"+ui->comboBoxBatteryList->currentText()+"</b>\" дата производства \"<b>"+ui->dateEditBatteryBuild->text()+"\"</b> номер батареи \"<b>"+ui->lineEditBatteryNumber->text()+"</b>\".", "def");
        setEnabled(false);
        ui->btnStopCheck->setEnabled(true);
        ui->btnCOMPortDisconnect->setEnabled(false);
        progressBarSetMaximum();
        if (ui->checkBoxVoltageOnTheHousing->isChecked())
            CheckBatteryVoltageOnTheHousing(QString::number(iBatteryCurrentIndex).toInt());
        if (ui->checkBoxInsulationResistance->isChecked())
            CheckBatteryInsulationResistance(QString::number(iBatteryCurrentIndex).toInt());
        if (ui->checkBoxBatteryOpenCircuitVoltageGroup->isChecked())
            CheckBatteryOpenCircuitVoltageGroup(QString::number(iBatteryCurrentIndex).toInt());
        if (ui->checkBoxBatteryClosedCircuitVoltageGroup->isChecked())
            CheckBatteryClosedCircuitVoltageGroup(QString::number(iBatteryCurrentIndex).toInt());
        if (ui->checkBoxClosedCircuitVoltage->isChecked())
            CheckBatteryClosedCircuitVoltage(QString::number(iBatteryCurrentIndex).toInt());
        if (iBatteryCurrentIndex == 1 or iBatteryCurrentIndex == 3 or iBatteryCurrentIndex == 7) {
            if (ui->checkBoxBatteryInsulationResistanceMeasuringBoardUUTBB->isChecked())
                CheckBatteryInsulationResistanceMeasuringBoardUUTBB(QString::number(iBatteryCurrentIndex).toInt());
            if (ui->checkBoxBatteryOpenCircuitVoltagePowerSupply->isChecked())
                CheckBatteryOpenCircuitVoltagePowerSupply(QString::number(iBatteryCurrentIndex).toInt());
            if (ui->checkBoxBatteryClosedCircuitVoltagePowerSupply->isChecked())
                CheckBatteryClosedCircuitVoltagePowerSupply(QString::number(iBatteryCurrentIndex).toInt());
        }
        Log("Проверка батареи \"<b>"+ui->comboBoxBatteryList->currentText()+"</b>\" завершена.", "def");
        setEnabled(true);
        ui->btnStopCheck->setEnabled(false);
        ui->btnBuildReport->setEnabled(true);
        Log(tr("[Отладка] progressBarValue= %1, progressBarMaximum= %2").arg(ui->progressBar->value()).arg(ui->progressBar->maximum()), "red");
    }
}

/*
 * Задержка в милисекундах
 */
void MainWindow::delay( int millisecondsToWait )
{
    QTime dieTime = QTime::currentTime().addMSecs( millisecondsToWait );
    while( QTime::currentTime() < dieTime )
    {
        QCoreApplication::processEvents( QEventLoop::AllEvents, 100 );
    }
}

/*
 * Напряжение на корпусе батареи
 */
void MainWindow::CheckBatteryVoltageOnTheHousing(int index)
{
    Log(" --- Напряжение на корпусе батареи.", "blue");
    switch (index) {
    case 1: //Самодиагностика с помощью имитатора батареи
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(1);
        break;
    case 2: //9ER20P-20

        //если меньше 1В то не останавливаемся, больше 1В останавливаемся
        delay(1000);
        paramVoltageOnTheHousing1 = 0.9;
        if (paramVoltageOnTheHousing1 < 1) {
            color = "green";
        } else {
            color = "red";
        }
        ui->labelVoltageOnTheHousing1->setText("1) "+QString::number(paramVoltageOnTheHousing1));
        Log("1) между точкой металлизации и контактом 1 соединителя Х1 «Х1+» = <b>"+QString::number(paramVoltageOnTheHousing1)+"</b>", color);
        progressBarSet(1);

        delay(1000);
        paramVoltageOnTheHousing2 = 1.1;
        if (paramVoltageOnTheHousing2 < 1) {
            color = "green";
        } else {
            color = "red";
        }
        ui->labelVoltageOnTheHousing2->setText("2) "+QString::number(paramVoltageOnTheHousing2));
        Log("2) между точкой металлизации и контактом 1 соединителя Х3 «Х3-» = <b>"+QString::number(paramVoltageOnTheHousing2)+"</b>", color);
        progressBarSet(1);

        break;
    case 3: //9ER20P-20 (УУТББ)
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(2);
        break;
    case 4: //9ER20P-28
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(2);
        break;
    case 5: //9ER14P-24
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(2);
        break;
    case 6: //9ER14PS-24
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(2);
        break;
    case 7: //9ER14PS-24 (УУТББ)
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(2);
        break;
    default:
        break;
    }
    //ui->checkBoxVoltageOnTheHousing->setEnabled(false);
}

/*
 * Сопротивление изоляции батареи
 */
void MainWindow::CheckBatteryInsulationResistance(int index)
{
    Log(" --- Сопротивление изоляции батареи.", "blue");
    switch (index) {
    case 1: //Самодиагностика с помощью имитатора батареи
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(1);
        break;
    case 2: //9ER20P-20

        //более 20МОм не останавливаемся, меньше останавливаемся
        delay(1000);
        paramInsulationResistance1 = 20.3;
        if (paramInsulationResistance1 >= 20) {
            color = "green";
        } else {
            color = "red";
        }
        ui->labelInsulationResistance1->setText("1) "+QString::number(paramInsulationResistance1));
        Log("1) между точкой металлизации и контактом 1 соединителя Х1 «Х1+» = <b>"+QString::number(paramInsulationResistance1)+"</b>", color);
        progressBarSet(1);

        delay(1000);
        paramInsulationResistance2 = 19.8;
        if (paramInsulationResistance2 >= 20) {
            color = "green";
        } else {
            color = "red";
        }
        ui->labelInsulationResistance2->setText("2) "+QString::number(paramInsulationResistance2));
        Log("2) между точкой металлизации и контактом 1 соединителя Х3 «Х3-» = <b>"+QString::number(paramInsulationResistance2)+"</b>", color);
        progressBarSet(1);

        delay(1000);
        paramInsulationResistance3 = 13.2;
        if (paramInsulationResistance3 >= 20) {
            color = "green";
        } else {
            color = "red";
        }
        ui->labelInsulationResistance3->setText("3) "+QString::number(paramInsulationResistance3));
        Log("3) между точкой металлизации и контактом 6 соединителя Х1 «Х1+» = <b>"+QString::number(paramInsulationResistance3)+"</b>", color);
        progressBarSet(1);

        delay(1000);
        paramInsulationResistance4 = 14.4;
        if (paramInsulationResistance4 >= 20) {
            color = "green";
        } else {
            color = "red";
        }
        ui->labelInsulationResistance4->setText("4) "+QString::number(paramInsulationResistance4));
        Log("4) между точкой металлизации и контактом 7 соединителя Х3 «Х3-» = <b>"+QString::number(paramInsulationResistance4)+"</b>", color);
        progressBarSet(1);

        break;
    case 3: //9ER20P-20 (УУТББ)
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(4);
        break;
    case 4: //9ER20P-28
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(4);
        break;
    case 5: //9ER14P-24
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(2);
        break;
    case 6: //9ER14PS-24
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(2);
        break;
    case 7: //9ER14PS-24 (УУТББ)
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(2);
        break;
    default:
        break;
    }
    //ui->checkBoxInsulationResistance->setEnabled(false);
}

/*
 * Напряжение разомкнутой цепи группы
 */
void MainWindow::CheckBatteryOpenCircuitVoltageGroup(int index)
{
    Log(" --- Напряжение разомкнутой цепи группы.", "blue");
    switch (index) {
    case 1: //Самодиагностика с помощью имитатора батареи
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(1);
        break;
    case 2: //9ER20P-20
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(20);
        break;
    case 3: //9ER20P-20 (УУТББ)
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(20);
        break;
    case 4: //9ER20P-28
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(28);
        break;
    case 5: //9ER14P-24
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(24);
        break;
    case 6: //9ER14PS-24
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(24);
        break;
    case 7: //9ER14PS-24 (УУТББ)
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(24);
        break;
    default:
        break;
    }
    //ui->checkBoxBatteryOpenCircuitVoltageGroup->setEnabled(false);
}

/*
 * Напряжение замкнутой цепи группы
 */
void MainWindow::CheckBatteryClosedCircuitVoltageGroup(int index)
{
    Log(" --- Напряжение замкнутой цепи группы.", "blue");
    switch (index) {
    case 1: //Самодиагностика с помощью имитатора батареи
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(1);
        break;
    case 2: //9ER20P-20
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(60);
        break;
    case 3: //9ER20P-20 (УУТББ)
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(60);
        break;
    case 4: //9ER20P-28
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(84);
        break;
    case 5: //9ER14P-24
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(72);
        break;
    case 6: //9ER14PS-24
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(72);
        break;
    case 7: //9ER14PS-24 (УУТББ)
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(72);
        break;
    default:
        break;
    }
    //ui->checkBoxBatteryClosedCircuitVoltageGroup->setEnabled(false);
}

/*
 * Напряжение замкнутой цепи батареи
 */
void MainWindow::CheckBatteryClosedCircuitVoltage(int index)
{
    Log(" --- Напряжение замкнутой цепи батареи.", "blue");
    switch (index) {
    case 1: //Самодиагностика с помощью имитатора батареи
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(1);
        break;
    case 2: //9ER20P-20
        //более 30.0В не останавливаемся, меньше останавливаемся
        delay(1000);
        paramClosedCircuitVoltage = 29.8;
        if (paramClosedCircuitVoltage >= 30) {
            color = "green";
        } else {
            color = "red";
        }
        ui->labelClosedCircuitVoltage->setText(QString::number(paramClosedCircuitVoltage));
        Log("между контактом 1 соединителя Х1 «1+» и контактом 1 соединителя Х3 «3-» = <b>"+QString::number(paramClosedCircuitVoltage)+"</b>", color);
        progressBarSet(1);
        break;
    case 3: //9ER20P-20 (УУТББ)
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(1);
        break;
    case 4: //9ER20P-28
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(1);
        break;
    case 5: //9ER14P-24
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(1);
        break;
    case 6: //9ER14PS-24
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(1);
        break;
    case 7: //9ER14PS-24 (УУТББ)
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(1);
        break;
    default:
        break;
    }
    //ui->checkBoxClosedCircuitVoltage->setEnabled(false);
}

/*
 * Сопротивление изоляции платы измерительной УУТББ
 */
void MainWindow::CheckBatteryInsulationResistanceMeasuringBoardUUTBB(int index)
{
    Log(" --- Сопротивление изоляции платы измерительной УУТББ.", "blue");
    switch (index) {
    case 1: //Самодиагностика с помощью имитатора батареи
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(1);
        break;
    case 3: //9ER20P-20 (УУТББ)
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(1);
        break;
    case 7: //9ER14PS-24 (УУТББ)
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(1);
        break;
    default:
        break;
    }
    //ui->checkBoxBatteryInsulationResistanceMeasuringBoardUUTBB->setEnabled(false);
}

/*
 * Напряжение разомкнутой цепи блока питания
 */
void MainWindow::CheckBatteryOpenCircuitVoltagePowerSupply(int index)
{
    Log(" --- Напряжение разомкнутой цепи блока питания.", "blue");
    switch (index) {
    case 1: //Самодиагностика с помощью имитатора батареи
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(1);
        break;
    case 3: //9ER20P-20 (УУТББ)
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(1);
        break;
    case 7: //9ER14PS-24 (УУТББ)
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(1);
        break;
    default:
        break;
    }
    //ui->checkBoxBatteryOpenCircuitVoltagePowerSupply->setEnabled(false);
}

/*
 * Напряжение замкнутой цепи блока питания
 */
void MainWindow::CheckBatteryClosedCircuitVoltagePowerSupply(int index)
{
    Log(" --- Напряжение замкнутой цепи блока питания.", "blue");
    switch (index) {
    case 1: //Самодиагностика с помощью имитатора батареи
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(1);
        break;
    case 3: //9ER20P-20 (УУТББ)
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(1);
        break;
    case 7: //9ER14PS-24 (УУТББ)
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(1);
        break;
    default:
        break;
    }
    //ui->checkBoxBatteryClosedCircuitVoltagePowerSupply->setEnabled(false);
}
