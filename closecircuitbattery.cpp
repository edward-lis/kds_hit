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
    QDateTime starttime; // время начала измерения
    QDateTime dt; // текущее время очередного измерения
    double x; // текущая координата Х
    int cycleTimeSec=settings.time_closecircuitbattery; // длительность цикла проверки в секундах
    bool firstMeasurement=true; // первое измерение

    // Подготовка графика
    ui->widgetClosedCircuitBattery->addGraph(); // blue line
    ui->widgetClosedCircuitBattery->graph(0)->setPen(QPen(Qt::blue));
    ui->widgetClosedCircuitBattery->graph(0)->clearData();
    ui->widgetClosedCircuitBattery->addGraph(); // blue dot
    ui->widgetClosedCircuitBattery->graph(1)->clearData();
    ui->widgetClosedCircuitBattery->graph(1)->setLineStyle(QCPGraph::lsNone);
    //ui->widgetClosedCircuitVoltagePowerUUTBB->graph(1)->setPen(QPen(Qt::green));
    ui->widgetClosedCircuitBattery->graph(1)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, Qt::blue, Qt::white, 7));
    ui->widgetClosedCircuitBattery->addGraph(); // red line
    ui->widgetClosedCircuitBattery->graph(2)->setPen(QPen(Qt::red));
    ui->widgetClosedCircuitBattery->graph(2)->setBrush(QBrush(QColor(255, 0, 0, 20)));
    ui->widgetClosedCircuitBattery->graph(2)->clearData();
    ui->widgetClosedCircuitBattery->graph(2)->addData(0, settings.closecircuitbattery_limit);
    ui->widgetClosedCircuitBattery->graph(2)->addData(cycleTimeSec+2, settings.closecircuitbattery_limit);

    ui->widgetClosedCircuitBattery->xAxis->setLabel(tr("Время, c"));
    ui->widgetClosedCircuitBattery->xAxis->setRange(0, cycleTimeSec+2);
    ui->widgetClosedCircuitBattery->yAxis->setLabel(tr("Напряжение, В"));
    ui->widgetClosedCircuitBattery->yAxis->setRange(24, 33);
    // показать закладку на экране
    ui->tabWidget->addTab(ui->tabClosedCircuitVoltageBattery, ui->rbClosedCircuitVoltageBattery->text());
    ui->tabWidget->setCurrentIndex(ui->tabWidget->count()-1);

    if(loop.isRunning()){qDebug()<<"loop.isRunning()!"; return;} // костыль: если цикл уже работает - выйти обратно
    timerPing->stop(); // остановить пинг

    baSendArray.clear();
    baSendCommand.clear();
    baRecvArray.clear();

    ui->statusBar->showMessage(tr("Проверка напряжения замкнутой цепи батареи ..."));
    Log(tr("Проверка напряжения замкнутой цепи батареи"), "blue");
    ui->labelClosedCircuitVoltageBattery0->setText("Идёт измерение...");

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

    starttime = QDateTime::currentDateTime(); // время начала измерения
    dt = QDateTime::currentDateTime(); // текущее время
    ui->widgetClosedCircuitBattery->graph(0)->clearData(); // очистить график

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
        ui->widgetClosedCircuitBattery->graph(0)->rescaleValueAxis(true); // для автоматического перерисовывания шкалы графика, если значения за пределами экрана
        ui->widgetClosedCircuitBattery->graph(0)->addData((double)x/1000, (double)fU);
        ui->widgetClosedCircuitBattery->replot();
    }

    if(bDeveloperState)
        Log("Цепь "+battery[iBatteryIndex].circuitbattery+" Receive "+qPrintable(baRecvArray)+" codeADC1=0x"+QString("%1").arg((ushort)codeADC, 0, 16), "blue");

    // проанализировать результаты
    if(codeADC >= codeLimit) // напряжение больше (норма)
    {
        Log("Напряжение цепи "+battery[iBatteryIndex].circuitbattery+" = "+QString::number(fU, 'f', 2)+" В.  Норма.", "blue");
        ui->labelClosedCircuitVoltageBattery0->setText("НЗЦб = "+QString::number(fU, 'f', 2)+" В.  Норма.");
        // если ручной режим, то выдать окно сообщения, и только потом разобрать режим измерения.
        if(bModeManual) QMessageBox::information(this, tr("Напряжение замкнутой цепи батареи"), tr("Напряжение цепи ")+battery[iBatteryIndex].circuitbattery+" = "+QString::number(fU, 'f', 2)+" В\nНорма");
    }
    else // напряжение меньше (не норма)
    {
        Log("Напряжение цепи "+battery[iBatteryIndex].circuitbattery+" = "+QString::number(fU, 'f', 2)+" В.  Не норма!.", "red");
        ui->labelClosedCircuitVoltageBattery0->setText("НЗЦб = "+QString::number(fU, 'f', 2)+" В.  Не норма!");
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
    qDebug() << "sender=" << ((QPushButton*)sender())->objectName() << "bState=" << bState;
    ui->tabWidget->addTab(ui->tabClosedCircuitVoltageBattery, ui->rbClosedCircuitVoltageBattery->text());
    ui->tabWidget->setCurrentIndex(ui->tabWidget->count()-1);
    Log(tr("Проверка начата - %1").arg(ui->rbClosedCircuitVoltageBattery->text()), "blue");

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
        ui->cbParamsAutoMode->setCurrentIndex(5); // переключаем режим комбокса на наш

    ui->groupBoxCOMPort->setDisabled(bState);
    ui->groupBoxDiagnosticDevice->setDisabled(bState);
    ui->groupBoxDiagnosticMode->setDisabled(bState);
    ui->cbParamsAutoMode->setDisabled(bState);
    ui->cbSubParamsAutoMode->setDisabled(bState);

    ui->progressBar->setMaximum(1);
    ui->progressBar->setValue(0);


    if (!bState) return;
    dArrayClosedCircuitVoltageBattery[0] = randMToN(29, 31); //число полученное с COM-порта

    str = tr("Напряжение цепи \"%0\" = <b>%1</b> В.").arg(battery[iBatteryIndex].circuitbattery).arg(dArrayClosedCircuitVoltageBattery[0]);
    if (dArrayClosedCircuitVoltageBattery[0] > settings.closecircuitbattery_limit) {
        str += " Не норма.";
        color = "red";
    } else
        color = "green";
    ui->labelClosedCircuitVoltageBattery0->setText(str);
    ui->labelClosedCircuitVoltageBattery0->setStyleSheet("QLabel { color : "+color+"; }");
    Log(str, color);
    ui->btnBuildReport->setEnabled(true);
    if (dArrayClosedCircuitVoltageBattery[0] > settings.closecircuitbattery_limit) {
        if (QMessageBox::question(this, "Внимание - "+ui->rbClosedCircuitVoltageBattery->text(), tr("%0 Продолжить?").arg(str), tr("Да"), tr("Нет"))) {
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

    Log(tr("Проверка завершена - %1").arg(ui->rbClosedCircuitVoltageBattery->text()), "blue");

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
