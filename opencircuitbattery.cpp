#include <QDebug>
#include <QMessageBox>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "battery.h"
#include "settings.h"

extern Settings settings;

extern QVector<Battery> battery;


// Нажата кнопка проверки напряжения разомкнутой цепи батареи
void MainWindow::on_btnOpenCircuitVoltageBattery_clicked()
{
    quint16 codeADC=0; // принятый код АЦП
    float fU=0; // принятое напряжение в вольтах
    // код порогового напряжения = пороговое напряжение В / коэфф. (вес разряда) + смещение (в коде)
    quint16 codeLimit=settings.opencircuitbattery_limit/settings.coefADC1 + settings.offsetADC1; // код, пороговое напряжение.
    int ret=0; // код возврата ошибки
    int i=0; // номер цепи
    //QLabel *label; // надпись в закладке

    if(bCheckInProgress) // если зашли в эту ф-ию по нажатию кнопки btnVoltageOnTheHousing ("Стоп"), будучи уже в состоянии проверки, значит стоп режима
    {
        // остановить текущую проверку, выход
        bCheckInProgress = false;
        timerSend->stop(); // остановить посылку очередной команды в порт
        timeoutResponse->stop(); // остановить предыдущий таймаут (если был, конечно)
        qDebug()<<"loop.isRunning()"<<loop.isRunning();
        if(loop.isRunning())
        {
            loop.exit(KDS_STOP); // прекратить цикл ожидания посылки/ожидания ответа от коробочки
        }
        return;
    }

    if(loop.isRunning()){qDebug()<<"loop.isRunning()!"; return;} // костыль: если цикл уже работает - выйти обратно
    timerPing->stop(); // остановить пинг
    bCheckInProgress = true; // вошли в состояние проверки

    // запретим виджеты, чтоб не нажимались
    ui->groupBoxCOMPort->setDisabled(true);
    ui->groupBoxDiagnosticDevice->setDisabled(true);
    ui->groupBoxDiagnosticMode->setDisabled(true);
    ui->cbParamsAutoMode->setDisabled(true);
    ui->cbSubParamsAutoMode->setDisabled(true);

    // откроем вкладку
    ui->tabWidget->addTab(ui->tabOpenCircuitVoltageBattery, ui->rbOpenCircuitVoltageBattery->text());
    ui->tabWidget->setCurrentIndex(ui->tabWidget->count()-1);
    Log(tr("Проверка начата - %1").arg(ui->rbOpenCircuitVoltageBattery->text()), "blue");
    ui->statusBar->showMessage(tr("Проверка ")+ui->rbOpenCircuitVoltageBattery->text()+" ...");

    if(bModeManual)// если в ручном режиме
    {
        // переименовать кнопку
        if(!bState) {
            bState = true;
            ui->groupBoxCheckParams->setEnabled(bState);
            ((QPushButton*)sender())->setText("Стоп");
        } else {
            bState = false;
            ((QPushButton*)sender())->setText("Пуск");
        }

        i=ui->cbOpenCircuitVoltageBattery->currentIndex();
        iCurrentStep=i; // чтобы цикл for выполнился только раз в ручном.
        iMaxSteps=i+1;
    }
    else
    {
        ui->cbParamsAutoMode->setCurrentIndex(3); // переключаем режим комбокса на наш
        iCurrentStep = ui->cbSubParamsAutoMode->currentIndex();
        iMaxSteps = ui->cbSubParamsAutoMode->count();
    }


    ui->progressBar->setMaximum(2); // установить кол-во ступеней прогресса
    ui->progressBar->reset();

    baSendArray.clear();
    baSendCommand.clear();
    baRecvArray.clear();

    // сбросить коробочку
    baSendArray = (baSendCommand="IDLE")+"#"; // подготовить буфер для передачи
    timerSend->start(settings.delay_after_request_before_next_ADC1); // послать baSendArray в порт
    // ждём ответа. по сигналу о готовности принятых данных или по таймауту, вывалимся из цикла
    ret=loop.exec();
    if(ret) goto stop; // если не ноль (ошибка таймаута) - вывалиться из режима. если 0, то приняли данные из порта
    ui->progressBar->setValue(ui->progressBar->value()+1);

    /// формируем строку и пишем на label "идет измерение..."
    sLabelText = tr("1) \"%0\"").arg(battery[iBatteryIndex].circuitbattery);
    ui->labelOpenCircuitVoltageBattery0->setText(sLabelText + " идет измерение...");
    ui->labelOpenCircuitVoltageBattery0->setStyleSheet("QLabel { color : blue; }");

    // собрать режим
    baSendArray=(baSendCommand="UocB")+"#";
    if(bDeveloperState) Log(QString("Sending ") + qPrintable(baSendArray), "blue");
    timerSend->start(settings.delay_after_IDLE_before_other);
    ret=loop.exec();
    if(ret) goto stop;
    ui->progressBar->setValue(ui->progressBar->value()+1);

    // опросить
    baSendArray=baSendCommand+"?#";
    timerSend->start(settings.delay_after_start_before_request_ADC1);
    ret=loop.exec();
    if(ret) goto stop;
    ui->progressBar->setValue(ui->progressBar->value()+1);
    codeADC = getRecvData(baRecvArray);

    fU = ((codeADC-settings.offsetADC1)*settings.coefADC1); // напряжение в вольтах
    dArrayOpenCircuitVoltageBattery[0] = fU;

    if(bDeveloperState)
        Log("Цепь "+battery[iBatteryIndex].circuitbattery+" Receive "+qPrintable(baRecvArray)+" codeADC1=0x"+QString("%1").arg((ushort)codeADC, 0, 16), "blue");

    if (dArrayOpenCircuitVoltageBattery[0] < settings.opencircuitbattery_limit) {
        sResult = "Не норма!";
        color = "red";
    }
    else {
        sResult = "Норма";
        color = "green";
    }
    ui->labelOpenCircuitVoltageBattery0->setText(tr("%0 = <b>%1</b> В. %2").arg(sLabelText).arg(dArrayOpenCircuitVoltageBattery[0], 0, 'f', 2).arg(sResult));
    ui->labelOpenCircuitVoltageBattery0->setStyleSheet("QLabel { color : "+color+"; }");
    Log(tr("%0 = <b>%1</b> В. %2").arg(sLabelText).arg(dArrayOpenCircuitVoltageBattery[0], 0, 'f', 2).arg(sResult), color);

    ui->btnBuildReport->setEnabled(true);

    /// заполняем массив проверок для отчета
    dateTime = QDateTime::currentDateTime();
    sArrayReportOpenCircuitVoltageBattery.append(
                tr("<tr>"\
                   "    <td>%0</td>"\
                   "    <td>%1</td>"\
                   "    <td>%2</td>"\
                   "    <td>%3</td>"\
                   "</tr>")
                .arg(dateTime.toString("hh:mm:ss"))
                .arg(battery[iBatteryIndex].circuitbattery)
                .arg(dArrayOpenCircuitVoltageBattery[0], 0, 'f', 2)
                .arg(sResult));

    // проанализировать результаты
    if(codeADC >= codeLimit) // напряжение больше (норма)
    {
        //Log("Напряжение цепи "+battery[iBatteryIndex].circuitbattery+" = "+QString::number(fU, 'f', 2)+" В.  Норма.", "blue");
        // если ручной режим, то выдать окно сообщения, и только потом разобрать режим измерения.
        if(bModeManual) QMessageBox::information(this, tr("Напряжение разомкнутой цепи батареи"), tr("Напряжение цепи ")+battery[iBatteryIndex].circuitbattery+" = "+QString::number(fU, 'f', 2)+" В\nНорма");
    }
    else // напряжение меньше (не норма)
    {
        //Log("Напряжение цепи "+battery[iBatteryIndex].circuitbattery+" = "+QString::number(fU, 'f', 2)+" В.  Не норма!.", "red");
        // если ручной режим, то выдать окно сообщения, и только потом разобрать режим измерения.
        if(bModeManual) QMessageBox::information(this, tr("Напряжение разомкнутой цепи батареи"), tr("Напряжение цепи ")+battery[iBatteryIndex].circuitbattery+" = "+QString::number(fU, 'f', 2)+" В\nНе норма!");
        else
        {
            if (QMessageBox::question(this, "Внимание - "+ui->rbOpenCircuitVoltageBattery->text(), tr("%0 Продолжить?").arg(str), tr("Да"), tr("Нет"))) {
                bState = false;
                ui->groupBoxCOMPort->setDisabled(bState);
                ui->groupBoxDiagnosticMode->setDisabled(bState);
                ui->cbParamsAutoMode->setDisabled(bState);
                ui->cbSubParamsAutoMode->setDisabled(bState);
                ((QPushButton*)sender())->setText("Пуск");
                // остановить текущую проверку, выход
                bCheckInProgress = false;
                ui->rbModeDiagnosticManual->setChecked(true);
                //break;
            }
        }
    }
stop:
    if(ret == KDS_STOP) {
        ui->labelOpenCircuitVoltageBattery0->setText(sLabelText + " измерение прервано!");
        ui->labelOpenCircuitVoltageBattery0->setStyleSheet("QLabel { color : red; }");
        Log(sLabelText + " измерение прервано!", "red");
    }
    // сбросить коробочку
    baSendArray = (baSendCommand="IDLE")+"#";
    timerSend->start(settings.delay_after_request_before_next_ADC1);
    ret=loop.exec();

    bCheckInProgress = false; // вышли из состояния проверки

    // если отладочный режим, напечатать отладочную инфу
    if(bDeveloperState)
    {
        if(ret == KDS_TIMEOUT) Log(tr("Timeout!"), "red");
        else if(ret == KDS_INCORRECT_REPLY) Log(tr("Incorrect reply!"), "red");
        else if(ret == KDS_STOP) Log(tr("Stop checking!"), "red");
    }

    if(bModeManual)
    {
        bState = false;
    }

    ui->groupBoxCOMPort->setEnabled(true);              // кнопка последовательного порта
    ui->groupBoxDiagnosticDevice->setEnabled(true);     // открыть группу выбора батареи
    ui->groupBoxDiagnosticMode->setEnabled(true);       // окрыть группу выбора режима
    ui->cbParamsAutoMode->setEnabled(true);             // открыть комбобокс выбора пункта начала автоматического режима
    ui->cbSubParamsAutoMode->setEnabled(true);          // открыть комбобокс выбора подпункта начала автоматического режима
    ui->btnOpenCircuitVoltageBattery->setText("Пуск");  // поменять текст на кнопке

    timerPing->start(delay_timerPing); // запустить пинг по выходу из режима
    baSendArray.clear(); // очистить буфера команд.
    baSendCommand.clear();
    baRecvArray.clear();
    ui->progressBar->reset();
}

/*
 * Напряжение разомкнутой цепи батареи
 */
/*void MainWindow::checkOpenCircuitVoltageBattery()
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
}*/
