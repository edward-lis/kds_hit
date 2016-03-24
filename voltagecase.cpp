#include <QDebug>
#include <QMessageBox>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "battery.h"
#include "settings.h"

extern Settings settings;

extern QVector<Battery> battery;

void MainWindow::on_btnVoltageOnTheHousing_clicked()
{
    //checkVoltageOnTheHousing(); return;
    int ret=0; // код возврата ошибки
    quint16 codeU=0; // код напряжение минус, плюс
    // код порогового напряжения = пороговое напряжение В / коэфф. (вес разряда) + смещение (в коде)
    quint16 codeLimit=0;//settings.voltage_corpus_limit/settings.coefADC2 + settings.offsetADC2; // код, пороговое напряжение.
    quint16 offset=0; // смещение, в зависимости от измерения напряжения на корпусе + или -
    float voltageU=0;
    int i=0; // номер цепи
    //QLabel *label; // надпись в закладке
    //qDebug()<<"on_btnVoltageOnTheHousing_clicked";

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
    ui->tabWidget->addTab(ui->tabVoltageOnTheHousing, ui->rbVoltageOnTheHousing->text());
    ui->tabWidget->setCurrentIndex(ui->tabWidget->count()-1);
    Log(tr("Проверка начата - %1").arg(ui->rbVoltageOnTheHousing->text()), "blue");
    ui->statusBar->showMessage(tr("Проверка ")+ui->rbVoltageOnTheHousing->text()+" ...");
    //ui->progressBar->setMaximum(4); // установить кол-во ступеней прогресса
    ui->progressBar->setRange(0,0);
//    ui->progressBar->reset();

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
        ui->progressBar->setValue(ui->progressBar->value()+1);

        i=ui->cbVoltageOnTheHousing->currentIndex();
        iCurrentStep=i; // чтобы цикл for выполнился только раз в ручном.
        iMaxSteps=i+1;
    }
    else
    {
        ui->cbParamsAutoMode->setCurrentIndex(0); // переключаем режим комбокса на наш
        iCurrentStep = (ui->rbModeDiagnosticAuto->isChecked()) ? ui->cbSubParamsAutoMode->currentIndex() : ui->cbVoltageOnTheHousing->currentIndex();
        iMaxSteps = (ui->rbModeDiagnosticAuto->isChecked()) ? ui->cbSubParamsAutoMode->count() : ui->cbVoltageOnTheHousing->count();
    }
    for(i=iCurrentStep; i<iMaxSteps; i++)
    {
        // очистить буфера
        baSendArray.clear();
        baSendCommand.clear();
        baRecvArray.clear();

        /// формируем строку и пишем на label "идет измерение..."
        sLabelText = tr("%0) \"%1\"").arg(i+1).arg(battery[iBatteryIndex].str_voltage_corpus[i]);
        label = findChild<QLabel*>(tr("labelVoltageOnTheHousing%0").arg(i));
        label->setText(sLabelText + " идет измерение...");
        label->setStyleSheet("QLabel { color : blue; }");

        for(int j=0; j<settings.voltagecase_num; j++)
        {
            // сбросить коробочку
            baSendArray = (baSendCommand="IDLE")+"#"; // подготовить буфер для передачи
            timerSend->start(settings.delay_after_request_before_next_ADC2); //sendSerialData(); // послать baSendArray в порт
            // ждём ответа. по сигналу о готовности принятых данных или по таймауту, вывалимся из цикла
            ret=loop.exec();
            if(ret) goto stop; // если не ноль (ошибка таймаута) - вывалиться из режима. если 0, то приняли данные из порта
            //ui->progressBar->setValue(ui->progressBar->value()+1);

            if(i == 1) // если выбрана в комбобоксе такая цепь
            {
                baSendArray=(baSendCommand="UcaseM")+"#";
                offset = settings.offsetADC2_minus;
            }
            else
            {
                baSendArray=(baSendCommand="UcaseP")+"#";
                offset = settings.offsetADC2_plus;
            }
            codeLimit=settings.voltage_corpus_limit/settings.coefADC2 + offset; // код, пороговое напряжение.

            if(bDeveloperState) Log(QString("Sending ") + qPrintable(baSendArray), "blue");
            timerSend->start(settings.delay_after_IDLE_before_other); // послать baSendArray в порт через некоторое время
            ret=loop.exec();
            if(ret) goto stop;
            ui->progressBar->setValue(ui->progressBar->value()+1);

            baSendArray=baSendCommand+"?#";
            timerSend->start(settings.delay_after_start_before_request_voltagecase);
            ret=loop.exec();
            if(ret) goto stop;
            codeU = getRecvData(baRecvArray); // получить данные опроса
            ui->progressBar->setValue(ui->progressBar->value()+1);

            voltageU = ((codeU-offset)*settings.coefADC2); // напряжение в вольтах
            dArrayVoltageOnTheHousing[i] = voltageU;
            // если отладочный режим, напечатать отладочную инфу
            if(bDeveloperState)
            {
                Log(QString("k = ") + qPrintable(QString::number(settings.coefADC2)) + " код смещения offset =0x"+qPrintable(QString::number(offset, 16))+ " код АЦП2 = 0x" + qPrintable(QString::number(codeU, 16)) + " U=k*(code-offset) = " + QString::number(voltageU), "green");
            }
        }

        if (dArrayVoltageOnTheHousing[i] > settings.voltage_corpus_limit) {
            sResult = "Не норма!";
            color = "red";
        }
        else {
            sResult = "Норма";
            color = "green";
        }
        label->setText(tr("%0 = <b>%1</b> В. %2").arg(sLabelText).arg(dArrayVoltageOnTheHousing[i], 0, 'f', 2).arg(sResult));
        label->setStyleSheet("QLabel { color : "+color+"; }");
        Log(tr("%0 = <b>%1</b> В. %2").arg(sLabelText).arg(dArrayVoltageOnTheHousing[i], 0, 'f', 2).arg(sResult), color);

        ui->btnBuildReport->setEnabled(true); // разрешить кнопку отчёта

        /// заполняем массив проверок для отчета
        dateTime = QDateTime::currentDateTime();
        sArrayReportVoltageOnTheHousing.append(
                    tr("<tr>"\
                       "    <td>%0</td>"\
                       "    <td>%1</td>"\
                       "    <td>%2</td>"\
                       "    <td>%3</td>"\
                       "    <td>%4</td>"\
                       "</tr>")
                    .arg(dateTime.toString("hh:mm:ss"))
                    .arg(battery[iBatteryIndex].str_voltage_corpus[i])
                    .arg(dArrayVoltageOnTheHousing[i], 0, 'f', 2)
                    .arg(sResult)
                    .arg((ui->rbModeDiagnosticAuto->isChecked()) ? "Автоматический" : "Ручной"));

        if(codeU > codeLimit) // напряжение больше (в кодах)
        {
            if(!bModeManual)// если в автоматическом режиме
            {
                //if(!bModeManual && !bDeveloperState)QMessageBox::critical(this, "Не норма!", "Напряжение цепи "+battery[iBatteryIndex].str_voltage_corpus[ui->cbVoltageOnTheHousing->currentIndex()]+" = "+QString::number(voltageU)+" В больше нормы");// !!!
                if (QMessageBox::question(this, "Внимание - "+ui->rbVoltageOnTheHousing->text(), tr("%0 = %1 В. %2 Продолжить?").arg(sLabelText).arg(dArrayVoltageOnTheHousing[i], 0, 'f', 2).arg(sResult), tr("Да"), tr("Нет")))
                {
                    qDebug()<<"переход в ручной режим";
                    Log("Останов проверки - переход в ручной режим", "blue");
                    bState = false;
                    ui->groupBoxCOMPort->setDisabled(bState); // разрешить кнопку ком-порта???
                    ui->groupBoxDiagnosticMode->setDisabled(bState); // разрешить группу выбора режима диагностики
                    ui->cbParamsAutoMode->setDisabled(bState); // разрешить комбобокс пунктов автомата
                    ui->cbSubParamsAutoMode->setDisabled(bState); // разрешать комбобокс подпунктов автомата
                    ((QPushButton*)sender())->setText("Пуск");
                    // остановить текущую проверку, выход
                    bCheckInProgress = false;
                    emit ui->rbModeDiagnosticManual->setChecked(true);
                    break;
                }
            }
        }

        // если ручной режим, то выдать окно сообщения, и только потом разобрать режим измерения.
        if(bModeManual) QMessageBox::information(this, tr("Напряжение на корпусе"), tr("Напряжение цепи ")+battery[iBatteryIndex].str_voltage_corpus[ui->cbVoltageOnTheHousing->currentIndex()]+" = "+QString::number(voltageU, 'f', 2)+" В");
        if(!bModeManual) ui->cbSubParamsAutoMode->setCurrentIndex(ui->cbSubParamsAutoMode->currentIndex()+1);
    } // for
stop:
    if(ret == KDS_STOP) {
        label->setText(sLabelText + " измерение прервано!");
        label->setStyleSheet("QLabel { color : red; }");
        Log(sLabelText + " измерение прервано!", "red");
    }
    // сбросить коробочку
    baSendArray = (baSendCommand="IDLE")+"#";
    timerSend->start(settings.delay_after_request_before_next_ADC2);
    loop.exec();
    ui->progressBar->setValue(ui->progressBar->value()+1);

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
    ui->btnVoltageOnTheHousing->setText("Пуск");        // поменять текст на кнопке

    timerPing->start(delay_timerPing); // запустить пинг по выходу из режима
    baSendArray.clear();
    baSendCommand.clear();
    baRecvArray.clear();

    ui->progressBar->setMaximum(1); // stop progress
    ui->progressBar->reset();
}

/*
 * Напряжение на корпусе батареи
 */
/*void MainWindow::checkVoltageOnTheHousing()
{
    qDebug() << "sender=" << ((QPushButton*)sender())->objectName() << "bState=" << bState;
    ui->tabWidget->addTab(ui->tabVoltageOnTheHousing, ui->rbVoltageOnTheHousing->text());
    ui->tabWidget->setCurrentIndex(ui->tabWidget->count()-1);
    Log(tr("Проверка начата - %1").arg(ui->rbVoltageOnTheHousing->text()), "blue");

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
        ui->cbParamsAutoMode->setCurrentIndex(0); // переключаем режим комбокса на наш

    ui->groupBoxCOMPort->setDisabled(bState);
    ui->groupBoxDiagnosticDevice->setDisabled(bState);
    ui->groupBoxDiagnosticMode->setDisabled(bState);
    ui->cbParamsAutoMode->setDisabled(bState);
    ui->cbSubParamsAutoMode->setDisabled(bState);

    iCurrentStep = (ui->rbModeDiagnosticAuto->isChecked()) ? ui->cbSubParamsAutoMode->currentIndex() : ui->cbVoltageOnTheHousing->currentIndex();
    iMaxSteps = (ui->rbModeDiagnosticAuto->isChecked()) ? ui->cbSubParamsAutoMode->count() : ui->cbVoltageOnTheHousing->count();
    ui->progressBar->setMaximum(iMaxSteps);
    ui->progressBar->setValue(iCurrentStep);

    switch (iBatteryIndex) {
    case 0: //9ER20P-20
        for (int i = iCurrentStep; i < iMaxSteps; i++) {
            if (!bState) return;
            switch (i) {
            case 0:
                delay(1000);
                dArrayVoltageOnTheHousing[i] = randMToN(0, 2); //число полученное с COM-порта
                break;
            case 1:
                delay(1000);
                dArrayVoltageOnTheHousing[i] = randMToN(0, 2); //число полученное с COM-порта
                break;
            default:
                return;
                break;
            }

            if(ui->rbModeDiagnosticManual->isChecked())
                ui->cbVoltageOnTheHousing->setCurrentIndex(i);
            else
                ui->cbSubParamsAutoMode->setCurrentIndex(i);

            str = tr("Напряжение цепи \"%0\" = <b>%1</b> В.").arg(battery[iBatteryIndex].str_voltage_corpus[i]).arg(dArrayVoltageOnTheHousing[i]);
            QLabel * label = findChild<QLabel*>(tr("labelVoltageOnTheHousing%0").arg(i));
            if (dArrayVoltageOnTheHousing[i] > settings.voltage_corpus_limit) {
                str += " Не норма.";
                color = "red";
            } else
                color = "green";
            label->setText(str);
            label->setStyleSheet("QLabel { color : "+color+"; }");
            Log(str, color);
            ui->btnBuildReport->setEnabled(true);
            if (dArrayVoltageOnTheHousing[i] > settings.voltage_corpus_limit) {
                if (QMessageBox::question(this, "Внимание - "+ui->rbVoltageOnTheHousing->text(), tr("%0 Продолжить?").arg(str), tr("Да"), tr("Нет"))) {
                    bState = false;
                    ui->groupBoxCOMPort->setDisabled(bState);
                    ui->groupBoxDiagnosticMode->setDisabled(bState);
                    ui->cbParamsAutoMode->setDisabled(bState);
                    ui->cbSubParamsAutoMode->setDisabled(bState);
                    ((QPushButton*)sender())->setText("Пуск");
                    return;
                }
            }

            ui->progressBar->setValue(i+1);
        }
        break;
    case 1:
        Log("Действия проверки.", "green");
        delay(1000);
        break;
    case 2:
        Log("Действия проверки.", "green");
        delay(1000);
        break;
    case 3:
        Log("Действия проверки.", "green");
        delay(1000);
        break;
    default:
        break;
    }

    Log(tr("Проверка завершена - %1").arg(ui->rbVoltageOnTheHousing->text()), "blue");

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
