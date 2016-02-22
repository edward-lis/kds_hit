#include <QDebug>
#include <QMessageBox>
#include <QDateTime>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "battery.h"

extern QVector<Battery> battery;

// Нажата кнопка проверки напряжения замкнутой цепи батареи
void MainWindow::on_btnClosedCircuitVoltageBattery_clicked()
{
    quint16 codeADC=0; // принятый код АЦП
    float fU=0; // принятое напряжение в вольтах
    // код порогового напряжения = пороговое напряжение В / коэфф. (вес разряда) + смещение (в коде)
    quint16 codeLimit=settings.closecircuitbattery_limit/settings.coefADC1 + settings.offsetADC1; // код, пороговое напряжение.
    int ret=0; // код возврата ошибки

    if(loop.isRunning()){qDebug()<<"loop.isRunning()!"; return;} // костыль: если цикл уже работает - выйти обратно
    timerPing->stop(); // остановить пинг

    baSendArray.clear();
    baSendCommand.clear();
    baRecvArray.clear();

    ui->statusBar->showMessage(tr("Проверка напряжения замкнутой цепи батареи ..."));
    Log(tr("Проверка напряжения замкнутой цепи батареи"), "blue");

    // сбросить коробочку
    baSendArray = (baSendCommand="IDLE")+"#"; // подготовить буфер для передачи
    sendSerialData(); // послать baSendArray в порт
    // ждём ответа. по сигналу о готовности принятых данных или по таймауту, вывалимся из цикла
    ret=loop.exec();
    if(ret) goto stop; // если не ноль (ошибка таймаута) - вывалиться из режима. если 0, то приняли данные из порта

    // собрать режим
    baSendArray=(baSendCommand="UccB")+"#";
    if(bDeveloperState) Log(QString("Sending ") + qPrintable(baSendArray), "blue");
    QTimer::singleShot(settings.delay_after_IDLE_before_other, this, SLOT(sendSerialData()));
    ret=loop.exec();
    if(ret) goto stop;

    // опросить
    baSendArray=baSendCommand+"?#";
    QTimer::singleShot(settings.delay_after_start_before_request_ADC1, this, SLOT(sendSerialData()));
    ret=loop.exec();
    if(ret) goto stop;
    codeADC = getRecvData(baRecvArray);

    fU = ((codeADC-settings.offsetADC1)*settings.coefADC1); // напряжение в вольтах

    if(bDeveloperState)
        Log("Цепь "+battery[iBatteryIndex].circuitbattery+" Receive "+qPrintable(baRecvArray)+" codeADC1=0x"+QString("%1").arg((ushort)codeADC, 0, 16), "blue");

    // проанализировать результаты
    if(codeADC >= codeLimit) // напряжение больше (норма)
    {
        Log("Напряжение цепи "+battery[iBatteryIndex].circuitbattery+" = "+QString::number(fU, 'f', 2)+" В.  Норма.", "blue");
        // если ручной режим, то выдать окно сообщения, и только потом разобрать режим измерения.
        if(bModeManual) QMessageBox::information(this, tr("Напряжение замкнутой цепи батареи"), tr("Напряжение цепи ")+battery[iBatteryIndex].circuitbattery+" = "+QString::number(fU, 'f', 2)+" В\nНорма");
    }
    else // напряжение меньше (не норма)
    {
        Log("Напряжение цепи "+battery[iBatteryIndex].circuitbattery+" = "+QString::number(fU, 'f', 2)+" В.  Не норма!.", "red");
        // если ручной режим, то выдать окно сообщения, и только потом разобрать режим измерения.
        if(bModeManual) QMessageBox::information(this, tr("Напряжение замкнутой цепи батареи"), tr("Напряжение цепи ")+battery[iBatteryIndex].circuitbattery+" = "+QString::number(fU, 'f', 2)+" В\nНе норма!");
        // !!! добавить цепь в список неисправных, запрет проверки батареи под нагрузкой
    }

    // разобрать режим
    baSendArray = (baSendCommand="IDLE")+"#";
    QTimer::singleShot(settings.delay_after_request_before_next_ADC1, this, SLOT(sendSerialData()));
    ret=loop.exec();
    if(ret) goto stop;

stop:
    // если отладочный режим, напечатать отладочную инфу
    if(bDeveloperState)
    {
        if(ret==1) Log(tr("Timeout!"), "red");
        else if(ret==2) Log(tr("Incorrect reply!"), "red");
    }

    timerPing->start(delay_timerPing); // запустить пинг по выходу из режима
    baSendArray.clear(); // очистить буфера команд.
    baSendCommand.clear();
    baRecvArray.clear();
}

/*
 * Напряжение замкнутой цепи батареи
 */
void MainWindow::checkClosedCircuitVoltageBattery()
{
    if (((QPushButton*)sender())->objectName() == "btnClosedCircuitVoltageBattery") {
        iStepClosedCircuitVoltageBattery = 1;
        bState = false;
        //ui->btnClosedCircuitVoltageBattery_2->setEnabled(false);
    }
    if (((QPushButton*)sender())->objectName() == "btnClosedCircuitVoltageBattery_2")
        bState = false;
    if (!bState) return;
    ui->groupBoxCOMPort->setEnabled(false);
    ui->groupBoxDiagnosticDevice->setEnabled(false);
    ui->groupBoxDiagnosticMode->setEnabled(false);
    ui->tabWidget->addTab(ui->tabClosedCircuitVoltageBattery, ui->rbClosedCircuitVoltageBattery->text());
    ui->tabWidget->setCurrentIndex(ui->tabWidget->count()-1);
    Log(tr("Проверка начата - %1").arg(ui->rbClosedCircuitVoltageBattery->text()), "blue");
    switch (iBatteryIndex) {
    case 0: //9ER20P-20
        if (!bState) return;
        delay(1000);
        ui->labelClosedCircuitVoltageBattery->setText(tr("1) %2").arg(QString::number(param)));
        str = tr("1) между контактом 1 соединителя Х1 «1+» и контактом 1 соединителя Х3 «3-» = <b>%2</b>").arg(QString::number(param));
        Log(str, (param > 30.0) ? "red" : "green");

        if (param > 30.0) {
            ui->rbModeDiagnosticManual->setChecked(true);
            ui->rbModeDiagnosticAuto->setEnabled(false);
            if (QMessageBox::question(this, "Внимание - "+ui->rbClosedCircuitVoltageBattery->text(), tr("%1 \nпродолжить?").arg(str), tr("Да"), tr("Нет"))) {
                //ui->btnClosedCircuitVoltageBattery_2->setEnabled(true);
                bState = true;
                return;
            }
        }
        //ui->btnClosedCircuitVoltageBattery_2->setEnabled(false);
        if (ui->rbModeDiagnosticAuto->isChecked())
            bCheckCompleteClosedCircuitVoltageBattery = true;
        break;
    case 1:
        if (!bState) return;
        Log("Действия проверки.", "green");
        delay(1000);

        break;
    case 2:
        if (!bState) return;
        Log("Действия проверки.", "green");
        delay(1000);

        break;
    case 3:
        if (!bState) return;
        Log("Действия проверки.", "green");
        delay(1000);

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
