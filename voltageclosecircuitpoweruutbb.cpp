#include <QDebug>
#include <QMessageBox>
#include <QDateTime>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "battery.h"

extern QVector<Battery> battery;


// Нажата кнопка проверки напряжения замкнутой цепи БП УУТББ
void MainWindow::on_btnClosedCircuitVoltagePowerSupply_clicked()
{
    quint16 codeADC=0; // принятый код АЦП
    float fU=0; // принятое напряжение в вольтах
    // код порогового напряжения = пороговое напряжение В / коэфф. (вес разряда) + смещение (в коде)
    quint16 codeLimit=settings.uutbb_closecircuitpower_limit/settings.coefADC1 + settings.offsetADC1; // код, пороговое напряжение.
    int ret=0; // код возврата ошибки
    QDateTime starttime; // время начала измерения
    QDateTime dt; // текущее время очередного измерения
    double x; // текущая координата Х
    int cycleTimeSec=settings.uutbb_time_ccp; // длительность цикла проверки в секундах
    bool firstMeasurement=true; // первое измерение

    // Подготовка графика
    ui->widgetClosedCircuitVoltagePowerUUTBB->addGraph(); // blue line
    ui->widgetClosedCircuitVoltagePowerUUTBB->graph(0)->setPen(QPen(Qt::blue));
    ui->widgetClosedCircuitVoltagePowerUUTBB->graph(0)->clearData();
    ui->widgetClosedCircuitVoltagePowerUUTBB->addGraph(); // blue dot
    ui->widgetClosedCircuitVoltagePowerUUTBB->graph(1)->clearData();
    ui->widgetClosedCircuitVoltagePowerUUTBB->graph(1)->setLineStyle(QCPGraph::lsNone);
    //ui->widgetClosedCircuitVoltagePowerUUTBB->graph(1)->setPen(QPen(Qt::green));
    ui->widgetClosedCircuitVoltagePowerUUTBB->graph(1)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, Qt::blue, Qt::white, 7));
    ui->widgetClosedCircuitVoltagePowerUUTBB->addGraph(); // red line
    ui->widgetClosedCircuitVoltagePowerUUTBB->graph(2)->setPen(QPen(Qt::red));
    ui->widgetClosedCircuitVoltagePowerUUTBB->graph(2)->setBrush(QBrush(QColor(255, 0, 0, 20)));
    ui->widgetClosedCircuitVoltagePowerUUTBB->graph(2)->clearData();
    ui->widgetClosedCircuitVoltagePowerUUTBB->graph(2)->addData(0, settings.uutbb_closecircuitpower_limit);
    ui->widgetClosedCircuitVoltagePowerUUTBB->graph(2)->addData(cycleTimeSec+0.2, settings.uutbb_closecircuitpower_limit);

    ui->widgetClosedCircuitVoltagePowerUUTBB->xAxis->setLabel(tr("Время, c"));
    ui->widgetClosedCircuitVoltagePowerUUTBB->xAxis->setRange(0, cycleTimeSec+0.2);
    ui->widgetClosedCircuitVoltagePowerUUTBB->yAxis->setLabel(tr("Напряжение, В"));
    ui->widgetClosedCircuitVoltagePowerUUTBB->yAxis->setRange(0, 9);
    // показать закладку на экране
    ui->tabWidget->addTab(ui->tabClosedCircuitVoltagePowerSupply, ui->rbClosedCircuitVoltagePowerSupply->text());
    ui->tabWidget->setCurrentIndex(ui->tabWidget->count()-1);

    if(loop.isRunning()){qDebug()<<"loop.isRunning()!"; return;} // костыль: если цикл уже работает - выйти обратно
    timerPing->stop(); // остановить пинг

    baSendArray.clear();
    baSendCommand.clear();
    baRecvArray.clear();

    ui->statusBar->showMessage(tr("Проверка напряжения замкнутой цепи БП УУТББ ..."));
    Log(tr("Проверка напряжения замкнутой цепи БП УУТББ"), "blue");

    // сбросить коробочку
    baSendArray = (baSendCommand="IDLE")+"#"; // подготовить буфер для передачи
    sendSerialData(); // послать baSendArray в порт
    // ждём ответа. по сигналу о готовности принятых данных или по таймауту, вывалимся из цикла
    ret=loop.exec();
    if(ret) goto stop; // если не ноль (ошибка таймаута) - вывалиться из режима. если 0, то приняли данные из порта

    // собрать режим
    baSendArray=(baSendCommand="UccPB")+"#";
    if(bDeveloperState) Log(QString("Sending ") + qPrintable(baSendArray), "blue");
    QTimer::singleShot(settings.delay_after_IDLE_before_other, this, SLOT(sendSerialData()));
    ret=loop.exec();
    if(ret) goto stop;

    starttime = QDateTime::currentDateTime(); // время начала измерения
    dt = QDateTime::currentDateTime(); // текущее время
    ui->widgetClosedCircuitVoltagePowerUUTBB->graph(0)->clearData(); // очистить график

    while(-dt.msecsTo(starttime) < cycleTimeSec*1000) // пока время цикла проверки не вышло, продолжим измерять
    {
        // опросить
        baSendArray=baSendCommand+"?#";
        QTimer::singleShot(settings.delay_after_request_before_next_ADC1, this, SLOT(sendSerialData()));
        ret=loop.exec();
        if(ret) goto stop;
        codeADC = getRecvData(baRecvArray); // напряжение в коде
        fU = ((codeADC-settings.offsetADC1)*settings.coefADC1); // напряжение в вольтах
        // нарисуем график
        if(firstMeasurement)
        {
            firstMeasurement = false;
            starttime = QDateTime::currentDateTime(); // время начала измерения начнём считать после получения первого ответа (чтобы график рисовался с нуля)
        }
        dt = QDateTime::currentDateTime(); // текущее время
        x= -dt.msecsTo(starttime); // кол-во миллисекунд, прошедших с начала измерения
        ui->widgetClosedCircuitVoltagePowerUUTBB->graph(0)->rescaleValueAxis(true); // для автоматического перерисовывания шкалы графика, если значения за пределами экрана
        ui->widgetClosedCircuitVoltagePowerUUTBB->graph(0)->addData((double)x/1000, (double)fU);
        ui->widgetClosedCircuitVoltagePowerUUTBB->replot();
    }

    if(bDeveloperState)
        Log("Цепь "+battery[iBatteryIndex].circuitbattery+" Receive "+qPrintable(baRecvArray)+" codeADC1=0x"+QString("%1").arg((ushort)codeADC, 0, 16), "blue");

    // проанализировать результаты
    if(codeADC >= codeLimit) // напряжение больше (норма)
    {
        Log("Напряжение цепи "+battery[iBatteryIndex].circuitbattery+" = "+QString::number(fU, 'f', 2)+" В.  Норма.", "blue");
        // если ручной режим, то выдать окно сообщения, и только потом разобрать режим измерения.
        if(bModeManual) QMessageBox::information(this, tr("Напряжение замкнутой цепи БП УУТББ"), tr("Напряжение цепи ")+battery[iBatteryIndex].circuitbattery+" = "+QString::number(fU, 'f', 2)+" В\nНорма");
    }
    else // напряжение меньше (не норма)
    {
        Log("Напряжение цепи "+battery[iBatteryIndex].circuitbattery+" = "+QString::number(fU, 'f', 2)+" В.  Не норма!.", "red");
        // если ручной режим, то выдать окно сообщения, и только потом разобрать режим измерения.
        if(bModeManual) QMessageBox::information(this, tr("Напряжение замкнутой цепи БП УУТББ"), tr("Напряжение цепи ")+battery[iBatteryIndex].circuitbattery+" = "+QString::number(fU, 'f', 2)+" В\nНе норма!");
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
 * Напряжение замкнутой цепи блока питания
 */
void MainWindow::checkClosedCircuitVoltagePowerSupply()
{
    //if (ui->rbModeDiagnosticAuto->isChecked() and bStop) return;
    if (!bState) return;
    ui->groupBoxCOMPort->setEnabled(false);
    ui->groupBoxDiagnosticDevice->setEnabled(false);
    ui->groupBoxDiagnosticMode->setEnabled(false);
    ui->tabWidget->addTab(ui->tabClosedCircuitVoltagePowerSupply, ui->rbClosedCircuitVoltagePowerSupply->text());
    ui->tabWidget->setCurrentIndex(ui->tabWidget->count()-1);
    Log(tr("Проверка начата - %1").arg(ui->rbClosedCircuitVoltagePowerSupply->text()), "blue");
    switch (iBatteryIndex) {
    case 0: //9ER20P-20
        /*while (iStepClosedCircuitVoltagePowerSupply <= 1) {
            if (!bState) return;
            switch (iStepClosedCircuitVoltagePowerSupply) {
            case 1:
                delay(1000);
                //Log(tr("1) между точкой металлизации и контактом 1 соединителя Х1 «Х1+» = <b>%1</b>").arg(QString::number(paramInsulationResistance1)), color);
                break;
            default:
                break;
            }
            iStepClosedCircuitVoltagePowerSupply++;
        }*/
        /*if (ui->rbModeDiagnosticAuto->isChecked())
            bCheckCompleteClosedCircuitVoltagePowerSupply = true;*/
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
    Log(tr("Проверка завершена - %1").arg(ui->rbClosedCircuitVoltagePowerSupply->text()), "blue");
    //iStepClosedCircuitVoltagePowerSupply = 1;
    ui->groupBoxCOMPort->setEnabled(true);
    ui->groupBoxDiagnosticDevice->setEnabled(true);
    ui->groupBoxDiagnosticMode->setEnabled(true);
}
