#include <QDebug>
#include <QMessageBox>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "battery.h"
#include "settings.h"

extern Settings settings;

extern QVector<Battery> battery;

// Нажата кнопка проверки напряжения разомкнутых цепей групп
void MainWindow::on_btnOpenCircuitVoltageGroup_clicked()
{
    //checkOpenCircuitVoltageGroup(); return;
    quint16 codeADC=0; // принятый код АЦП
    float fU=0; // принятое напряжение в вольтах
    // код порогового напряжения = пороговое напряжение В / коэфф. (вес разряда) + смещение (в коде)
    quint16 codeLimit=settings.opencircuitgroup_limit_min/settings.coefADC1 + settings.offsetADC1; // код, пороговое напряжение.
    int ret=0; // код возврата ошибки
    QString str_num; // номер цепи
    int i=0; // номер цепи
    //QLabel *label; // надпись в закладке

    /// по началу проверки очистим все label'ы и полученные результаты
    /*for (int i = 0; i < 28; i++) {
        dArrayOpenCircuitVoltageGroup[i] = -1;
        label = findChild<QLabel*>(tr("labelOpenCircuitVoltageGroup%0").arg(i));
        label->setStyleSheet("QLabel { color : black; }");
        label->clear();
        if (i < battery[iBatteryIndex].group_num)
            label->setText(tr("%0) \"%1\" не измерялось.").arg(i+1).arg(battery[iBatteryIndex].circuitgroup[i]));
    }*/

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
    ui->tabWidget->addTab(ui->tabOpenCircuitVoltageGroup, ui->rbOpenCircuitVoltageGroup->text());
    ui->tabWidget->setCurrentIndex(ui->tabWidget->count()-1);
    Log(tr("Проверка начата - %1").arg(ui->rbOpenCircuitVoltageGroup->text()), "blue");
    ui->statusBar->showMessage(tr("Проверка ")+ui->rbOpenCircuitVoltageGroup->text()+" ...");

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

        //i=ui->cbOpenCircuitVoltageGroup->currentIndex();
        iCurrentStep=0; // в ручном начнём сначала
        iMaxSteps=modelOpenCircuitVoltageGroup->rowCount()-1; // -1 с учётом первой строки в комбобоксе
    }
    else
    {
        ui->cbParamsAutoMode->setCurrentIndex(2); // переключаем режим комбокса на наш
        iCurrentStep = ui->cbSubParamsAutoMode->currentIndex();
        iMaxSteps = ui->cbSubParamsAutoMode->count();
    }

    // Пробежимся по списку цепей
    for(i=iCurrentStep; i < iMaxSteps; i++)
    {
        if(bModeManual) // в ручном будем идти по чекбоксам
        {
            QStandardItem *sitm = modelOpenCircuitVoltageGroup->item(i+1, 0); // взять очередной номер
            Qt::CheckState checkState = sitm->checkState(); // и его состояние
            if (checkState != Qt::Checked) continue; // если не отмечено, то следующий.
        }

        ui->progressBar->setMaximum(2); // установить кол-во ступеней прогресса
        ui->progressBar->reset();

        // очистить массивы посылки/приёма
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
        sLabelText = tr("%0) \"%1\"").arg(i+1).arg(battery[iBatteryIndex].circuitgroup[i]);
        label = findChild<QLabel*>(tr("labelOpenCircuitVoltageGroup%0").arg(i));
        label->setText(sLabelText + " идет измерение...");
        label->setStyleSheet("QLabel { color : blue; }");

        // собрать режим
        str_num.sprintf(" %02i", i+1); // напечатать номер цепи. т.к. счётчик от нуля, поэтому +1
        baSendArray=(baSendCommand="UocG")+str_num.toLocal8Bit()+"#";
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
        dArrayOpenCircuitVoltageGroup[i] = fU;

        battery[iBatteryIndex].b_flag_circuit[i] |= CIRCUIT_OCG_TESTED; // установить флаг - цепь проверялась
        if(bDeveloperState)
            Log("Цепь "+battery[iBatteryIndex].circuitgroup[i]+" Receive "+qPrintable(baRecvArray)+" codeADC1=0x"+QString("%1").arg((ushort)codeADC, 0, 16), "blue");

        // напечатать рез-т в закладку и в журнал
        if (dArrayOpenCircuitVoltageGroup[i] < settings.opencircuitgroup_limit_min){
            sResult = "Не норма!";
            color = "red";
        }
        else {
            sResult = "Норма";
            color = "green";
        }
        label->setText(tr("%0 = <b>%1</b> В. %2").arg(sLabelText).arg(dArrayOpenCircuitVoltageGroup[i], 0, 'f', 2).arg(sResult));
        label->setStyleSheet("QLabel { color : "+color+"; }");
        Log(tr("%0 = <b>%1</b> В. %2").arg(sLabelText).arg(dArrayOpenCircuitVoltageGroup[i], 0, 'f', 2).arg(sResult), color);

        ui->btnBuildReport->setEnabled(true);

        /// заполняем массив проверок для отчета
        dateTime = QDateTime::currentDateTime();
        sArrayReportOpenCircuitVoltageGroup.append(
                    tr("<tr>"\
                       "    <td>%0</td>"\
                       "    <td>%1</td>"\
                       "    <td>%2</td>"\
                       "    <td>%3</td>"\
                       "</tr>")
                    .arg(dateTime.toString("hh:mm:ss"))
                    .arg(battery[iBatteryIndex].circuitgroup[i])
                    .arg(dArrayOpenCircuitVoltageGroup[i], 0, 'f', 2)
                    .arg(sResult));

        // проанализировать результаты
        if(codeADC >= codeLimit) // напряжение больше (норма)
        {
            //Log("Напряжение цепи "+battery[iBatteryIndex].circuitgroup[i]+" = "+QString::number(fU, 'f', 2)+" В.  Норма.", "blue");
            // если ручной режим, то выдать окно сообщения, и только потом разобрать режим измерения.
            if(bModeManual) QMessageBox::information(this, tr("Напряжение разомкнутой цепи группы"), tr("Напряжение цепи ")+battery[iBatteryIndex].circuitgroup[i]+" = "+QString::number(fU, 'f', 2)+" В\nНорма");
            // добавить цепь в список исправных
            battery[iBatteryIndex].b_flag_circuit[i] &= ~CIRCUIT_FAULT; // снять флаг - цепь неисправна
        }
        else // напряжение меньше (не норма)
        {
            //Log("Напряжение цепи "+battery[iBatteryIndex].circuitgroup[i]+" = "+QString::number(fU, 'f', 2)+" В.  Не норма! Проверка группы под нагрузкой запрещена.", "red");
            // если ручной режим, то выдать окно сообщения, и только потом разобрать режим измерения.
            if(bModeManual) QMessageBox::information(this, tr("Напряжение разомкнутой цепи группы"), tr("Напряжение цепи ")+battery[iBatteryIndex].circuitgroup[i]+" = "+QString::number(fU, 'f', 2)+" В\nНе норма!");
            // установить флаг - цепь неисправна, запрет проверки цепи под нагрузкой
            battery[iBatteryIndex].b_flag_circuit[i] |= CIRCUIT_FAULT;
        }

        // при напряжении меньше нормы в автоматическом режиме проверка продолжается
        /*if (dArrayOpenCircuitVoltageGroup[i] < settings.opencircuitgroup_limit_min) {
            if(!bModeManual)// если в автоматическом режиме
            {
                if (QMessageBox::question(this, "Внимание - "+ui->rbOpenCircuitVoltageGroup->text(), tr("%0 Продолжить?").arg(str), tr("Да"), tr("Нет"))) {
                    bState = false;
                    ui->groupBoxCOMPort->setDisabled(bState);
                    ui->groupBoxDiagnosticMode->setDisabled(bState);
                    ui->cbParamsAutoMode->setDisabled(bState);
                    ui->cbSubParamsAutoMode->setDisabled(bState);
                    ((QPushButton*)sender())->setText("Пуск");
                    // остановить текущую проверку, выход
                    bCheckInProgress = false;
                    ui->rbModeDiagnosticManual->setChecked(true);
                    break;
                }
            }
        }*/

        // флаги цепи qDebug()<<"battery[iBatteryIndex].b_flag_circuit[i-1]"<<battery[iBatteryIndex].b_flag_circuit[i];

    }//for

stop:
    if(ret == KDS_STOP) {
        label->setText(sLabelText + " измерение прервано!");
        label->setStyleSheet("QLabel { color : red; }");
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
    ui->btnOpenCircuitVoltageGroup->setText("Пуск");          // поменять текст на кнопке

    timerPing->start(delay_timerPing); // запустить пинг по выходу из режима
    baSendArray.clear(); // очистить буфера команд.
    baSendCommand.clear();
    baRecvArray.clear();
    ui->progressBar->reset();

    // оформить комбобокс НЗЦг в соответствии с полученными данными, запретить выбор просевших групп
    for (int r = 0; r < battery[iBatteryIndex].group_num; r++)
    {
        item = new QStandardItem(QString("%0").arg(battery[iBatteryIndex].circuitgroup[r]));
        if(!(battery[iBatteryIndex].b_flag_circuit[r] & CIRCUIT_FAULT))
        {
            item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
            item->setData(Qt::Checked, Qt::CheckStateRole);
        }
        else
        {
            item->setFlags(Qt::NoItemFlags);
            item->setData(Qt::Unchecked, Qt::CheckStateRole);
        }
        modelClosedCircuitVoltageGroup->setItem(r+1, 0, item);
    }
    ui->cbClosedCircuitVoltageGroup->setModel(modelClosedCircuitVoltageGroup);
}

// слот вызывается при изменении чекбоксов элементов списка комбобокса
void MainWindow::itemChangedOpenCircuitVoltageGroup(QStandardItem* itm)
{
    itm->text(); /// чтобы небыло варнинга при компиляции на неиспользование itm
    int count = 0;
    for(int i=1; i < modelOpenCircuitVoltageGroup->rowCount(); i++)
    {
        QStandardItem *sitm = modelOpenCircuitVoltageGroup->item(i, 0);
        Qt::CheckState checkState = sitm->checkState();
        if (checkState == Qt::Checked)
            count++;
    }
    ui->cbOpenCircuitVoltageGroup->setItemText(0, tr("Выбрано: %0 из %1").arg(count).arg(modelOpenCircuitVoltageGroup->rowCount()-1));
    ui->cbOpenCircuitVoltageGroup->setCurrentIndex(0);
}

/*
 * Напряжение разомкнутой цепи группы
 */
/*void MainWindow::checkOpenCircuitVoltageGroup()
{
    qDebug() << "sender=" << ((QPushButton*)sender())->objectName() << "bState=" << bState;
    ui->tabWidget->addTab(ui->tabOpenCircuitVoltageGroup, ui->rbOpenCircuitVoltageGroup->text());
    ui->tabWidget->setCurrentIndex(ui->tabWidget->count()-1);
    Log(tr("Проверка начата - %1").arg(ui->rbOpenCircuitVoltageGroup->text()), "blue");

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
        ui->cbParamsAutoMode->setCurrentIndex(2); // переключаем режим комбокса на наш

    ui->groupBoxCOMPort->setDisabled(bState);
    ui->groupBoxDiagnosticDevice->setDisabled(bState);
    ui->groupBoxDiagnosticMode->setDisabled(bState);
    ui->cbParamsAutoMode->setDisabled(bState);
    ui->cbSubParamsAutoMode->setDisabled(bState);

    iCurrentStep = (ui->rbModeDiagnosticAuto->isChecked()) ? ui->cbSubParamsAutoMode->currentIndex() : 0;
    iMaxSteps = (ui->rbModeDiagnosticAuto->isChecked()) ? ui->cbSubParamsAutoMode->count() : ui->cbOpenCircuitVoltageGroup->count();

    if (ui->rbModeDiagnosticManual->isChecked()) { /// для ручного режима свой максимум для прогресс бара
        int count = 0;
        for(int i = 0; i < iMaxSteps; i++) {
            QModelIndex index = ui->cbOpenCircuitVoltageGroup->model()->index(i+1, 0);
            if(index.data(Qt::CheckStateRole) == 2) /// проходимся только по выбранным
                count++;
        }
        ui->progressBar->setMaximum(count);
    } else {
        ui->progressBar->setMaximum(iMaxSteps);
    }
    ui->progressBar->setValue(iCurrentStep);

    switch (iBatteryIndex) {
    case 0: //9ER20P-20
        for (int i = iCurrentStep; i < iMaxSteps; i++) {
            if (!bState) return; /// если прожали Стоп выходим из цикла
            QModelIndex index = ui->cbOpenCircuitVoltageGroup->model()->index(i+1, 0);
            if(index.data(Qt::CheckStateRole) == 2) { /// проходимся только по выбранным
                switch (i) {
                case 0:
                    delay(1000);
                    dArrayOpenCircuitVoltageGroup[i] = randMToN(26, 28); //число полученное с COM-порта
                    break;
                case 1:
                    delay(1000);
                    dArrayOpenCircuitVoltageGroup[i] = randMToN(26, 28); //число полученное с COM-порта
                    break;
                case 2:
                    delay(1000);
                    dArrayOpenCircuitVoltageGroup[i] = randMToN(26, 28); //число полученное с COM-порта
                    break;
                case 3:
                    delay(1000);
                    dArrayOpenCircuitVoltageGroup[i] = randMToN(26, 28); //число полученное с COM-порта
                    break;
                case 4:
                    delay(1000);
                    dArrayOpenCircuitVoltageGroup[i] = randMToN(26, 28); //число полученное с COM-порта
                    break;
                case 5:
                    delay(1000);
                    dArrayOpenCircuitVoltageGroup[i] = randMToN(26, 28); //число полученное с COM-порта
                    break;
                case 6:
                    delay(1000);
                    dArrayOpenCircuitVoltageGroup[i] = randMToN(26, 28); //число полученное с COM-порта
                    break;
                case 7:
                    delay(1000);
                    dArrayOpenCircuitVoltageGroup[i] = randMToN(26, 28); //число полученное с COM-порта
                    break;
                case 8:
                    delay(1000);
                    dArrayOpenCircuitVoltageGroup[i] = randMToN(26, 28); //число полученное с COM-порта
                    break;
                case 9:
                    delay(1000);
                    dArrayOpenCircuitVoltageGroup[i] = randMToN(26, 28); //число полученное с COM-порта
                    break;
                case 10:
                    delay(1000);
                    dArrayOpenCircuitVoltageGroup[i] = randMToN(26, 28); //число полученное с COM-порта
                    break;
                case 11:
                    delay(1000);
                    dArrayOpenCircuitVoltageGroup[i] = randMToN(26, 28); //число полученное с COM-порта
                    break;
                case 12:
                    delay(1000);
                    dArrayOpenCircuitVoltageGroup[i] = randMToN(26, 28); //число полученное с COM-порта
                    break;
                case 13:
                    delay(1000);
                    dArrayOpenCircuitVoltageGroup[i] = randMToN(26, 28); //число полученное с COM-порта
                    break;
                case 14:
                    delay(1000);
                    dArrayOpenCircuitVoltageGroup[i] = randMToN(26, 28); //число полученное с COM-порта
                    break;
                case 15:
                    delay(1000);
                    dArrayOpenCircuitVoltageGroup[i] = randMToN(26, 28); //число полученное с COM-порта
                    break;
                case 16:
                    delay(1000);
                    dArrayOpenCircuitVoltageGroup[i] = randMToN(26, 28); //число полученное с COM-порта
                    break;
                case 17:
                    delay(1000);
                    dArrayOpenCircuitVoltageGroup[i] = randMToN(26, 28); //число полученное с COM-порта
                    break;
                case 18:
                    delay(1000);
                    dArrayOpenCircuitVoltageGroup[i] = randMToN(26, 28); //число полученное с COM-порта
                    break;
                case 19:
                    delay(1000);
                    dArrayOpenCircuitVoltageGroup[i] = randMToN(26, 28); //число полученное с COM-порта
                    break;
                default:
                    return;
                    break;
                }
                qDebug() << "dArrayOpenCircuitVoltageGroup[" << i << "]=" << dArrayOpenCircuitVoltageGroup[i];
                if(ui->rbModeDiagnosticAuto->isChecked())
                    ui->cbSubParamsAutoMode->setCurrentIndex(i);

                str = tr("Напряжение цепи \"%0\" = <b>%1</b> В.").arg(battery[iBatteryIndex].circuitgroup[i]).arg(dArrayOpenCircuitVoltageGroup[i]);
                QLabel * label = findChild<QLabel*>(tr("labelOpenCircuitVoltageGroup%0").arg(i));
                if (dArrayOpenCircuitVoltageGroup[i] > settings.closecircuitgroup_limit) {
                    str += " Не норма.";
                    color = "red";
                } else
                    color = "green";

                label->setText(str);
                label->setStyleSheet("QLabel { color : "+color+"; }");
                Log(str, color);
                ui->btnBuildReport->setEnabled(true);
                if (dArrayOpenCircuitVoltageGroup[i] > settings.closecircuitgroup_limit) {
                    if (QMessageBox::question(this, "Внимание - "+ui->rbOpenCircuitVoltageGroup->text(), tr("%0 Продолжить?").arg(str), tr("Да"), tr("Нет"))) {
                        bState = false;
                        ui->groupBoxCOMPort->setDisabled(bState);
                        ui->groupBoxDiagnosticMode->setDisabled(bState);
                        ui->cbParamsAutoMode->setDisabled(bState);
                        ui->cbSubParamsAutoMode->setDisabled(bState);
                        ((QPushButton*)sender())->setText("Пуск");
                        return;
                    }
                }
                ui->progressBar->setValue(ui->progressBar->value()+1);
            }
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

    Log(tr("Проверка завершена - %1").arg(ui->rbOpenCircuitVoltageGroup->text()), "blue");

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
