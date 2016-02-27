#include <QDebug>
#include <QMessageBox>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "battery.h"

extern QVector<Battery> battery;


// Нажата кнопка проверки напряжения разомкнутой цепи батареи
void MainWindow::on_btnOpenCircuitVoltageBattery_clicked()
{
    quint16 codeADC=0; // принятый код АЦП
    float fU=0; // принятое напряжение в вольтах
    // код порогового напряжения = пороговое напряжение В / коэфф. (вес разряда) + смещение (в коде)
    quint16 codeLimit=settings.opencircuitbattery_limit/settings.coefADC1 + settings.offsetADC1; // код, пороговое напряжение.
    int ret=0; // код возврата ошибки

    if(loop.isRunning()){qDebug()<<"loop.isRunning()!"; return;} // костыль: если цикл уже работает - выйти обратно
    timerPing->stop(); // остановить пинг

    baSendArray.clear();
    baSendCommand.clear();
    baRecvArray.clear();

    ui->statusBar->showMessage(tr("Проверка напряжения разомкнутой цепи батареи ..."));
    Log(tr("Проверка напряжения разомкнутой цепи батареи"), "blue");

    // сбросить коробочку
    baSendArray = (baSendCommand="IDLE")+"#"; // подготовить буфер для передачи
    sendSerialData(); // послать baSendArray в порт
    // ждём ответа. по сигналу о готовности принятых данных или по таймауту, вывалимся из цикла
    ret=loop.exec();
    if(ret) goto stop; // если не ноль (ошибка таймаута) - вывалиться из режима. если 0, то приняли данные из порта

    // собрать режим
    baSendArray=(baSendCommand="UocB")+"#";
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
        if(bModeManual) QMessageBox::information(this, tr("Напряжение разомкнутой цепи батареи"), tr("Напряжение цепи ")+battery[iBatteryIndex].circuitbattery+" = "+QString::number(fU, 'f', 2)+" В\nНорма");
    }
    else // напряжение меньше (не норма)
    {
        Log("Напряжение цепи "+battery[iBatteryIndex].circuitbattery+" = "+QString::number(fU, 'f', 2)+" В.  Не норма!.", "red");
        // если ручной режим, то выдать окно сообщения, и только потом разобрать режим измерения.
        if(bModeManual) QMessageBox::information(this, tr("Напряжение разомкнутой цепи батареи"), tr("Напряжение цепи ")+battery[iBatteryIndex].circuitbattery+" = "+QString::number(fU, 'f', 2)+" В\nНе норма!");
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
 * Напряжение разомкнутой цепи батареи
 */
void MainWindow::checkOpenCircuitVoltageBattery()
{
    qDebug() << "sender=" << ((QPushButton*)sender())->objectName() << "bState=" << bState;
    ui->tabWidget->addTab(ui->tabOpenCircuitVoltageBattery, ui->rbOpenCircuitVoltageBattery->text());
    ui->tabWidget->setCurrentIndex(ui->tabWidget->count()-1);
    Log(tr("Проверка начата - %1").arg(ui->rbOpenCircuitVoltageBattery->text()), "blue");

    if(ui->rbModeDiagnosticManual->isChecked()) {
        if(!bState) {
            bState = true;
            ui->groupBoxCheckParams->setEnabled(bState);
            ((QPushButton*)sender())->setText("Стоп");
        } else {
            bState = false;
            ((QPushButton*)sender())->setText("Пуск");
        }
    } else
        ui->cbParamsAutoMode->setCurrentIndex(3); // переключаем режим комбокса на наш

    ui->groupBoxCOMPort->setDisabled(bState);
    ui->groupBoxDiagnosticDevice->setDisabled(bState);
    ui->groupBoxDiagnosticMode->setDisabled(bState);
    ui->cbParamsAutoMode->setDisabled(bState);
    ui->cbSubParamsAutoMode->setDisabled(bState);

    ui->progressBar->setMaximum(1);
    ui->progressBar->setValue(0);


    if (!bState) return;
    dArrayOpenCircuitVoltageBattery[0] = randMToN(31,33); //число полученное с COM-порта

    str = tr("Напряжение цепи \"%0\" = <b>%1</b> В.").arg(battery[iBatteryIndex].circuitbattery).arg(dArrayOpenCircuitVoltageBattery[0]);
    if (dArrayOpenCircuitVoltageBattery[0] > settings.opencircuitbattery_limit) {
        str += " Не норма.";
        color = "red";
    } else
        color = "green";
    ui->labelOpenCircuitVoltageBattery0->setText(str);
    ui->labelOpenCircuitVoltageBattery0->setStyleSheet("QLabel { color : "+color+"; }");
    Log(str, color);
    ui->btnBuildReport->setEnabled(true);
    if (dArrayOpenCircuitVoltageBattery[0] > settings.opencircuitbattery_limit) {
        if (QMessageBox::question(this, "Внимание - "+ui->rbOpenCircuitVoltageBattery->text(), tr("%0 Продолжить?").arg(str), tr("Да"), tr("Нет"))) {
            bState = false;
            ui->groupBoxCOMPort->setDisabled(bState);
            ui->groupBoxDiagnosticMode->setDisabled(bState);
            ui->cbParamsAutoMode->setDisabled(bState);
            ui->cbSubParamsAutoMode->setDisabled(bState);
            ((QPushButton*)sender())->setText("Пуск");
            return;
        }
    }

    ui->progressBar->setValue(1);

    Log(tr("Проверка завершена - %1").arg(ui->rbOpenCircuitVoltageBattery->text()), "blue");

    if(ui->rbModeDiagnosticManual->isChecked()) {
        bState = false;
        ui->groupBoxCOMPort->setEnabled(bState);
        ui->groupBoxDiagnosticDevice->setDisabled(bState);
        ui->groupBoxDiagnosticMode->setDisabled(bState);
        ui->cbParamsAutoMode->setDisabled(bState);
        ui->cbSubParamsAutoMode->setDisabled(bState);
        ((QPushButton*)sender())->setText("Пуск");
    }
}
