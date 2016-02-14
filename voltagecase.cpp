#include <QDebug>
#include <QMessageBox>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "battery.h"

extern QVector<Battery> battery;

void MainWindow::on_btnVoltageOnTheHousing_clicked()
{
    quint16 uCaseM, uCaseP, u; // напряжение минус, плюс
    quint16 Ucase=settings.voltage_corpus_limit/settings.coefADC2; // код, пороговое напряжение.
    qDebug()<<"on_btnVoltageOnTheHousing_clicked";
    qDebug()<<ui->rbModeDiagnosticAuto->isChecked()<<ui->rbModeDiagnosticManual->isChecked()<<ui->cbVoltageOnTheHousing->currentIndex()<<Ucase;

    if(loop.isRunning()){qDebug()<<"loop.isRunning()!"; return;} // костыль: если цикл уже работает - выйти обратно
    ui->btnVoltageOnTheHousing->setEnabled(false); // на время проверки запретить кнопку
    timerPing->stop(); // остановить пинг

    baSendArray.clear();
    baSendCommand.clear();
    baRecvArray.clear();

    ui->statusBar->showMessage(tr("Проверка напряжения на корпусе ..."));
    ui->progressBar->setValue(ui->progressBar->value()+1);
    Log(tr("Проверка напряжения на корпусе"), "blue");

    // сбросить коробочку
    baSendArray = (baSendCommand="IDLE")+"#"; // подготовить буфер для передачи
    sendSerialData(); // послать baSendArray в порт
    // ждём ответа. по сигналу о готовности принятых данных или по таймауту, вывалимся из цикла
    if(loop.exec()) goto stop; // если не ноль (ошибка таймаута) - вывалиться из режима. если 0, то приняли данные из порта

    if(bDeveloperState || ui->rbModeDiagnosticManual->isChecked())// если в ручном режиме
    {
        if(ui->cbVoltageOnTheHousing->currentIndex() == 1) // если выбрана в комбобоксе такая цепь
        {
            baSendArray=(baSendCommand="Ucase")+"M#";
            QTimer::singleShot(delay_after_IDLE_before_other, this, SLOT(sendSerialData())); // послать baSendArray в порт через некоторое время
            if(loop.exec()) goto stop; // если ошибка - вывалиться из режима

            baSendArray=baSendCommand+"?#";
            QTimer::singleShot(delay_command_after_start_before_request, this, SLOT(sendSerialData()));
            if(loop.exec()) goto stop; // если ошибка - вывалиться из режима
            u=uCaseM = getRecvData(baRecvArray); // получить данные опроса
        }
        else
        {
            baSendArray=(baSendCommand="Ucase")+"P#";
            QTimer::singleShot(delay_after_IDLE_before_other, this, SLOT(sendSerialData())); // послать baSendArray в порт через некоторое время
            if(loop.exec()) goto stop; // если ошибка - вывалиться из режима

            baSendArray=baSendCommand+"?#";
            QTimer::singleShot(delay_command_after_start_before_request, this, SLOT(sendSerialData()));
            if(loop.exec()) goto stop; // если ошибка - вывалиться из режима
            u=uCaseP = getRecvData(baRecvArray); // получить данные опроса
        }
        ui->progressBar->setValue(ui->progressBar->value()+1);

        // сбросить коробочку
        baSendArray = (baSendCommand="IDLE")+"#";
        QTimer::singleShot(delay_command_after_request_before_next, this, SLOT(sendSerialData()));
        if(loop.exec()) goto stop; // если ошибка - вывалиться из режима

        if(u > Ucase) // напряжение больше
        {
            qDebug()<<baSendCommand<<" > "<<u;
            Log("Напряжение цепи "+battery[iBatteryIndex].str_voltage_corpus[ui->cbVoltageOnTheHousing->currentIndex()]+" = "+QString::number((float)(u*settings.coefADC2))+" В. Не норма!", "red"); // !!! отформатировать вывод напряжения!
            if(ui->rbModeDiagnosticAuto->isChecked())// если в автоматическом режиме
            {
                // !!! переход в ручной режим
                if(!bDeveloperState)QMessageBox::critical(this, "Не норма!", "Напряжение цепи "+battery[iBatteryIndex].str_voltage_corpus[ui->cbVoltageOnTheHousing->currentIndex()]+" = "+QString::number((float)(u*settings.coefADC2))+" В больше нормы");// !!!
            }
        }
        if(u <= Ucase) // напряжение в норме
        {
            qDebug()<<baSendCommand<<" norm"<<u;
            Log("Напряжение цепи "+battery[iBatteryIndex].str_voltage_corpus[ui->cbVoltageOnTheHousing->currentIndex()]+" = "+QString::number((float)(u*settings.coefADC2))+" В.  Норма.", "blue"); // !!!
        }
    }
stop:
    // если отладочный режим, напечатать отладочную инфу
    if(bDeveloperState) Log(QString("k = ") + qPrintable(QString::number(settings.coefADC2)) + " code = 0x" + qPrintable(QString::number(u, 16)) + " U=k*code = " + QString::number((float)(u*settings.coefADC2)), "green");

    ui->btnVoltageOnTheHousing->setEnabled(true); // разрешить кнопку
    timerPing->start(delay_timerPing); // запустить пинг по выходу из режима
    baSendArray.clear(); // надо ли?
    baSendCommand.clear();
    baRecvArray.clear();
// !!! сбросить прогрессбар
}
#if 0
on_btnVoltageOnTheHousing_clicked
true false 1 1872
Incorrect reply. Should be  "IDLE and OK"  but got:  "PING"
recvSerialData "IDLE#OK" command: ""
#endif
