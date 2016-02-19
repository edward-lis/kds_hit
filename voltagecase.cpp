#include <QDebug>
#include <QMessageBox>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "battery.h"

extern QVector<Battery> battery;

void MainWindow::on_btnVoltageOnTheHousing_clicked()
{
    int ret=0; // код возврата ошибки
    quint16 codeU=0; // код напряжение минус, плюс
    // код порогового напряжения = пороговое напряжение В / коэфф. (вес разряда) + смещение (в коде)
    quint16 codeLimit=settings.voltage_corpus_limit/settings.coefADC2 + settings.offsetADC2; // код, пороговое напряжение.
    float voltageU=0;
    qDebug()<<"on_btnVoltageOnTheHousing_clicked";
    //qDebug()<<ui->rbModeDiagnosticAuto->isChecked()<<ui->rbModeDiagnosticManual->isChecked()<<ui->cbVoltageOnTheHousing->currentIndex()<<Ucase;

    if(loop.isRunning()){qDebug()<<"loop.isRunning()!"; return;} // костыль: если цикл уже работает - выйти обратно
    ui->btnVoltageOnTheHousing->setEnabled(false); // на время проверки запретить кнопку
    timerPing->stop(); // остановить пинг

    baSendArray.clear();
    baSendCommand.clear();
    baRecvArray.clear();

    ui->statusBar->showMessage(tr("Проверка напряжения на корпусе ..."));
//    ui->progressBar->setValue(ui->progressBar->value()+1);
    Log(tr("Проверка напряжения на корпусе"), "blue");

    // сбросить коробочку
    baSendArray = (baSendCommand="IDLE")+"#"; // подготовить буфер для передачи
    sendSerialData(); // послать baSendArray в порт
    // ждём ответа. по сигналу о готовности принятых данных или по таймауту, вывалимся из цикла
    ret=loop.exec();
    if(ret) goto stop; // если не ноль (ошибка таймаута) - вывалиться из режима. если 0, то приняли данные из порта

//    if(bDeveloperState || ui->rbModeDiagnosticManual->isChecked())// если в ручном режиме
    {
        if(ui->cbVoltageOnTheHousing->currentIndex() == 1) // если выбрана в комбобоксе такая цепь
        {
            baSendArray=(baSendCommand="UcaseM")+"#";
            if(bDeveloperState) Log(QString("Sending ") + qPrintable(baSendArray), "blue");
            QTimer::singleShot(settings.delay_after_IDLE_before_other, this, SLOT(sendSerialData())); // послать baSendArray в порт через некоторое время
            ret=loop.exec();
            if(ret) goto stop;

            baSendArray=baSendCommand+"?#";
            QTimer::singleShot(settings.delay_after_start_before_request_ADC2, this, SLOT(sendSerialData()));
            ret=loop.exec();
            if(ret) goto stop;
            codeU = getRecvData(baRecvArray); // получить данные опроса
        }
        else
        {
            baSendArray=(baSendCommand="UcaseP")+"#";
            if(bDeveloperState) Log(QString("Sending ") + qPrintable(baSendArray), "blue");
            QTimer::singleShot(settings.delay_after_IDLE_before_other, this, SLOT(sendSerialData())); // послать baSendArray в порт через некоторое время
            ret=loop.exec();
            if(ret) goto stop;

            baSendArray=baSendCommand+"?#";
            QTimer::singleShot(settings.delay_after_start_before_request_ADC2, this, SLOT(sendSerialData()));
            ret=loop.exec();
            if(ret) goto stop;
            codeU = getRecvData(baRecvArray); // получить данные опроса
        }
        voltageU = ((codeU-settings.offsetADC2)*settings.coefADC2); // напряжение в вольтах
//        ui->progressBar->setValue(ui->progressBar->value()+1);

        if(codeU > codeLimit) // напряжение больше
        {
            qDebug()<<baSendCommand<<" > "<<codeU;
            Log("Напряжение цепи "+battery[iBatteryIndex].str_voltage_corpus[ui->cbVoltageOnTheHousing->currentIndex()]+" = "+QString::number(voltageU, 'f', 2)+" В. Не норма!", "red");
//            if(ui->rbModeDiagnosticAuto->isChecked())// если в автоматическом режиме
            {
                // !!! переход в ручной режим
                //if(!bModeManual && !bDeveloperState)QMessageBox::critical(this, "Не норма!", "Напряжение цепи "+battery[iBatteryIndex].str_voltage_corpus[ui->cbVoltageOnTheHousing->currentIndex()]+" = "+QString::number(voltageU)+" В больше нормы");// !!!
            }
        }
        if(codeU <= codeLimit) // напряжение в норме
        {
            qDebug()<<baSendCommand<<" norm"<<codeU;
            Log("Напряжение цепи "+battery[iBatteryIndex].str_voltage_corpus[ui->cbVoltageOnTheHousing->currentIndex()]+" = "+QString::number(voltageU, 'f', 2)+" В.  Норма.", "blue");
        }
    }
    // если отладочный режим, напечатать отладочную инфу
    if(bDeveloperState)
    {
        Log(QString("k = ") + qPrintable(QString::number(settings.coefADC2)) + " код смещения offset =0x "+settings.offsetADC2+ " код АЦП2 = 0x" + qPrintable(QString::number(codeU, 16)) + " U=k*(code-offset) = " + QString::number(voltageU), "green");
    }

    // если ручной режим, то выдать окно сообщения, и только потом разобрать режим измерения.
    if(bModeManual) QMessageBox::information(this, tr("Напряжение на корпусе"), tr("Напряжение цепи ")+battery[iBatteryIndex].str_voltage_corpus[ui->cbVoltageOnTheHousing->currentIndex()]+" = "+QString::number(voltageU, 'f', 2)+" В");
stop:
    // сбросить коробочку
    baSendArray = (baSendCommand="IDLE")+"#";
    QTimer::singleShot(settings.delay_after_request_before_next_ADC2, this, SLOT(sendSerialData()));
    loop.exec();

    // если отладочный режим, напечатать отладочную инфу
    if(bDeveloperState)
    {
        if(ret==1) Log(tr("Timeout!"), "red");
        else if(ret==2) Log(tr("Incorrect reply!"), "red");
    }
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
