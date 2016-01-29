#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->groupBoxDiagnosticDevice->setDisabled(true);
    ui->groupBoxDiagnosticMode->setDisabled(true);
    ui->groupBoxCheckParams->setDisabled(true);
    ui->rbInsulationResistanceMeasuringBoardUUTBB->hide();
    ui->btnInsulationResistanceMeasuringBoardUUTBB->hide();
    ui->btnInsulationResistanceMeasuringBoardUUTBB_2->hide();
    ui->rbOpenCircuitVoltagePowerSupply->hide();
    ui->btnOpenCircuitVoltagePowerSupply->hide();
    ui->btnOpenCircuitVoltagePowerSupply_2->hide();
    ui->rbClosedCircuitVoltagePowerSupply->hide();
    ui->btnClosedCircuitVoltagePowerSupply->hide();
    ui->btnClosedCircuitVoltagePowerSupply_2->hide();

    for (int i = 1; i < 9; i++) {
        ui->tabWidget->removeTab(1);
    }

    //bStop = false;
    bPause = false;
    bCheckCompleteVoltageOnTheHousing = false;
    bCheckCompleteInsulationResistance = false;
    bCheckCompleteOpenCircuitVoltageGroup = false;
    bCheckCompleteClosedCircuitVoltageGroup = false;
    bCheckCompleteClosedCircuitVoltageBattery = false;
    bCheckCompleteInsulationResistanceMeasuringBoardUUTBB = false;
    bCheckCompleteOpenCircuitVoltagePowerSupply = false;
    bCheckCompleteClosedCircuitVoltagePowerSupply = false;
    iBatteryIndex = 0;
    iStep = 0;
    iAllSteps = 0;
    iStepVoltageOnTheHousing = 1;
    iStepInsulationResistance = 1;
    iStepOpenCircuitVoltageGroup = 1;

    //ResetCheck();
    getCOMPorts();
    com = new QSerialPort(this);
    connect(ui->btnCOMPortConnect, SIGNAL(clicked(bool)), this, SLOT(openCOMPort()));
    connect(ui->btnCOMPortDisconnect, SIGNAL(clicked(bool)), this, SLOT(closeCOMPort()));
    connect(ui->cbIsUUTBB, SIGNAL(toggled(bool)), this, SLOT(isUUTBB()));
    connect(ui->comboBoxBatteryList, SIGNAL(currentIndexChanged(int)), this , SLOT(handleSelectionChangedBattery(int)));
    connect(ui->rbModeDiagnosticAuto, SIGNAL(toggled(bool)), ui->groupBoxCheckParams, SLOT(setDisabled(bool)));
    connect(ui->rbModeDiagnosticAuto, SIGNAL(toggled(bool)), ui->btnStartAutoModeDiagnostic, SLOT(setEnabled(bool)));
    connect(ui->rbModeDiagnosticManual, SIGNAL(toggled(bool)), ui->rbVoltageOnTheHousing, SLOT(setEnabled(bool)));
    //connect(ui->rbModeDiagnosticManual, SIGNAL(toggled(bool)), ui->btnPauseAutoModeDiagnostic, SLOT(setDisabled(bool)));
    connect(ui->rbVoltageOnTheHousing, SIGNAL(toggled(bool)), ui->btnVoltageOnTheHousing, SLOT(setEnabled(bool)));
    connect(ui->rbInsulationResistance, SIGNAL(toggled(bool)), ui->btnInsulationResistance, SLOT(setEnabled(bool)));
    connect(ui->rbOpenCircuitVoltageGroup, SIGNAL(toggled(bool)), ui->btnOpenCircuitVoltageGroup, SLOT(setEnabled(bool)));
    connect(ui->rbClosedCircuitVoltageGroup, SIGNAL(toggled(bool)), ui->btnClosedCircuitVoltageGroup, SLOT(setEnabled(bool)));
    connect(ui->rbClosedCircuitVoltageBattery, SIGNAL(toggled(bool)), ui->btnClosedCircuitVoltageBattery, SLOT(setEnabled(bool)));
    connect(ui->rbInsulationResistanceMeasuringBoardUUTBB, SIGNAL(toggled(bool)), ui->btnInsulationResistanceMeasuringBoardUUTBB, SLOT(setEnabled(bool)));
    connect(ui->rbOpenCircuitVoltagePowerSupply, SIGNAL(toggled(bool)), ui->btnOpenCircuitVoltagePowerSupply, SLOT(setEnabled(bool)));
    connect(ui->rbClosedCircuitVoltagePowerSupply, SIGNAL(toggled(bool)), ui->btnClosedCircuitVoltagePowerSupply, SLOT(setEnabled(bool)));
    //connect(ui->btnVoltageOnTheHousing, SIGNAL(clicked(int)), this, SLOT(checkVoltageOnTheHousing(iBatteryIndex, iStepVoltageOnTheHousing)));
    connect(ui->btnVoltageOnTheHousing, SIGNAL(clicked(bool)), this, SLOT(checkVoltageOnTheHousing()));
    connect(ui->btnInsulationResistance, SIGNAL(clicked(bool)), this, SLOT(checkInsulationResistance()));
    connect(ui->btnOpenCircuitVoltageGroup, SIGNAL(clicked(bool)), this, SLOT(checkOpenCircuitVoltageGroup()));
    connect(ui->btnClosedCircuitVoltageGroup, SIGNAL(clicked(bool)), this, SLOT(checkClosedCircuitVoltageGroup()));
    connect(ui->btnClosedCircuitVoltageBattery, SIGNAL(clicked(bool)), this, SLOT(checkClosedCircuitVoltageBattery()));
    connect(ui->btnInsulationResistanceMeasuringBoardUUTBB, SIGNAL(clicked(bool)), this, SLOT(checkInsulationResistanceMeasuringBoardUUTBB()));
    connect(ui->btnOpenCircuitVoltagePowerSupply, SIGNAL(clicked(bool)), this, SLOT(checkOpenCircuitVoltagePowerSupply()));
    connect(ui->btnClosedCircuitVoltagePowerSupply, SIGNAL(clicked(bool)), this, SLOT(checkClosedCircuitVoltagePowerSupply()));
    connect(ui->btnVoltageOnTheHousing_2, SIGNAL(clicked(bool)), this, SLOT(checkVoltageOnTheHousing()));
    connect(ui->btnInsulationResistance_2, SIGNAL(clicked(bool)), this, SLOT(checkInsulationResistance()));
    connect(ui->btnOpenCircuitVoltageGroup_2, SIGNAL(clicked(bool)), this, SLOT(checkOpenCircuitVoltageGroup()));
    connect(ui->btnClosedCircuitVoltageGroup_2, SIGNAL(clicked(bool)), this, SLOT(checkClosedCircuitVoltageGroup()));
    connect(ui->btnClosedCircuitVoltageBattery_2, SIGNAL(clicked(bool)), this, SLOT(checkClosedCircuitVoltageBattery()));
    connect(ui->btnInsulationResistanceMeasuringBoardUUTBB_2, SIGNAL(clicked(bool)), this, SLOT(checkInsulationResistanceMeasuringBoardUUTBB()));
    connect(ui->btnOpenCircuitVoltagePowerSupply_2, SIGNAL(clicked(bool)), this, SLOT(checkOpenCircuitVoltagePowerSupply()));
    connect(ui->btnClosedCircuitVoltagePowerSupply_2, SIGNAL(clicked(bool)), this, SLOT(checkClosedCircuitVoltagePowerSupply()));
    connect(ui->btnStartAutoModeDiagnostic, SIGNAL(clicked(bool)), this, SLOT(checkAutoModeDiagnostic()));
    connect(ui->btnPauseAutoModeDiagnostic, SIGNAL(clicked(bool)), this, SLOT(setPause()));
    connect(ui->btnCOMPortDisconnect, SIGNAL(clicked(bool)), ui->btnStartAutoModeDiagnostic, SLOT(setEnabled(bool)));
}

MainWindow::~MainWindow()
{
    delete ui;
}

/*
 * УУТББ дополнительные параметры проверки
 */
void MainWindow::setPause() {
    //bPause = ((QPushButton*)sender())->isChecked() ? true : false;
    bPause = true;
    ((QPushButton*)sender())->setEnabled(false);
    ui->btnStartAutoModeDiagnostic->setEnabled(true);
}


/*
 * УУТББ дополнительные параметры проверки
 */
void MainWindow::isUUTBB()
{
    if (ui->cbIsUUTBB->isChecked()) {
        ui->rbInsulationResistanceMeasuringBoardUUTBB->show();
        ui->btnInsulationResistanceMeasuringBoardUUTBB->show();
        ui->btnInsulationResistanceMeasuringBoardUUTBB_2->show();
        ui->rbOpenCircuitVoltagePowerSupply->show();
        ui->btnOpenCircuitVoltagePowerSupply->show();
        ui->btnOpenCircuitVoltagePowerSupply_2->show();
        ui->rbClosedCircuitVoltagePowerSupply->show();
        ui->btnClosedCircuitVoltagePowerSupply->show();
        ui->btnClosedCircuitVoltagePowerSupply_2->show();
    } else {
        ui->rbInsulationResistanceMeasuringBoardUUTBB->hide();
        ui->btnInsulationResistanceMeasuringBoardUUTBB->hide();
        ui->btnInsulationResistanceMeasuringBoardUUTBB_2->hide();
        ui->rbOpenCircuitVoltagePowerSupply->hide();
        ui->btnOpenCircuitVoltagePowerSupply->hide();
        ui->btnOpenCircuitVoltagePowerSupply_2->hide();
        ui->rbClosedCircuitVoltagePowerSupply->hide();
        ui->btnClosedCircuitVoltagePowerSupply->hide();
        ui->btnClosedCircuitVoltagePowerSupply_2->hide();
    }
}

/*
 * Проверка проверяемых параметров
 */
/*void MainWindow::paramCheck()
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
        if (paramMsg.length() > 0)
            iStartCheck = QMessageBox::question(this, "Внимание", "Вы уверны что хотите пропустить следующие этапы проверки?\n"+paramMsg, tr("Да"), tr("Нет"));
    } else {
        iStartCheck = QMessageBox::information(this, "Внимание", "Вы должны выбрать проверяемый параметр.");
    }
    Log(tr("[ОТЛАДКА] iParamsNumberChecked=%1").arg(iParamsNumberChecked), "blue");
}*/

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
/*void MainWindow::progressBarSetMaximum()
{
    ui->progressBar->setValue(0);
    iAllSteps = 0;
    switch (iBatteryCurrentIndex) {
    case 1: //Самодиагностика с помощью имитатора батареи
        if (ui->checkBoxVoltageOnTheHousing->isChecked())
            iAllSteps += 1;
        if (ui->checkBoxInsulationResistance->isChecked())
            iAllSteps += 1;
        if (ui->checkBoxBatteryOpenCircuitVoltageGroup->isChecked())
            iAllSteps += 1;
        if (ui->checkBoxBatteryClosedCircuitVoltageGroup->isChecked())
            iAllSteps += 1;
        if (ui->checkBoxClosedCircuitVoltage->isChecked())
            iAllSteps += 1;
        if (ui->checkBoxBatteryInsulationResistanceMeasuringBoardUUTBB->isChecked())
            iAllSteps += 1;
        if (ui->checkBoxBatteryOpenCircuitVoltagePowerSupply->isChecked())
            iAllSteps += 1;
        if (ui->checkBoxBatteryClosedCircuitVoltagePowerSupply->isChecked())
            iAllSteps += 1;
        break;
    case 2: //9ER20P-20
        if (ui->checkBoxVoltageOnTheHousing->isChecked())
            iAllSteps += 2;
        if (ui->checkBoxInsulationResistance->isChecked())
            iAllSteps += 4;
        if (ui->checkBoxBatteryOpenCircuitVoltageGroup->isChecked())
            iAllSteps += 20;
        if (ui->checkBoxBatteryClosedCircuitVoltageGroup->isChecked())
            iAllSteps += 60;
        if (ui->checkBoxClosedCircuitVoltage->isChecked())
            iAllSteps += 1;
        break;
    case 3: //9ER20P-20 (УУТББ)
        if (ui->checkBoxVoltageOnTheHousing->isChecked())
            iAllSteps += 2;
        if (ui->checkBoxInsulationResistance->isChecked())
            iAllSteps += 4;
        if (ui->checkBoxBatteryOpenCircuitVoltageGroup->isChecked())
            iAllSteps += 20;
        if (ui->checkBoxBatteryClosedCircuitVoltageGroup->isChecked())
            iAllSteps += 60;
        if (ui->checkBoxClosedCircuitVoltage->isChecked())
            iAllSteps += 1;
        if (ui->checkBoxBatteryInsulationResistanceMeasuringBoardUUTBB->isChecked())
            iAllSteps += 1;
        if (ui->checkBoxBatteryOpenCircuitVoltagePowerSupply->isChecked())
            iAllSteps += 1;
        if (ui->checkBoxBatteryClosedCircuitVoltagePowerSupply->isChecked())
            iAllSteps += 1;
        break;
    case 4: //9ER20P-28
        if (ui->checkBoxVoltageOnTheHousing->isChecked())
            iAllSteps += 2;
        if (ui->checkBoxInsulationResistance->isChecked())
            iAllSteps += 4;
        if (ui->checkBoxBatteryOpenCircuitVoltageGroup->isChecked())
            iAllSteps += 28;
        if (ui->checkBoxBatteryClosedCircuitVoltageGroup->isChecked())
            iAllSteps += 84;
        if (ui->checkBoxClosedCircuitVoltage->isChecked())
            iAllSteps += 1;
        break;
    case 5: //9ER14P-24
        if (ui->checkBoxVoltageOnTheHousing->isChecked())
            iAllSteps += 2;
        if (ui->checkBoxInsulationResistance->isChecked())
            iAllSteps += 2;
        if (ui->checkBoxBatteryOpenCircuitVoltageGroup->isChecked())
            iAllSteps += 24;
        if (ui->checkBoxBatteryClosedCircuitVoltageGroup->isChecked())
            iAllSteps += 72;
        if (ui->checkBoxClosedCircuitVoltage->isChecked())
            iAllSteps += 1;
        break;
    case 6: //9ER14PS-24
        if (ui->checkBoxVoltageOnTheHousing->isChecked())
            iAllSteps += 2;
        if (ui->checkBoxInsulationResistance->isChecked())
            iAllSteps += 2;
        if (ui->checkBoxBatteryOpenCircuitVoltageGroup->isChecked())
            iAllSteps += 24;
        if (ui->checkBoxBatteryClosedCircuitVoltageGroup->isChecked())
            iAllSteps += 72;
        if (ui->checkBoxClosedCircuitVoltage->isChecked())
            iAllSteps += 1;
        break;
    case 7: //9ER14PS-24 (УУТББ)
        if (ui->checkBoxVoltageOnTheHousing->isChecked())
            iAllSteps += 2;
        if (ui->checkBoxInsulationResistance->isChecked())
            iAllSteps += 2;
        if (ui->checkBoxBatteryOpenCircuitVoltageGroup->isChecked())
            iAllSteps += 24;
        if (ui->checkBoxBatteryClosedCircuitVoltageGroup->isChecked())
            iAllSteps += 72;
        if (ui->checkBoxClosedCircuitVoltage->isChecked())
            iAllSteps += 1;
        if (ui->checkBoxBatteryInsulationResistanceMeasuringBoardUUTBB->isChecked())
            iAllSteps += 1;
        if (ui->checkBoxBatteryOpenCircuitVoltagePowerSupply->isChecked())
            iAllSteps += 1;
        if (ui->checkBoxBatteryClosedCircuitVoltagePowerSupply->isChecked())
            iAllSteps += 1;
        break;
    default:
        break;
    }
    ui->progressBar->setMaximum(iAllSteps);
}*/

/*
 * COM Порт получения списка портов
 */
void MainWindow::getCOMPorts()
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
        Log(tr("[COM Порт] - Соединение установлено на порт %1.").arg(ui->comboBoxCOMPort->currentText()), "green");
        foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
            if (info.portName() == ui->comboBoxCOMPort->currentText()) {
                Log(tr("[COM Порт] - Описание: %1.").arg(info.description()), "blue");
            }
        }
        ui->btnCOMPortConnect->setEnabled(false);
        ui->btnCOMPortDisconnect->setEnabled(true);
        ui->comboBoxCOMPort->setEnabled(true);
        ui->groupBoxDiagnosticDevice->setEnabled(true);
        ui->groupBoxDiagnosticMode->setEnabled(true);
        ui->groupBoxCheckParams->setEnabled(true);
    } else {
        Log(tr("[COM Порт] - Ошибка устройства: %1.").arg(com->errorString()), "red");
    }
}

/*
 * COM Порт запись данных
 */
void MainWindow::writeCOMPortData()
{
    QByteArray data;
    //data.append(ui->lineEditCOMPortCommand->text());
    com->write(data);
    //Log(tr("[COM Порт] - Отправка: %1").arg(ui->lineEditCOMPortCommand->text()), "green");
}

/*
 * COM Порт чтение данных
 */
void MainWindow::readCOMPortData()
{
    QByteArray data = com->readAll();
    Log(tr("[COM Порт] - Получение: %1").arg(QString(data)), "blue");
}

/*
 * COM Порт отключение
 */
void MainWindow::closeCOMPort()
{
    if (com->isOpen()) {
        com->close();
        ui->btnCOMPortDisconnect->setEnabled(false);
        ui->btnCOMPortConnect->setEnabled(true);
        ui->comboBoxCOMPort->setEnabled(true);
        ui->groupBoxDiagnosticDevice->setEnabled(false);
        ui->groupBoxDiagnosticMode->setEnabled(false);
        ui->groupBoxCheckParams->setEnabled(false);
        ui->btnStartAutoModeDiagnostic->setEnabled(false);
    }
    Log("[COM Порт] - Соединение разъединено.", "red");
}

/*
 * Включение отключение элементов
 */
/*void MainWindow::setEnabled(bool flag)
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
    ui->btnCheckConnectedBattery->setEnabled(flag);
    ui->dateEditBatteryBuild->setEnabled(flag);
    ui->lineEditBatteryNumber->setEnabled(flag);
    //ui->rbModeDiagnosticAuto->setEnabled(flag);
    //ui->rbModeDiagnosticManual->setEnabled(flag);
    ui->groupBoxDiagnosticMode->setEnabled(true);
    //ui->groupBoxCheckParams->setEnabled(flag);
    if (ui->rbModeDiagnosticManual->isChecked()) {
        ui->rbVoltageOnTheHousing->setEnabled(flag);
        ui->btnVoltageOnTheHousing->setEnabled(flag);
    } else {
        ui->rbVoltageOnTheHousing->setEnabled(!flag);
        ui->btnVoltageOnTheHousing->setEnabled(!flag);
    }
    if(iBatteryCurrentIndex == 0 or iBatteryCurrentIndex == 1 or iBatteryCurrentIndex == 4 ) {
        ui->checkBoxUUTBB->setEnabled(flag);
    }
    ui->btnStartCheckAll->setEnabled(flag);
}*/

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
 * Сброс параметров проверки
 */
void MainWindow::resetCheck()
{
    iStep = 0;
    iAllSteps = 0;
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
    if (index == 0 or index == 1 or index == 4) {
        ui->cbIsUUTBB->setEnabled(true);
    } else {
        ui->cbIsUUTBB->setEnabled(false);
        ui->cbIsUUTBB->setChecked(false);

    }

    iBatteryIndex = QString::number(index).toInt();
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
 * Напряжение на корпусе батареи
 */
/*void MainWindow::checkVoltageOnTheHousing()
{
    //ui->progressBar->setValue(0);
    //ui->progressBar->setMaximum(2);
    Log(tr("Начало проверки - %1").arg(ui->rbVoltageOnTheHousing->text()), "blue");
    checkBattery(iBatteryIndex, 1, 2);
    Log(tr("Завершение проверки - %1").arg(ui->rbVoltageOnTheHousing->text()), "blue");
    ui->rbInsulationResistance->setEnabled(true);
}*/

/*
 * Напряжение на корпусе батареи
 */
/*void MainWindow::checkInsulationResistance()
{
    Log(tr("Начало проверки - %1").arg(ui->rbInsulationResistance->text()), "blue");
    checkBattery(iBatteryIndex, 3, 6);
    Log(tr("Завершение проверки - %1").arg(ui->rbInsulationResistance->text()), "blue");
    ui->rbOpenCircuitVoltageGroup->setEnabled(true);
}*/

/*
 * Автоматический режим диагностики
 */
void MainWindow::checkAutoModeDiagnostic()
{
    bPause = false;
    ui->btnStartAutoModeDiagnostic->setEnabled(false);
    ui->groupBoxCOMPort->setEnabled(false);
    ui->groupBoxDiagnosticDevice->setEnabled(false);
    ui->groupBoxDiagnosticMode->setEnabled(false);
    ui->btnPauseAutoModeDiagnostic->setEnabled(true);
    ui->btnBuildReport->setEnabled(false);
    if(!bCheckCompleteVoltageOnTheHousing)
        checkVoltageOnTheHousing();
    if(!bCheckCompleteInsulationResistance)
        checkInsulationResistance();
    if(!bCheckCompleteOpenCircuitVoltageGroup)
        checkOpenCircuitVoltageGroup();
    if(!bCheckCompleteClosedCircuitVoltageGroup)
        checkClosedCircuitVoltageGroup();
    if(!bCheckCompleteClosedCircuitVoltageBattery)
        checkClosedCircuitVoltageBattery();
    if (ui->cbIsUUTBB->isChecked()) {
        if(!bCheckCompleteClosedCircuitVoltageBattery)
            checkInsulationResistanceMeasuringBoardUUTBB();
        if(!bCheckCompleteOpenCircuitVoltagePowerSupply)
            checkOpenCircuitVoltagePowerSupply();
        if(!bCheckCompleteClosedCircuitVoltagePowerSupply)
            checkClosedCircuitVoltagePowerSupply();
    }
    //ui->btnStopCheck->setEnabled(false);
    //ui->btnBuildReport->setEnabled(true);
    if (bPause) { Log("ПАУЗА.", "red"); return; }
    ui->groupBoxCOMPort->setEnabled(true);
    ui->groupBoxDiagnosticDevice->setEnabled(true);
    ui->groupBoxDiagnosticMode->setEnabled(true);
    ui->btnPauseAutoModeDiagnostic->setEnabled(false);
    ui->btnBuildReport->setEnabled(true);
    Log("Проверка завершена - Автоматический режим", "blue");
    Log(tr("[ОТЛАДКА] progressBarValue= %1, progressBarMaximum= %2").arg(ui->progressBar->value()).arg(ui->progressBar->maximum()), "blue");
}

/*
 * Напряжение на корпусе батареи
 */
void MainWindow::checkVoltageOnTheHousing()
{
    if (((QPushButton*)sender())->objectName() == "btnVoltageOnTheHousing") {
        iStepVoltageOnTheHousing = 1;
        bPause = false;
        ui->btnVoltageOnTheHousing_2->setEnabled(false);
    }
    if (((QPushButton*)sender())->objectName() == "btnVoltageOnTheHousing_2")
        bPause = false;
    if (bPause) return;
    ui->groupBoxCOMPort->setEnabled(false);
    ui->groupBoxDiagnosticDevice->setEnabled(false);
    ui->groupBoxDiagnosticMode->setEnabled(false);
    ui->tabWidget->addTab(ui->tabVoltageOnTheHousing, ui->rbVoltageOnTheHousing->text());
    //ui->tabWidget->setCurrentIndex(ui->tabWidget->count()-1);
    Log(tr("Проверка начата - %1").arg(ui->rbVoltageOnTheHousing->text()), "blue");
    switch (iBatteryIndex) {
    case 0: //9ER20P-20
        ui->progressBar->setValue(iStepVoltageOnTheHousing-1);
        ui->progressBar->setMaximum(2);
        while (iStepVoltageOnTheHousing <= 2) {
            //if (bPause) { Log("паузаVoltageOnTheHousing", "red"); return; }
            if (bPause) return;
            switch (iStepVoltageOnTheHousing) {
            case 1://1) между точкой металлизации и контактом 1 соединителя Х1 «Х1+»
                //если меньше 1В то не останавливаемся идем дальше, больше 1В спрашиваем продолжить или нет
                delay(1000);
                paramVoltageOnTheHousing1 = qrand()%2; //число полученное с COM-порта
                color = (paramVoltageOnTheHousing1 > 1) ? "red" : "green";
                ui->labelVoltageOnTheHousing1->setText("1) "+QString::number(paramVoltageOnTheHousing1));
                Log(tr("1) между точкой металлизации и контактом 1 соединителя Х1 «Х1+» = <b>%1</b>").arg(QString::number(paramVoltageOnTheHousing1)), color);
                progressBarSet(1);
                iStepVoltageOnTheHousing++;
                if ((paramVoltageOnTheHousing1 > 1) and QMessageBox::question(this, "Внимание - "+ui->rbVoltageOnTheHousing->text(), tr("1) между точкой металлизации и контактом 1 соединителя Х1 «Х1+» = <b>%1</b> \nпродолжить?").arg(paramVoltageOnTheHousing1), tr("Да"), tr("Нет"))) {
                    return;
                }
                //iStepVoltageOnTheHousing++;
                break;
            case 2://2) между точкой металлизации и контактом 1 соединителя Х3 «Х3-»
                delay(1000);
                paramVoltageOnTheHousing2 = qrand()%2;; //число полученное с COM-порта
                color = (paramVoltageOnTheHousing2 > 1) ? "red" : "green";
                ui->labelVoltageOnTheHousing2->setText("2) "+QString::number(paramVoltageOnTheHousing2));
                Log(tr("2) между точкой металлизации и контактом 1 соединителя Х3 «Х3-» = <b>%1</b>").arg(QString::number(paramVoltageOnTheHousing2)), color);
                progressBarSet(1);
                iStepVoltageOnTheHousing++;
                if ((paramVoltageOnTheHousing2 > 1) and QMessageBox::question(this, "Внимание - "+ui->rbVoltageOnTheHousing->text(), tr("2) между точкой металлизации и контактом 1 соединителя Х3 «Х3-» = <b>%1</b> \nпродолжить?").arg(paramVoltageOnTheHousing2), tr("Да"), tr("Нет"))) {
                    return;
                }
                break;
            default:
                break;
            }
        }
        if (ui->rbModeDiagnosticAuto->isChecked())
            bCheckCompleteVoltageOnTheHousing = true;
        break;
    case 1:
        if (bPause) return;
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(1);
        break;
    case 2:
        if (bPause) return;
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(1);
        break;
    case 3:
        if (bPause) return;
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(1);
        break;
    default:
        break;
    }
    Log(tr("Проверка завершена - %1").arg(ui->rbVoltageOnTheHousing->text()), "blue");
    iStepVoltageOnTheHousing = 1;
    ui->rbInsulationResistance->setEnabled(true);
    ui->groupBoxCOMPort->setEnabled(true);
    ui->groupBoxDiagnosticDevice->setEnabled(true);
    ui->groupBoxDiagnosticMode->setEnabled(true);
}

/*
 * Сопротивление изоляции
 */
void MainWindow::checkInsulationResistance()
{
    if (((QPushButton*)sender())->objectName() == "btnInsulationResistance") {
        iStepInsulationResistance = 1;
        bPause = false;
        ui->btnInsulationResistance_2->setEnabled(false);
    }
    if (((QPushButton*)sender())->objectName() == "btnInsulationResistance_2")
        bPause = false;
    if (bPause) return;
    ui->groupBoxCOMPort->setEnabled(false);
    ui->groupBoxDiagnosticDevice->setEnabled(false);
    ui->groupBoxDiagnosticMode->setEnabled(false);
    ui->tabWidget->addTab(ui->tabInsulationResistance, ui->rbInsulationResistance->text());
    //ui->tabWidget->setCurrentIndex(ui->tabWidget->count()-1);
    Log(tr("Проверка начата - %1").arg(ui->rbInsulationResistance->text()), "blue");
    switch (iBatteryIndex) {
    case 0: //9ER20P-20
        ui->progressBar->setValue(iStepInsulationResistance-1);
        ui->progressBar->setMaximum(4);
        while (iStepInsulationResistance <= 4) {
            if (bPause) return;
            switch (iStepInsulationResistance) {
            case 1://1) между точкой металлизации и контактом 1 соединителя Х1 «Х1+»
                //меньше 20МОм спрашиваем продолжить или нет, больше 20МОм продолжаем не спрашивая
                delay(1000);
                paramInsulationResistance1 = qrand()%25; //число полученное с COM-порта
                color = (paramInsulationResistance1 < 20) ? "red" : "green";
                ui->labelInsulationResistance1->setText("1) "+QString::number(paramInsulationResistance1));
                Log(tr("1) между точкой металлизации и контактом 1 соединителя Х1 «Х1+» = <b>%1</b>").arg(QString::number(paramInsulationResistance1)), color);
                progressBarSet(1);
                iStepInsulationResistance++;
                if (paramInsulationResistance1 < 20) {
                    ui->rbModeDiagnosticManual->setChecked(true);
                    ui->rbModeDiagnosticAuto->setEnabled(false);
                    ui->rbInsulationResistance->setChecked(true);
                    if (QMessageBox::question(this, "Внимание - "+ui->rbInsulationResistance->text(), tr("1) между точкой металлизации и контактом 1 соединителя Х1 «Х1+» = <b>%1</b> \nпродолжить?").arg(paramInsulationResistance1), tr("Да"), tr("Нет"))) {
                        ui->btnInsulationResistance_2->setEnabled(true);
                        bPause = true;
                        return;
                    }
                }
                break;
            case 2://2) между точкой металлизации и контактом 1 соединителя Х3 «Х3-»
                //меньше 20МОм спрашиваем продолжить или нет, больше 20МОм продолжаем не спрашивая
                delay(1000);
                paramInsulationResistance2 = qrand()%25; //число полученное с COM-порта
                color = (paramInsulationResistance2 < 20) ? "red" : "green";
                ui->labelInsulationResistance2->setText("2) "+QString::number(paramInsulationResistance2));
                Log(tr("2) между точкой металлизации и контактом 1 соединителя Х3 «Х3-» = <b>%1</b>").arg(QString::number(paramInsulationResistance2)), color);
                progressBarSet(1);
                iStepInsulationResistance++;
                if (paramInsulationResistance2 < 20) {
                        ui->rbModeDiagnosticManual->setChecked(true);
                        ui->rbModeDiagnosticAuto->setEnabled(false);
                        ui->rbInsulationResistance->setChecked(true);
                        if (QMessageBox::question(this, "Внимание - "+ui->rbInsulationResistance->text(), tr("2) между точкой металлизации и контактом 1 соединителя Х3 «Х3-» = <b>%1</b> \nпродолжить?").arg(paramInsulationResistance2), tr("Да"), tr("Нет"))) {
                            ui->btnInsulationResistance_2->setEnabled(true);
                            bPause = true;
                            return;
                        }
                }
                break;
            case 3://3) между точкой металлизации и контактом 6 соединителя Х1 «Х1+»
                //меньше 20МОм спрашиваем продолжить или нет, больше 20МОм продолжаем не спрашивая
                delay(1000);
                paramInsulationResistance3 = qrand()%25; //число полученное с COM-порта
                color = (paramInsulationResistance3 < 20) ? "red" : "green";
                ui->labelInsulationResistance3->setText("3) "+QString::number(paramInsulationResistance3));
                Log(tr("3) между точкой металлизации и контактом 6 соединителя Х1 «Х1+» = <b>%1</b>").arg(QString::number(paramInsulationResistance3)), color);
                progressBarSet(1);
                iStepInsulationResistance++;
                if (paramInsulationResistance3 < 20) {
                    ui->rbModeDiagnosticManual->setChecked(true);
                    ui->rbModeDiagnosticAuto->setEnabled(false);
                    ui->rbInsulationResistance->setChecked(true);
                    if (QMessageBox::question(this, "Внимание - "+ui->rbInsulationResistance->text(), tr("3) между точкой металлизации и контактом 6 соединителя Х1 «Х1+» = <b>%1</b> \nпродолжить?").arg(paramInsulationResistance3), tr("Да"), tr("Нет"))) {
                        ui->btnInsulationResistance_2->setEnabled(true);
                        bPause = true;
                        return;
                    }
                }
                break;
            case 4://4) между точкой металлизации и контактом 7 соединителя Х3 «Х3-»
                //меньше 20МОм спрашиваем продолжить или нет, больше 20МОм продолжаем не спрашивая
                delay(1000);
                paramInsulationResistance4 = qrand()%25; //число полученное с COM-порта
                color = (paramInsulationResistance4 < 20) ? "red" : "green";
                ui->labelInsulationResistance4->setText("3) "+QString::number(paramInsulationResistance4));
                Log(tr("4) между точкой металлизации и контактом 7 соединителя Х3 «Х3-» = <b>%1</b>").arg(QString::number(paramInsulationResistance4)), color);
                progressBarSet(1);
                iStepInsulationResistance++;
                if (paramInsulationResistance4 < 20) {
                    ui->rbModeDiagnosticManual->setChecked(true);
                    ui->rbModeDiagnosticAuto->setEnabled(false);
                    ui->rbInsulationResistance->setChecked(true);
                    if (QMessageBox::question(this, "Внимание - "+ui->rbInsulationResistance->text(), tr("4) между точкой металлизации и контактом 7 соединителя Х3 «Х3-» = <b>%1</b> \nпродолжить?").arg(paramInsulationResistance4), tr("Да"), tr("Нет"))) {
                        ui->btnInsulationResistance_2->setEnabled(true);
                        bPause = true;
                        return;
                    }
                }
                break;
            default:
                break;
            }
        }
        ui->btnInsulationResistance_2->setEnabled(false);
        if (ui->rbModeDiagnosticAuto->isChecked())
             bCheckCompleteInsulationResistance = true;
        break;
    case 1:
        if (bPause) return;
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(1);
        break;
    case 2:
        if (bPause) return;
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(1);
        break;
    case 3:
        if (bPause) return;
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(1);
        break;
    default:
        break;
    }
    Log(tr("Проверка завершена - %1").arg(ui->rbInsulationResistance->text()), "blue");
    iStepInsulationResistance = 1;
    ui->rbOpenCircuitVoltageGroup->setEnabled(true);
    ui->groupBoxCOMPort->setEnabled(true);
    ui->groupBoxDiagnosticDevice->setEnabled(true);
    ui->groupBoxDiagnosticMode->setEnabled(true);
}

/*
 * Напряжение разомкнутой цепи группы
 */
void MainWindow::checkOpenCircuitVoltageGroup()
{
    if (((QPushButton*)sender())->objectName() == "btnOpenCircuitVoltageGroup") {
        iStepOpenCircuitVoltageGroup = 1;
        bPause = false;
        ui->btnOpenCircuitVoltageGroup_2->setEnabled(false);
    }
    if (((QPushButton*)sender())->objectName() == "btnOpenCircuitVoltageGroup_2")
        bPause = false;
    if (bPause) return;
    ui->groupBoxCOMPort->setEnabled(false);
    ui->groupBoxDiagnosticDevice->setEnabled(false);
    ui->groupBoxDiagnosticMode->setEnabled(false);
    ui->tabWidget->addTab(ui->tabOpenCircuitVoltageGroup, ui->rbOpenCircuitVoltageGroup->text());
    //ui->tabWidget->setCurrentIndex(ui->tabWidget->count()-1);
    Log(tr("Проверка начата - %1").arg(ui->rbOpenCircuitVoltageGroup->text()), "blue");
    switch (iBatteryIndex) {
    case 0: //9ER20P-20
        ui->progressBar->setValue(iStepOpenCircuitVoltageGroup-1);
        ui->progressBar->setMaximum(20);
        while (iStepOpenCircuitVoltageGroup <= 20) {
            if (bPause) return;
            switch (iStepOpenCircuitVoltageGroup) {
            default:
                delay(1000);
                paramOpenCircuitVoltageGroup1 = qrand()%40+10; //число полученное с COM-порта
                color = (paramOpenCircuitVoltageGroup1 < 32.3) ? "red" : "green";
                Log(tr("%1) Между контактом 1 соединителя Х3 «Х3-» и контактом %1 соединителя Х4 «4» = <b>%2</b>").arg(iStepOpenCircuitVoltageGroup).arg(paramOpenCircuitVoltageGroup1), color);
                progressBarSet(1);
                iStepOpenCircuitVoltageGroup++;
                if (paramOpenCircuitVoltageGroup1 < 32.3) {
                    ui->rbModeDiagnosticManual->setChecked(true);
                    ui->rbModeDiagnosticAuto->setEnabled(false);
                    ui->rbOpenCircuitVoltageGroup->setChecked(true);
                    if (QMessageBox::question(this, "Внимание - "+ui->rbOpenCircuitVoltageGroup->text(), tr("%1) Между контактом 1 соединителя Х3 «Х3-» и контактом %1 соединителя Х4 «4» = <b>%2</b> \nпродолжить?").arg(iStepOpenCircuitVoltageGroup).arg(paramOpenCircuitVoltageGroup1), tr("Да"), tr("Нет"))) {
                        ui->btnOpenCircuitVoltageGroup_2->setEnabled(true);
                        bPause = true;
                        return;
                    }
                }
                break;
            }
        }
        if (ui->rbModeDiagnosticAuto->isChecked())
            bCheckCompleteClosedCircuitVoltageGroup = true;
        break;
    case 1:
        if (bPause) return;
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(1);
        break;
    case 2:
        if (bPause) return;
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(1);
        break;
    case 3:
        if (bPause) return;
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(1);
        break;
    default:
        break;
    }

    Log(tr("Проверка завершена - %1").arg(ui->rbOpenCircuitVoltageGroup->text()), "blue");
    iStepOpenCircuitVoltageGroup = 1;
    ui->rbClosedCircuitVoltageGroup->setEnabled(true);
    ui->groupBoxCOMPort->setEnabled(true);
    ui->groupBoxDiagnosticDevice->setEnabled(true);
    ui->groupBoxDiagnosticMode->setEnabled(true);
}

/*
 * Напряжение замкнутой цепи группы
 */
void MainWindow::checkClosedCircuitVoltageGroup()
{
    //if (ui->rbModeDiagnosticAuto->isChecked() and bStop) return;
    if (bPause) return;
    ui->groupBoxCOMPort->setEnabled(false);
    ui->groupBoxDiagnosticDevice->setEnabled(false);
    ui->groupBoxDiagnosticMode->setEnabled(false);
    ui->tabWidget->addTab(ui->tabClosedCircuitVoltageGroup, ui->rbClosedCircuitVoltageGroup->text());
    //ui->tabWidget->setCurrentIndex(ui->tabWidget->count()-1);
    Log(tr("Проверка начата - %1").arg(ui->rbClosedCircuitVoltageGroup->text()), "blue");
    switch (iBatteryIndex) {
    case 0: //9ER20P-20
        while (iStepClosedCircuitVoltageGroup <= 1) {
            if (bPause) return;
            switch (iStepClosedCircuitVoltageGroup) {
            case 1:
                delay(1000);
                //Log(tr("1) между точкой металлизации и контактом 1 соединителя Х1 «Х1+» = <b>%1</b>").arg(QString::number(paramInsulationResistance1)), color);
                progressBarSet(1);
                break;
            default:
                break;
            }
            iStepClosedCircuitVoltageGroup++;
        }
        if (ui->rbModeDiagnosticAuto->isChecked())
            bCheckCompleteClosedCircuitVoltageGroup = true;
        break;
    case 1:
        if (bPause) return;
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(1);
        break;
    case 2:
        if (bPause) return;
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(1);
        break;
    case 3:
        if (bPause) return;
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(1);
        break;
    default:
        break;
    }
    Log(tr("Проверка завершена - %1").arg(ui->rbClosedCircuitVoltageGroup->text()), "blue");
    iStepClosedCircuitVoltageGroup = 1;
    ui->rbClosedCircuitVoltageBattery->setEnabled(true);
    ui->groupBoxCOMPort->setEnabled(true);
    ui->groupBoxDiagnosticDevice->setEnabled(true);
    ui->groupBoxDiagnosticMode->setEnabled(true);
}

/*
 * Напряжение замкнутой цепи батареи
 */
void MainWindow::checkClosedCircuitVoltageBattery()
{
    //if (ui->rbModeDiagnosticAuto->isChecked() and bStop) return;
    if (bPause) return;
    ui->groupBoxCOMPort->setEnabled(false);
    ui->groupBoxDiagnosticDevice->setEnabled(false);
    ui->groupBoxDiagnosticMode->setEnabled(false);
    ui->tabWidget->addTab(ui->tabClosedCircuitVoltageBattery, ui->rbClosedCircuitVoltageBattery->text());
    //ui->tabWidget->setCurrentIndex(ui->tabWidget->count()-1);
    Log(tr("Проверка начата - %1").arg(ui->rbClosedCircuitVoltageBattery->text()), "blue");
    switch (iBatteryIndex) {
    case 0: //9ER20P-20
        while (iStepClosedCircuitVoltageBattery <= 1) {
            if (bPause) return;
            switch (iStepClosedCircuitVoltageBattery) {
            case 1:
                delay(1000);
                //Log(tr("1) между точкой металлизации и контактом 1 соединителя Х1 «Х1+» = <b>%1</b>").arg(QString::number(paramInsulationResistance1)), color);
                progressBarSet(1);
                break;
            default:
                break;
            }
            iStepClosedCircuitVoltageBattery++;
        }
        if (ui->rbModeDiagnosticAuto->isChecked())
            bCheckCompleteClosedCircuitVoltageBattery = true;
        break;
    case 1:
        if (bPause) return;
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(1);
        break;
    case 2:
        if (bPause) return;
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(1);
        break;
    case 3:
        if (bPause) return;
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(1);
        break;
    default:
        break;
    }
    Log(tr("Проверка завершена - %1").arg(ui->rbClosedCircuitVoltageBattery->text()), "blue");
    iStepClosedCircuitVoltageBattery = 1;
    ui->rbInsulationResistanceMeasuringBoardUUTBB->setEnabled(true);
    ui->groupBoxCOMPort->setEnabled(true);
    ui->groupBoxDiagnosticDevice->setEnabled(true);
    ui->groupBoxDiagnosticMode->setEnabled(true);
}

/*
 * Сопротивление изоляции платы измерительной УУТББ
 */
void MainWindow::checkInsulationResistanceMeasuringBoardUUTBB()
{
    //if (ui->rbModeDiagnosticAuto->isChecked() and bStop) return;
    if (bPause) return;
    ui->groupBoxCOMPort->setEnabled(false);
    ui->groupBoxDiagnosticDevice->setEnabled(false);
    ui->groupBoxDiagnosticMode->setEnabled(false);
    ui->tabWidget->addTab(ui->tabInsulationResistanceMeasuringBoardUUTBB, ui->rbInsulationResistanceMeasuringBoardUUTBB->text());
    //ui->tabWidget->setCurrentIndex(ui->tabWidget->count()-1);
    Log(tr("Проверка начата - %1").arg(ui->rbInsulationResistanceMeasuringBoardUUTBB->text()), "blue");
    switch (iBatteryIndex) {
    case 0: //9ER20P-20
        while (iStepInsulationResistanceMeasuringBoardUUTBB <= 1) {
            if (bPause) return;
            switch (iStepInsulationResistanceMeasuringBoardUUTBB) {
            case 1:
                delay(1000);
                //Log(tr("1) между точкой металлизации и контактом 1 соединителя Х1 «Х1+» = <b>%1</b>").arg(QString::number(paramInsulationResistance1)), color);
                progressBarSet(1);
                break;
            default:
                break;
            }
            iStepInsulationResistanceMeasuringBoardUUTBB++;
        }
        if (ui->rbModeDiagnosticAuto->isChecked())
            bCheckCompleteInsulationResistanceMeasuringBoardUUTBB = true;
        break;
    case 1:
        if (bPause) return;
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(1);
        break;
    case 2:
        if (bPause) return;
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(1);
        break;
    case 3:
        if (bPause) return;
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(1);
        break;
    default:
        break;
    }

    Log(tr("Проверка завершена - %1").arg(ui->rbInsulationResistanceMeasuringBoardUUTBB->text()), "blue");
    iStepInsulationResistanceMeasuringBoardUUTBB = 1;
    ui->rbOpenCircuitVoltagePowerSupply->setEnabled(true);
    ui->groupBoxCOMPort->setEnabled(true);
    ui->groupBoxDiagnosticDevice->setEnabled(true);
    ui->groupBoxDiagnosticMode->setEnabled(true);
}

/*
 * Напряжение разомкнутой цепи блока питания
 */
void MainWindow::checkOpenCircuitVoltagePowerSupply()
{
    //if (ui->rbModeDiagnosticAuto->isChecked() and bStop) return;
    if (bPause) return;
    ui->groupBoxCOMPort->setEnabled(false);
    ui->groupBoxDiagnosticDevice->setEnabled(false);
    ui->groupBoxDiagnosticMode->setEnabled(false);
    ui->tabWidget->addTab(ui->tabOpenCircuitVoltagePowerSupply, ui->rbOpenCircuitVoltagePowerSupply->text());
    //ui->tabWidget->setCurrentIndex(ui->tabWidget->count()-1);
    Log(tr("Проверка начата - %1").arg(ui->rbOpenCircuitVoltagePowerSupply->text()), "blue");
    switch (iBatteryIndex) {
    case 0: //9ER20P-20
        while (iStepOpenCircuitVoltagePowerSupply <= 1) {
            if (bPause) return;
            switch (iStepOpenCircuitVoltagePowerSupply) {
            case 1:
                delay(1000);
                //Log(tr("1) между точкой металлизации и контактом 1 соединителя Х1 «Х1+» = <b>%1</b>").arg(QString::number(paramInsulationResistance1)), color);
                progressBarSet(1);
                break;
            default:
                break;
            }
            iStepOpenCircuitVoltagePowerSupply++;
        }
        if (ui->rbModeDiagnosticAuto->isChecked())
            bCheckCompleteOpenCircuitVoltagePowerSupply = true;
        break;
    case 1:
        if (bPause) return;
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(1);
        break;
    case 2:
        if (bPause) return;
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(1);
        break;
    case 3:
        if (bPause) return;
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(1);
        break;
    default:
        break;
    }
    Log(tr("Проверка завершена - %1").arg(ui->rbOpenCircuitVoltagePowerSupply->text()), "blue");
    iStepOpenCircuitVoltagePowerSupply = 1;
    ui->rbClosedCircuitVoltagePowerSupply->setEnabled(true);
    ui->groupBoxCOMPort->setEnabled(true);
    ui->groupBoxDiagnosticDevice->setEnabled(true);
    ui->groupBoxDiagnosticMode->setEnabled(true);
}

/*
 * Напряжение замкнутой цепи блока питания
 */
void MainWindow::checkClosedCircuitVoltagePowerSupply()
{
    //if (ui->rbModeDiagnosticAuto->isChecked() and bStop) return;
    if (bPause) return;
    ui->groupBoxCOMPort->setEnabled(false);
    ui->groupBoxDiagnosticDevice->setEnabled(false);
    ui->groupBoxDiagnosticMode->setEnabled(false);
    ui->tabWidget->addTab(ui->tabClosedCircuitVoltagePowerSupply, ui->rbClosedCircuitVoltagePowerSupply->text());
    //ui->tabWidget->setCurrentIndex(ui->tabWidget->count()-1);
    Log(tr("Проверка начата - %1").arg(ui->rbClosedCircuitVoltagePowerSupply->text()), "blue");
    switch (iBatteryIndex) {
    case 0: //9ER20P-20
        while (iStepClosedCircuitVoltagePowerSupply <= 1) {
            if (bPause) return;
            switch (iStepClosedCircuitVoltagePowerSupply) {
            case 1:
                delay(1000);
                //Log(tr("1) между точкой металлизации и контактом 1 соединителя Х1 «Х1+» = <b>%1</b>").arg(QString::number(paramInsulationResistance1)), color);
                progressBarSet(1);
                break;
            default:
                break;
            }
            iStepClosedCircuitVoltagePowerSupply++;
        }
        if (ui->rbModeDiagnosticAuto->isChecked())
            bCheckCompleteClosedCircuitVoltagePowerSupply = true;
        break;
    case 1:
        if (bPause) return;
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(1);
        break;
    case 2:
        if (bPause) return;
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(1);
        break;
    case 3:
        if (bPause) return;
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(1);
        break;
    default:
        break;
    } 
    Log(tr("Проверка завершена - %1").arg(ui->rbClosedCircuitVoltagePowerSupply->text()), "blue");
    iStepClosedCircuitVoltagePowerSupply = 1;
    ui->groupBoxCOMPort->setEnabled(true);
    ui->groupBoxDiagnosticDevice->setEnabled(true);
    ui->groupBoxDiagnosticMode->setEnabled(true);
}
