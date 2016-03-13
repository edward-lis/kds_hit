#include <QDebug>
#include <QMessageBox>
#include <QDateTime>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "battery.h"
#include "settings.h"

extern Settings settings;

extern QVector<Battery> battery;

// Нажата кнопка проверки напряжения замкнутых цепей групп
void MainWindow::on_btnClosedCircuitVoltageGroup_clicked()
{
    //checkClosedCircuitVoltageGroup(); return;
    //qDebug()<<"checkClosedCircuitVoltageGroup";
    quint16 codeADC=0; // принятый код АЦП
    float fU=0; // принятое напряжение в вольтах
    // код порогового напряжения = пороговое напряжение В / коэфф. (вес разряда) + смещение (в коде)
    quint16 codeLimit=settings.closecircuitgroup_limit/settings.coefADC1 + settings.offsetADC1; // код, пороговое напряжение.
    int ret=0; // код возврата ошибки
    QString str_num; // номер цепи
    QDateTime starttime; // время начала измерения
    QDateTime dt; // текущее время очередного измерения
    double x; // текущая координата Х
    int cycleTimeSec=settings.time_depassivation[2]; // длительность цикла проверки в секундах
    bool bFirstPoll=true; // первое измерение
    //int i=0; // номер цепи
    //QLabel *label; // надпись в закладке

    if(bCheckInProgress) // если зашли в эту ф-ию по нажатию кнопки ("Стоп"), будучи уже в состоянии проверки, значит стоп режима
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
    } else {
        /// по началу проверки очистим все label'ы и полученные результаты
        for (int i = 0; i < 28; i++) {
            dArrayClosedCircuitVoltageGroup[i] = -1;
            label = findChild<QLabel*>(tr("labelClosedCircuitVoltageGroup%0").arg(i));
            label->setStyleSheet("QLabel { color : black; }");
            label->clear();
            if (i < battery[iBatteryIndex].group_num)
                label->setText(tr("%0) \"%1\" не измерялось.").arg(i+1).arg(battery[iBatteryIndex].circuitgroup[i]));
        }
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
    ui->tabWidget->addTab(ui->tabClosedCircuitVoltageGroup, ui->rbClosedCircuitVoltageGroup->text());
    ui->tabWidget->setCurrentIndex(ui->tabWidget->count()-1);
    Log(tr("Проверка начата - %1").arg(ui->rbClosedCircuitVoltageGroup->text()), "blue");
    ui->statusBar->showMessage(tr("Проверка ")+ui->rbClosedCircuitVoltageGroup->text()+" ...");

    if(bModeManual)// если в ручном режиме
    {
        // переименовать кнопку
        if(!bState) {
            bState = true;
            ((QPushButton*)sender())->setText("Стоп");
        } else {
            bState = false;
            ((QPushButton*)sender())->setText("Пуск");
        }

        //i=ui->cbOpenCircuitVoltageGroup->currentIndex();
        iCurrentStep=0; // в ручном начнём сначала
        iMaxSteps=modelClosedCircuitVoltageGroup->rowCount()-1; // -1 с учётом первой строки в комбобоксе
    }
    else
    {
        ui->cbParamsAutoMode->setCurrentIndex(4); // переключаем режим комбокса на наш
        iCurrentStep = ui->cbSubParamsAutoMode->currentIndex();
        iMaxSteps = ui->cbSubParamsAutoMode->count();
    }

    // написать про группы, в зависимости от признаков и флагов.
    for(int i=0; i<battery[iBatteryIndex].group_num; i++)
    {
        label = findChild<QLabel*>(tr("labelClosedCircuitVoltageGroup%1").arg(i));
        if(!(battery[iBatteryIndex].b_flag_circuit[i] & CIRCUIT_OCG_TESTED))
        {
            label->setText(tr("%0) НРЦг не проверялось.").arg(i+1));
        }
        else if(battery[iBatteryIndex].b_flag_circuit[i] & CIRCUIT_FAULT)
        {
            label->setText(tr("%0) НРЦг <нормы, проверка под нагрузкой запрещена.").arg(i+1));
        }
        else
        {
            label->setText(tr("%0)").arg(i+1));
        }
    }
    // если все цепи меньше нормы, или не проверялись - в автомате батарею под нагрузкой не проверять
    bool bAllCircuitsFail=true;
    for(int i=0; i<battery[iBatteryIndex].group_num; i++)
    {
        if(!((!(battery[iBatteryIndex].b_flag_circuit[i] & CIRCUIT_OCG_TESTED))
                || (battery[iBatteryIndex].b_flag_circuit[i] & CIRCUIT_FAULT)))
        {
            bAllCircuitsFail=false;
        }
    }
    if(bAllCircuitsFail)
    {
        QMessageBox::information(this, "Внимание!", "Все цепи меньше нормы или не проверялись под нагрузкой.\nПроверка цепей под нагрузкой запрещена.");
        bState = false;
        goto stop;
    }

    // Пробежимся по списку цепей
    for(int i=iCurrentStep; i < iMaxSteps; i++)
    {
        if(bModeManual) // в ручном будем идти по чекбоксам
        {
            QStandardItem *sitm = modelClosedCircuitVoltageGroup->item(i+1, 0); // взять очередной номер
            Qt::CheckState checkState = sitm->checkState(); // и его состояние
            if (checkState != Qt::Checked) continue; // если не отмечено, то следующий.
        }
        else // в автомате проверять флаги. если НРЦг не проверялось, то и НЗЦг не проверять, во избежание.
        {
            if((!(battery[iBatteryIndex].b_flag_circuit[i] & CIRCUIT_OCG_TESTED))
                    || (battery[iBatteryIndex].b_flag_circuit[i] & CIRCUIT_FAULT))
                continue;
        }

        ui->progressBar->setMaximum(2); // установить кол-во ступеней прогресса !!! в зависимости от времени!
        ui->progressBar->reset();

        // очистить массивы посылки/приёма
        baSendArray.clear();
        baSendCommand.clear();
        baRecvArray.clear();

        // написать в закладке, что измеряется некоторая текущая цепь
        /// формируем строку и пишем на label "идет измерение..."
        sLabelText = tr("%0) \"%1\"").arg(i+1).arg(battery[iBatteryIndex].circuitgroup[i]);
        label = findChild<QLabel*>(tr("labelClosedCircuitVoltageGroup%0").arg(i));
        label->setText(sLabelText + " идет измерение...");
        label->setStyleSheet("QLabel { color : blue; }");

        // сбросить коробочку
        baSendArray = (baSendCommand="IDLE")+"#"; // подготовить буфер для передачи
        timerSend->start(settings.delay_after_request_before_next_ADC1); // послать baSendArray в порт
        // ждём ответа. по сигналу о готовности принятых данных или по таймауту, вывалимся из цикла
        ret=loop.exec();
        if(ret) goto stop; // если не ноль (ошибка таймаута) - вывалиться из режима. если 0, то приняли данные из порта
        ui->progressBar->setValue(ui->progressBar->value()+1);

        // собрать режим
        str_num.sprintf(" %02i %1i", i+1, 3); // напечатать номер цепи и номер тока по протоколу (3 в данном случае)
        baSendArray=(baSendCommand="UccG")+str_num.toLocal8Bit()+"#";
        if(bDeveloperState) Log(QString("Sending ") + qPrintable(baSendArray), "blue");
        timerSend->start(settings.delay_after_IDLE_before_other);
        ret=loop.exec();
        if(ret) goto stop;
        ui->progressBar->setValue(ui->progressBar->value()+1);

        starttime = QDateTime::currentDateTime(); // время начала измерения
        dt = QDateTime::currentDateTime(); // текущее время
        ui->widgetClosedCircuitVoltageGroup->graph(0)->clearData(); // очистить график

        bFirstPoll=true;// после сбора режима первый опрос

        while(-dt.msecsTo(starttime) < cycleTimeSec*1000) // пока время цикла проверки не вышло, продолжим измерять
        {
            // опросить
            baSendArray=baSendCommand+"?#";
            timerSend->start(bFirstPoll?settings.delay_after_start_before_request_ADC1:settings.delay_after_request_before_next_ADC1);
            ret=loop.exec();
            if(ret) goto stop;
            codeADC = getRecvData(baRecvArray); // напряжение в коде
            fU = ((codeADC-settings.offsetADC1)*settings.coefADC1); // напряжение в вольтах
            // нарисуем график
            if(bFirstPoll)
            {
                bFirstPoll = false;
                starttime = QDateTime::currentDateTime(); // время начала измерения начнём считать после получения первого ответа (чтобы график рисовался с нуля)
            }
            dt = QDateTime::currentDateTime(); // текущее время
            x= -dt.msecsTo(starttime); // кол-во миллисекунд, прошедших с начала измерения
            ui->widgetClosedCircuitVoltageGroup->graph(0)->rescaleValueAxis(true); // для автоматического перерисовывания шкалы графика, если значения за пределами экрана
            ui->widgetClosedCircuitVoltageGroup->graph(0)->addData((double)x/1000, (double)fU);
            ui->widgetClosedCircuitVoltageGroup->replot();
        }

        dArrayClosedCircuitVoltageGroup[i] = fU;

        if(bDeveloperState) // если отладочный режим, написать в лог код АЦП
            Log("Цепь "+battery[iBatteryIndex].circuitgroup[i]+" Receive "+qPrintable(baRecvArray)+" codeADC1=0x"+QString("%1").arg((ushort)codeADC, 0, 16), "blue");

        // напечатать рез-т в закладку и в журнал
        if (dArrayClosedCircuitVoltageGroup[i] < settings.closecircuitgroup_limit) {
            sResult = "Не норма!";
            color = "red";
        }
        else {
            sResult = "Норма";
            color = "green";
        }
        label->setText(tr("%0 = <b>%1</b> В. %2").arg(sLabelText).arg(dArrayClosedCircuitVoltageGroup[i], 0, 'f', 2).arg(sResult));
        label->setStyleSheet("QLabel { color : "+color+"; }");
        Log(tr("%0 = <b>%1</b> В. %2").arg(sLabelText).arg(dArrayClosedCircuitVoltageGroup[i], 0, 'f', 2).arg(sResult), color);

        ui->btnBuildReport->setEnabled(true);

        /// заполняем массив проверок для отчета
        dateTime = QDateTime::currentDateTime();
        sArrayReportClosedCircuitVoltageGroup.append(
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

        // по окончанию цикла снять нагрузку, разобрать режим (!!! даже в ручном режиме)
        baSendArray = (baSendCommand="IDLE")+"#";
        timerSend->start(settings.delay_after_request_before_next_ADC1);
        ret=loop.exec();
        if(ret) goto stop;

        // проанализировать результаты, в кодах
        if(codeADC >= codeLimit) // напряжение больше (норма)
        {
            //Log("Напряжение цепи "+battery[iBatteryIndex].circuitgroup[i-1]+" = "+QString::number(fU, 'f', 2)+" В. Норма.", "blue");
            //label->setText(tr("%1) %2 В. Норма.").arg(i).arg(QString::number(fU, 'f', 2)));
            // если ручной режим, то выдать окно сообщения, и только потом разобрать режим измерения.
            // без нагрузки показывать нет смысла if(bModeManual) QMessageBox::information(this, tr("Напряжение замкнутой цепи группы"), tr("Напряжение цепи ")+battery[iBatteryIndex].circuitgroup[i-1]+" = "+QString::number(fU, 'f', 2)+" В\nНорма");
        }
        else // напряжение меньше (не норма)
        {
            //Log("Напряжение цепи "+battery[iBatteryIndex].circuitgroup[i-1]+" = "+QString::number(fU, 'f', 2)+" В. Не норма!", "red");
            //label->setText(tr("%1) %2 В. Не норма!").arg(i).arg(QString::number(fU, 'f', 2)));
            // если ручной режим, то выдать окно сообщения, и только потом разобрать режим измерения.
            // без нагрузки показывать нет смысла if(bModeManual) QMessageBox::information(this, tr("Напряжение замкнутой цепи группы"), tr("Напряжение цепи ")+battery[iBatteryIndex].circuitgroup[i-1]+" = "+QString::number(fU, 'f', 2)+" В\nНе норма!");

            // добавить цепь в список распассивируемых
            switch (QMessageBox::question(this, "Внимание - "+ui->rbClosedCircuitVoltageGroup->text(), tr("%0 = %1 В. %2 Продолжить?").arg(sLabelText).arg(dArrayClosedCircuitVoltageGroup[i], 0, 'f', 2).arg(sResult), tr("Да"), tr("Да, необходима \"Распассивация\""), tr("Нет"))) {
            case 0:
                break;
            case 1:
                //imDepassivation.append(iStepClosedCircuitVoltageGroup-1);
                //ui->cbDepassivation->addItem(battery[iBatteryIndex].circuitgroup[iStepClosedCircuitVoltageGroup-1]);
                // !!! Log(tr("%1) %1 - Х4 «4» добавлен для распассивации.").arg(iStepClosedCircuitVoltageGroup-1), "blue");
                battery[iBatteryIndex].b_flag_circuit[i] |= CIRCUIT_DEPASS; // добавить признак, что группе нужна депассивация
                break;
            case 2:
                //ui->btnClosedCircuitVoltageGroup_2->setEnabled(true);
                bState = true;
                // выйти из цикла, разобрать режим
                goto stop;
                break;
            default:
                break;
            }
            //ui->rbModeDiagnosticManual->setChecked(true); // переключить в ручной принудительно
            //ui->rbModeDiagnosticAuto->setEnabled(false); // запрет автоматической диагностики
        }

    }// конец цикла проверок цепей
stop:
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
    if(ret == KDS_STOP) {
        label->setText(sLabelText + " измерение прервано!");
        label->setStyleSheet("QLabel { color : red; }");
        Log(sLabelText + " измерение прервано!", "red");
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
    ui->btnClosedCircuitVoltageGroup->setText("Пуск");  // поменять текст на кнопке

    timerPing->start(delay_timerPing); // запустить пинг по выходу из режима
    baSendArray.clear(); // очистить буфера команд.
    baSendCommand.clear();
    baRecvArray.clear();
    ui->progressBar->reset();

    // добавить цепи в комбобокс распассивации
    //modelDepassivation = new QStandardItemModel(battery[iBatteryIndex].group_num, 1);
    for (int r = 0; r < battery[iBatteryIndex].group_num; r++)
    {
        QStandardItem* item;
        item = new QStandardItem(QString("%0").arg(battery[iBatteryIndex].circuitgroup[r]));
        if(battery[iBatteryIndex].b_flag_circuit[r] & CIRCUIT_DEPASS)
        {
            item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
            item->setData(Qt::Checked, Qt::CheckStateRole);
        }
        else
        {
            item->setFlags(Qt::NoItemFlags);
            item->setData(Qt::Unchecked, Qt::CheckStateRole);
        }
        modelDepassivation->setItem(r+1, 0, item);

        if(dArrayClosedCircuitVoltageGroup[r] < settings.closecircuitgroup_limit) // если какая-либо цепь была меньше нормы
        {
            ui->rbModeDiagnosticManual->setChecked(true); // переключить в ручной принудительно
            bState = false;
        }
    }
    //ui->cbDepassivation->setModel(modelDepassivation);
    //ui->cbDepassivation->setItemData(0, "DISABLE", Qt::UserRole-1);
    //ui->cbDepassivation->setItemText(0, tr("Выбрано: %0 из %1").arg(battery[iBatteryIndex].group_num).arg(battery[iBatteryIndex].group_num));
}

// слот вызывается при изменении чекбоксов элементов списка комбобокса
void MainWindow::itemChangedClosedCircuitVoltageGroup(QStandardItem* itm)
{
    itm->text(); /// чтобы небыло варнинга при компиляции на неиспользование itm
    int count = 0;
    for(int i=1; i < modelClosedCircuitVoltageGroup->rowCount(); i++)
    {
        QStandardItem *sitm = modelClosedCircuitVoltageGroup->item(i, 0);
        Qt::CheckState checkState = sitm->checkState();
        if (checkState == Qt::Checked)
            count++;
    }
    ui->cbClosedCircuitVoltageGroup->setItemText(0, tr("Выбрано: %0 из %1").arg(count).arg(modelClosedCircuitVoltageGroup->rowCount()-1));
    ui->cbClosedCircuitVoltageGroup->setCurrentIndex(0);
}

/*
 * Напряжение замкнутой цепи группы
 */
/*void MainWindow::checkClosedCircuitVoltageGroup()
{
    qDebug() << "sender=" << ((QPushButton*)sender())->objectName() << "bState=" << bState;
    int x = 10; /// затычка
    ui->widgetClosedCircuitVoltageGroup->graph(0)->clearData(); // очистить график
    ui->tabWidget->addTab(ui->tabClosedCircuitVoltageGroup, ui->rbClosedCircuitVoltageGroup->text());
    ui->tabWidget->setCurrentIndex(ui->tabWidget->count()-1);
    Log(tr("Проверка начата - %1").arg(ui->rbClosedCircuitVoltageGroup->text()), "blue");

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
        ui->cbParamsAutoMode->setCurrentIndex(4); // переключаем режим комбокса на наш

    ui->groupBoxCOMPort->setDisabled(bState);
    ui->groupBoxDiagnosticDevice->setDisabled(bState);
    ui->groupBoxDiagnosticMode->setDisabled(bState);
    ui->cbParamsAutoMode->setDisabled(bState);
    ui->cbSubParamsAutoMode->setDisabled(bState);

    iCurrentStep = (ui->rbModeDiagnosticAuto->isChecked()) ? ui->cbSubParamsAutoMode->currentIndex() : 0;
    iMaxSteps = (ui->rbModeDiagnosticAuto->isChecked()) ? ui->cbSubParamsAutoMode->count() : ui->cbClosedCircuitVoltageGroup->count();

    if (ui->rbModeDiagnosticManual->isChecked()) { /// для ручного режима свой максимум для прогресс бара
        int count = 0;
        for(int i = 0; i < iMaxSteps; i++) {
            QModelIndex index = ui->cbClosedCircuitVoltageGroup->model()->index(i+1, 0);
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
            QModelIndex index = ui->cbClosedCircuitVoltageGroup->model()->index(i+1, 0);
            if(index.data(Qt::CheckStateRole) == 2) { /// проходимся только по выбранным
                switch (i) {
                case 0:
                    delay(1000);
                    dArrayClosedCircuitVoltageGroup[i] = randMToN(26, 28); //число полученное с COM-порта
                    break;
                case 1:
                    delay(1000);
                    dArrayClosedCircuitVoltageGroup[i] = randMToN(26, 28); //число полученное с COM-порта
                    break;
                case 2:
                    delay(1000);
                    dArrayClosedCircuitVoltageGroup[i] = randMToN(26, 28); //число полученное с COM-порта
                    break;
                case 3:
                    delay(1000);
                    dArrayClosedCircuitVoltageGroup[i] = randMToN(26, 28); //число полученное с COM-порта
                    break;
                case 4:
                    delay(1000);
                    dArrayClosedCircuitVoltageGroup[i] = randMToN(26, 28); //число полученное с COM-порта
                    break;
                case 5:
                    delay(1000);
                    dArrayClosedCircuitVoltageGroup[i] = randMToN(26, 28); //число полученное с COM-порта
                    break;
                case 6:
                    delay(1000);
                    dArrayClosedCircuitVoltageGroup[i] = randMToN(26, 28); //число полученное с COM-порта
                    break;
                case 7:
                    delay(1000);
                    dArrayClosedCircuitVoltageGroup[i] = randMToN(26, 28); //число полученное с COM-порта
                    break;
                case 8:
                    delay(1000);
                    dArrayClosedCircuitVoltageGroup[i] = randMToN(26, 28); //число полученное с COM-порта
                    break;
                case 9:
                    delay(1000);
                    dArrayClosedCircuitVoltageGroup[i] = randMToN(26, 28); //число полученное с COM-порта
                    break;
                case 10:
                    delay(1000);
                    dArrayClosedCircuitVoltageGroup[i] = randMToN(26, 28); //число полученное с COM-порта
                    break;
                case 11:
                    delay(1000);
                    dArrayClosedCircuitVoltageGroup[i] = randMToN(26, 28); //число полученное с COM-порта
                    break;
                case 12:
                    delay(1000);
                    dArrayClosedCircuitVoltageGroup[i] = randMToN(26, 28); //число полученное с COM-порта
                    break;
                case 13:
                    delay(1000);
                    dArrayClosedCircuitVoltageGroup[i] = randMToN(26, 28); //число полученное с COM-порта
                    break;
                case 14:
                    delay(1000);
                    dArrayClosedCircuitVoltageGroup[i] = randMToN(26, 28); //число полученное с COM-порта
                    break;
                case 15:
                    delay(1000);
                    dArrayClosedCircuitVoltageGroup[i] = randMToN(26, 28); //число полученное с COM-порта
                    break;
                case 16:
                    delay(1000);
                    dArrayClosedCircuitVoltageGroup[i] = randMToN(26, 28); //число полученное с COM-порта
                    break;
                case 17:
                    delay(1000);
                    dArrayClosedCircuitVoltageGroup[i] = randMToN(26, 28); //число полученное с COM-порта
                    break;
                case 18:
                    delay(1000);
                    dArrayClosedCircuitVoltageGroup[i] = randMToN(26, 28); //число полученное с COM-порта
                    break;
                case 19:
                    delay(1000);
                    dArrayClosedCircuitVoltageGroup[i] = randMToN(26, 28); //число полученное с COM-порта
                    break;
                default:
                    return;
                    break;
                }
                qDebug() << "dArrayClosedCircuitVoltageGroup[" << i << "]=" << dArrayClosedCircuitVoltageGroup[i];
                if(ui->rbModeDiagnosticAuto->isChecked())
                    ui->cbSubParamsAutoMode->setCurrentIndex(i);

                str = tr("Напряжение цепи \"%0\" = <b>%1</b> В.").arg(battery[iBatteryIndex].circuitgroup[i]).arg(dArrayClosedCircuitVoltageGroup[i]);
                QLabel * label = findChild<QLabel*>(tr("labelClosedCircuitVoltageGroup%0").arg(i));
                if (dArrayClosedCircuitVoltageGroup[i] > settings.closecircuitgroup_limit) {
                    str += " Не норма.";
                    color = "red";
                } else
                    color = "green";

                label->setText(str);
                label->setStyleSheet("QLabel { color : "+color+"; }");
                Log(str, color);
                /// рисуем график
                ui->widgetClosedCircuitVoltageGroup->graph(0)->rescaleValueAxis(true); // для автоматического перерисовывания шкалы графика, если значения за пределами экрана
                ui->widgetClosedCircuitVoltageGroup->graph(0)->addData((double)x/100, (double)dArrayClosedCircuitVoltageGroup[i]);
                ui->widgetClosedCircuitVoltageGroup->replot();
                x +=40; /// затычка
                ui->btnBuildReport->setEnabled(true);
                if (dArrayClosedCircuitVoltageGroup[i] > settings.closecircuitgroup_limit) {
                    switch (QMessageBox::question(this, "Внимание - "+ui->rbClosedCircuitVoltageGroup->text(), tr("%0 Продолжить?").arg(str), tr("Да"), tr("Да, необходима \"Распассивация\""), tr("Нет"))) {
                    case 0:
                        break;
                    case 1:
                        QStandardItem* item;
                        item = new QStandardItem(QString("%0").arg(battery[iBatteryIndex].circuitgroup[i]));
                        item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
                        item->setData(Qt::Checked, Qt::CheckStateRole);
                        modelDepassivation->setItem(i+1, 0, item);
                        break;
                    case 2:
                        bState = false;
                        ui->groupBoxCOMPort->setDisabled(bState);
                        ui->groupBoxDiagnosticMode->setDisabled(bState);
                        ui->cbParamsAutoMode->setDisabled(bState);
                        ui->cbSubParamsAutoMode->setDisabled(bState);
                        ((QPushButton*)sender())->setText("Пуск");
                        return;
                        break;
                    default:
                        break;
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

    Log(tr("Проверка завершена - %1").arg(ui->rbClosedCircuitVoltageGroup->text()), "blue");

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

#if 0
switch (iBatteryIndex) {
case 0: //9ER20P-20 // !!! ваще кейс не нужен, кол-во цепей брать из структуры.
    ui->progressBar->setValue(iStepClosedCircuitVoltageGroup-1);
    ui->progressBar->setMaximum(20); // !!! кол-во проверяемых цепей зависит от кол-ва установленных чекбоксов (и от неисправных без нагрузки цепей, допустим)
    while (iStepClosedCircuitVoltageGroup <= 20) {
        if (bPause) return;
        switch (iStepClosedCircuitVoltageGroup) {
        case 1:
            delay(1000);
            param = randMToN(26, 29); //число полученное с COM-порта
            break;
        case 2:
            delay(1000);
            param = randMToN(26, 29); //число полученное с COM-порта
            break;
        case 3:
            delay(1000);
            param = randMToN(26, 29); //число полученное с COM-порта
            break;
        case 4:
            delay(1000);
            param = randMToN(26, 29); //число полученное с COM-порта
            break;
        case 5:
            delay(1000);
            param = randMToN(26, 29); //число полученное с COM-порта
            break;
        case 6:
            delay(1000);
            param = randMToN(26, 29); //число полученное с COM-порта
            break;
        case 7:
            delay(1000);
            param = randMToN(26, 29); //число полученное с COM-порта
            break;
        case 8:
            delay(1000);
            param = randMToN(26, 29); //число полученное с COM-порта
            break;
        case 9:
            delay(1000);
            param = randMToN(26, 29); //число полученное с COM-порта
            break;
        case 10:
            delay(1000);
            param = randMToN(26, 29); //число полученное с COM-порта
            break;
        case 11:
            delay(1000);
            param = randMToN(26, 29); //число полученное с COM-порта
            break;
        case 12:
            delay(1000);
            param = randMToN(26, 29); //число полученное с COM-порта
            break;
        case 13:
            delay(1000);
            param = randMToN(26, 29); //число полученное с COM-порта
            break;
        case 14:
            delay(1000);
            param = randMToN(26, 29); //число полученное с COM-порта
            break;
        case 15:
            delay(1000);
            param = randMToN(26, 29); //число полученное с COM-порта
            break;
        case 16:
            delay(1000);
            param = randMToN(26, 29); //число полученное с COM-порта
            break;
        case 17:
            delay(1000);
            param = randMToN(26, 29); //число полученное с COM-порта
            break;
        case 18:
            delay(1000);
            param = randMToN(26, 29); //число полученное с COM-порта
            break;
        case 19:
            delay(1000);
            param = randMToN(26, 29); //число полученное с COM-порта
            break;
        case 20:
            delay(1000);
            param = randMToN(26, 29); //число полученное с COM-порта
            break;
        default:
            return;
            break;
        }
        //ui->widgetClosedCircuitVoltageGroup->graph(0)->clearData();
        ui->widgetClosedCircuitVoltageGroup->graph(0)->rescaleValueAxis(true);
        ui->widgetClosedCircuitVoltageGroup->graph(0)->addData(h*(iStepClosedCircuitVoltageGroup-1), param);
        //ui->widgetClosedCircuitVoltageGroup->graph(1)->clearData();
        //ui->widgetClosedCircuitVoltageGroup->graph(1)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, Qt::green, Qt::white, 7));
        /*if (param < settings.closecircuitgroup_limit) {
            QPen gridPen;
            gridPen.setStyle(Qt::DotLine);
            gridPen.setColor(QColor(255, 0, 0, 25));
            ui->widgetClosedCircuitVoltageGroup->graph(1)->setPen(gridPen);
        }*/
        //ui->widgetClosedCircuitVoltageGroup->graph(1)->addData(h*(iStepClosedCircuitVoltageGroup-1), param);
        ui->widgetClosedCircuitVoltageGroup->replot();

        QLabel * label = findChild<QLabel*>(tr("labelClosedCircuitVoltageGroup%1").arg(iStepClosedCircuitVoltageGroup));
        label->setText(tr("%1) %2").arg(iStepClosedCircuitVoltageGroup).arg(QString::number(param)));
        str = tr("%1) между контактом 1 соединителя Х3 «Х3-» и контактом %1 соединителя Х4 «4» = <b>%2</b>").arg(iStepClosedCircuitVoltageGroup).arg(QString::number(param));
        Log(str, (param < settings.closecircuitgroup_limit) ? "red" : "green");
        if (param < settings.closecircuitgroup_limit) {
            int ret = QMessageBox::question(this, "Внимание - "+ui->rbClosedCircuitVoltageGroup->text(), tr("%1 \nпродолжить?").arg(str), tr("Да"), tr("Да, необходима \"Распассивация\""), tr("Нет"));
            switch (ret) {
            case 0:
                break;
            case 1:
                imDepassivation.append(iStepClosedCircuitVoltageGroup-1);
                Log(tr("%1) %1 - Х4 «4» добавлен для распассивации.").arg(iStepClosedCircuitVoltageGroup-1), "blue");
                break;
            case 2:
                ui->btnClosedCircuitVoltageGroup_2->setEnabled(true);
                bPause = true;
                return;
                break;
            default:
                break;
            }
            ui->rbModeDiagnosticManual->setChecked(true);
            ui->rbModeDiagnosticAuto->setEnabled(false);
            //ui->rbInsulationResistance->setChecked(true);
            /*if (QMessageBox::question(this, "Внимание - "+ui->rbClosedCircuitVoltageGroup->text(), tr("%1 \nпродолжить?").arg(str), tr("Да"), tr("Нет"))) {
                ui->btnClosedCircuitVoltageGroup_2->setEnabled(true);
                bPause = true;
                return;
            }*/
        }
        progressBarSet(1);
        iStepClosedCircuitVoltageGroup++;
    }
    if (imDepassivation.count() != 0)
        ui->rbDepassivation->setEnabled(true);
    ui->btnClosedCircuitVoltageGroup_2->setEnabled(false);
    if (ui->rbModeDiagnosticAuto->isChecked())
        bCheckCompleteClosedCircuitVoltageGroup = true;
    break;
case 1:
    if (bPause) return;
    Log("Действия проверки.", "green");
    delay(1000);
    progressBarSet(1);
    break;
case 2:
    if (bPause) return;
    Log("Действия проверки.", "green");
    delay(1000);
    progressBarSet(1);
    break;
case 3:
    if (bPause) return;
    Log("Действия проверки.", "green");
    delay(1000);
    progressBarSet(1);
    break;
default:
    break;
}
#endif

#if 0


#endif
