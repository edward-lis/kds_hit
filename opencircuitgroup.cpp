#include <QDebug>
#include <QMessageBox>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "battery.h"

extern QVector<Battery> battery;

// Нажата кнопка проверки напряжения разомкнутых цепей групп
void MainWindow::on_btnOpenCircuitVoltageGroup_clicked()
{
    quint16 codeADC=0; // принятый код АЦП
    float fU=0; // принятое напряжение в вольтах
    // код порогового напряжения = пороговое напряжение В / коэфф. (вес разряда) + смещение (в коде)
    quint16 codeLimit=settings.opencircuitgroup_limit_min/settings.coefADC1 + settings.offsetADC1; // код, пороговое напряжение.
    int ret=0; // код возврата ошибки
    QString str_num; // номер цепи

    if(loop.isRunning()){qDebug()<<"loop.isRunning()!"; return;} // костыль: если цикл уже работает - выйти обратно
    timerPing->stop(); // остановить пинг

    baSendArray.clear();
    baSendCommand.clear();
    baRecvArray.clear();

    ui->statusBar->showMessage(tr("Проверка напряжения разомкнутых цепей групп ..."));
    Log(tr("Проверка напряжения разомкнутых цепей групп"), "blue");

    // сбросить коробочку
    baSendArray = (baSendCommand="IDLE")+"#"; // подготовить буфер для передачи
    sendSerialData(); // послать baSendArray в порт
    // ждём ответа. по сигналу о готовности принятых данных или по таймауту, вывалимся из цикла
    ret=loop.exec();
    if(ret) goto stop; // если не ноль (ошибка таймаута) - вывалиться из режима. если 0, то приняли данные из порта

    // !!! если нечего проверять, список пуст - проверить алгоритм
    // Пробежимся по списку цепей
    for(int i=1; i < modelOpenCircuitVoltageGroup->rowCount(); i++)
    {
        QStandardItem *sitm = modelOpenCircuitVoltageGroup->item(i, 0);
        Qt::CheckState checkState = sitm->checkState();
        if (checkState != Qt::Checked) continue;

        // собрать режим
        str_num.sprintf(" %02i", i); // напечатать номер цепи
        baSendArray=(baSendCommand="UocG")+str_num.toLocal8Bit()+"#";
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
            Log("Цепь "+battery[iBatteryIndex].circuitgroup[i-1]+" Receive "+qPrintable(baRecvArray)+" codeADC1=0x"+QString("%1").arg((ushort)codeADC, 0, 16), "blue");

        // проанализировать результаты
        if(codeADC >= codeLimit) // напряжение больше (норма)
        {
            Log("Напряжение цепи "+battery[iBatteryIndex].circuitgroup[i-1]+" = "+QString::number(fU, 'f', 2)+" В.  Норма.", "blue");
            // если ручной режим, то выдать окно сообщения, и только потом разобрать режим измерения.
            if(bModeManual) QMessageBox::information(this, tr("Напряжение разомкнутой цепи группы"), tr("Напряжение цепи ")+battery[iBatteryIndex].circuitgroup[i-1]+" = "+QString::number(fU, 'f', 2)+" В\nНорма");
        }
        else // напряжение меньше (не норма)
        {
            Log("Напряжение цепи "+battery[iBatteryIndex].circuitgroup[i-1]+" = "+QString::number(fU, 'f', 2)+" В.  Не норма!.", "red");
            // если ручной режим, то выдать окно сообщения, и только потом разобрать режим измерения.
            if(bModeManual) QMessageBox::information(this, tr("Напряжение разомкнутой цепи группы"), tr("Напряжение цепи ")+battery[iBatteryIndex].circuitgroup[i-1]+" = "+QString::number(fU, 'f', 2)+" В\nНе норма!");
            // !!! добавить цепь в список неисправных, запрет проверки цепи под нагрузкой
        }

        // разобрать режим
        baSendArray = (baSendCommand="IDLE")+"#";
        QTimer::singleShot(settings.delay_after_request_before_next_ADC1, this, SLOT(sendSerialData()));
        ret=loop.exec();
        if(ret) goto stop;
    }

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
