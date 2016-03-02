#include <QDebug>
#include <QMessageBox>
#include <QDateTime>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "battery.h"

extern QVector<Battery> battery;

// Нажата кнопка распассивации
void MainWindow::on_btnDepassivation_clicked()
{
    //checkDepassivation(); return;
    quint16 codeADC=0; // принятый код АЦП
    float fU=0; // принятое напряжение в вольтах
    // код порогового напряжения = пороговое напряжение В / коэфф. (вес разряда) + смещение (в коде)
    //quint16 codeLimit=settings.closecircuitgroup_limit/settings.coefADC1 + settings.offsetADC1; // код, пороговое напряжение.
    int ret=0; // код возврата ошибки
    QString str_num; // номер цепи
    QDateTime starttime; // время начала измерения
    QDateTime dt; // текущее время очередного измерения
    double x; // текущая координата Х
    int cycleTimeSec=0; // длительность цикла проверки в секундах
    bool firstMeasurement=true; // первое измерение

    if(loop.isRunning()){qDebug()<<"loop.isRunning()!"; return;} // костыль: если цикл уже работает - выйти обратно
    timerPing->stop(); // остановить пинг

    baSendArray.clear();
    baSendCommand.clear();
    baRecvArray.clear();

    // написать про группы, в зависимости от признаков и флагов
    for(int i=1; i<battery[iBatteryIndex].group_num+1; i++)
    {
        QStandardItem *sitm = modelDepassivation->item(i, 0);
        Qt::CheckState checkState = sitm->checkState();

        QLabel * label = findChild<QLabel*>(tr("labelDepassivation%1").arg(i));
        if(!(battery[iBatteryIndex].b_flag_circuit[i-1] & CIRCUIT_OCG_TESTED))
        {
            label->setText(tr("%1) НРЦг не проверялось.").arg(i));
        }
        else if(battery[iBatteryIndex].b_flag_circuit[i-1] & CIRCUIT_FAULT)
        {
            label->setText(tr("%1) НРЦг меньше нормы, проверка под нагрузкой запрещена.").arg(i));
        }
        else if(!(battery[iBatteryIndex].b_flag_circuit[i-1] & CIRCUIT_DEPASS) || (checkState != Qt::Checked))
        {
            label->setText(tr("%1) Распассивация не требуется.").arg(i));
        }
        else
        {
            label->setText(tr("%1)").arg(i));
        }
    }
    // !!! лишние label вообще стереть.
    // Подготовка графика
    /*ui->widgetDepassivation->addGraph(); // blue line
    ui->widgetDepassivation->graph(0)->setPen(QPen(Qt::blue));
    ui->widgetDepassivation->graph(0)->clearData();
    ui->widgetDepassivation->addGraph(); // blue dot
    ui->widgetDepassivation->graph(1)->clearData();
    ui->widgetDepassivation->graph(1)->setLineStyle(QCPGraph::lsNone);
    //ui->widgetDepassivation->graph(1)->setPen(QPen(Qt::green));
    ui->widgetDepassivation->graph(1)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, Qt::blue, Qt::white, 7));*/
    /*ui->widgetDepassivation->addGraph(); // red line
    ui->widgetDepassivation->graph(2)->setPen(QPen(Qt::red));
    ui->widgetDepassivation->graph(2)->setBrush(QBrush(QColor(255, 0, 0, 20)));
    ui->widgetDepassivation->graph(2)->clearData();
    ui->widgetDepassivation->graph(2)->addData(0, settings.closecircuitgroup_limit);
    ui->widgetDepassivation->graph(2)->addData(cycleTimeSec+1, settings.closecircuitgroup_limit);*/

    /*ui->widgetDepassivation->xAxis->setLabel(tr("Время, c"));
    // ниже в цикле ui->widgetDepassivation->xAxis->setRange(0, cycleTimeSec+1);
    ui->widgetDepassivation->yAxis->setLabel(tr("Напряжение, В"));
    ui->widgetDepassivation->yAxis->setRange(24, 33);*/

    ui->tabWidget->addTab(ui->tabDepassivation, ui->rbDepassivation->text());
    ui->tabWidget->setCurrentIndex(ui->tabWidget->count()-1);
    Log(tr("Проверка начата - %1").arg(ui->rbClosedCircuitVoltageGroup->text()), "blue");

    // сбросить коробочку
    baSendArray = (baSendCommand="IDLE")+"#"; // подготовить буфер для передачи
    sendSerialData(); // послать baSendArray в порт
    // ждём ответа. по сигналу о готовности принятых данных или по таймауту, вывалимся из цикла
    ret=loop.exec();
    if(ret) goto stop; // если не ноль (ошибка таймаута) - вывалиться из режима. если 0, то приняли данные из порта

    // !!! если нечего проверять, список пуст - проверить алгоритм
    // Пробежимся по списку цепей
    for(int i=1; i < modelDepassivation->rowCount(); i++)
    {
        QStandardItem *sitm = modelDepassivation->item(i, 0);
        Qt::CheckState checkState = sitm->checkState();
        if (checkState != Qt::Checked) continue;
        label = findChild<QLabel*>(tr("labelDepassivation%0").arg(i));

        // три ступени распассивации
        for(int k=0; k<3; k++)
        {
            cycleTimeSec = settings.time_depassivation[k];
            ui->widgetDepassivation->xAxis->setRange(0, cycleTimeSec+1); // длительность цикла
            label->setText(tr("%1) Идёт распассивация током %2 А...").arg(i).arg(QString::number(settings.depassivation_current[k])));

            // собрать режим
            str_num.sprintf(" %02i %1i", i, k+1); // напечатать номер цепи и номер тока по протоколу
            baSendArray=(baSendCommand="UccG")+str_num.toLocal8Bit()+"#";
            if(bDeveloperState) Log(QString("Sending ") + qPrintable(baSendArray), "blue");
            QTimer::singleShot(settings.delay_after_IDLE_before_other, this, SLOT(sendSerialData()));
            ret=loop.exec();
            if(ret) goto stop;

            starttime = QDateTime::currentDateTime(); // время начала измерения
            dt = QDateTime::currentDateTime(); // текущее время
            ui->widgetDepassivation->graph(0)->clearData(); // очистить график

            // цикл измерения
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
                ui->widgetDepassivation->graph(0)->rescaleValueAxis(true); // для автоматического перерисовывания шкалы графика, если значения за пределами экрана
                ui->widgetDepassivation->graph(0)->addData((double)x/1000, (double)fU);
                ui->widgetDepassivation->replot();
            }

            if(bDeveloperState)
                Log("Цепь "+battery[iBatteryIndex].circuitgroup[i-1]+" Receive "+qPrintable(baRecvArray)+" codeADC1=0x"+QString("%1").arg((ushort)codeADC, 0, 16), "blue");

            // по окончанию цикла снять нагрузку, разобрать режим
            baSendArray = (baSendCommand="IDLE")+"#";
            QTimer::singleShot(settings.delay_after_request_before_next_ADC1, this, SLOT(sendSerialData()));
            ret=loop.exec();
            if(ret) goto stop;
        }
        //label->setText(tr("%1) Распассивация закончена").arg(i));

        sResult = "Выполнена";
        color = "green";
        label->setText(str+" "+sResult);
        label->setStyleSheet("QLabel { color : "+color+"; }");
        Log(str+" "+sResult, color);

        ui->btnBuildReport->setEnabled(true);

        /// заполняем массив проверок для отчета
        dateTime = QDateTime::currentDateTime();
        sArrayReportDepassivation.append(
                    tr("<tr>"\
                       "    <td>%0</td>"\
                       "    <td>%1</td>"\
                       "    <td>%2</td>"\
                       "    <td>%3</td>"\
                       "</tr>")
                    .arg(dateTime.toString("hh:mm:ss"))
                    .arg(i+1)
                    .arg(battery[iBatteryIndex].circuitgroup[i])
                    .arg(sResult));
#if 0
        str = tr("%1) между контактом 1 соединителя Х3 «Х3-» и контактом %1 соединителя Х4 «4» = <b>%2</b>").arg(i).arg(QString::number(fU));
        Log(str, (fU < settings.closecircuitgroup_limit) ? "red" : "green");
        // проанализировать результаты
        if(codeADC >= codeLimit) // напряжение больше (норма)
        {
            Log("Напряжение цепи "+battery[iBatteryIndex].circuitgroup[i-1]+" = "+QString::number(fU, 'f', 2)+" В.  Норма.", "blue");
            // если ручной режим, то выдать окно сообщения, и только потом разобрать режим измерения.
            if(bModeManual) QMessageBox::information(this, tr("Напряжение замкнутой цепи группы"), tr("Напряжение цепи ")+battery[iBatteryIndex].circuitgroup[i-1]+" = "+QString::number(fU, 'f', 2)+" В\nНорма");
        }
        else // напряжение меньше (не норма)
        {
            Log("Напряжение цепи "+battery[iBatteryIndex].circuitgroup[i-1]+" = "+QString::number(fU, 'f', 2)+" В.  Не норма!.", "red");
            // если ручной режим, то выдать окно сообщения, и только потом разобрать режим измерения.
            if(bModeManual) QMessageBox::information(this, tr("Напряжение замкнутой цепи группы"), tr("Напряжение цепи ")+battery[iBatteryIndex].circuitgroup[i-1]+" = "+QString::number(fU, 'f', 2)+" В\nНе норма!");
            // !!! добавить цепь в список распассивируемых
            //int rett = QMessageBox::question(this, "Внимание - "+ui->rbClosedCircuitVoltageGroup->text(), tr("%1 \nпродолжить?").arg(str), tr("Да"), tr("Да, необходима \"Распассивация\""), tr("Нет"));
            switch (QMessageBox::question(this, "Внимание - "+ui->rbClosedCircuitVoltageGroup->text(), tr("%1 \nпродолжить?").arg(str), tr("Да"), tr("Да, необходима \"Распассивация\""), tr("Нет"))) {
            case 0:
                break;
            case 1:
                imDepassivation.append(iStepClosedCircuitVoltageGroup-1);
                ui->cbDepassivation->addItem(battery[iBatteryIndex].circuitgroup[iStepClosedCircuitVoltageGroup-1]);
                Log(tr("%1) %1 - Х4 «4» добавлен для распассивации.").arg(iStepClosedCircuitVoltageGroup-1), "blue");
                break;
            case 2:
                //ui->btnClosedCircuitVoltageGroup_2->setEnabled(true);
                bState = true;
                return; // !!! тут как-то надо выйти из цикла, чтобы разобрать режим
                break;
            default:
                break;
            }
            ui->rbModeDiagnosticManual->setChecked(true);
            ui->rbModeDiagnosticAuto->setEnabled(false);
        }
#endif
    }// конец цикла проверок цепей
stop:
    // если отладочный режим, напечатать отладочную инфу
    if(bDeveloperState)
    {
        if(ret==1) Log(tr("Timeout!"), "red");
        else if(ret==2) Log(tr("Incorrect reply!"), "red");
    }

    // разобрать режим
    baSendArray = (baSendCommand="IDLE")+"#";
    QTimer::singleShot(settings.delay_after_request_before_next_ADC1, this, SLOT(sendSerialData()));
    ret=loop.exec();
    if(ret) goto stop;

    timerPing->start(delay_timerPing); // запустить пинг по выходу из режима
    baSendArray.clear(); // очистить буфера команд.
    baSendCommand.clear();
    baRecvArray.clear();

    Log(tr("Проверка завершена - %1").arg(ui->rbDepassivation->text()), "blue");
    //iStepClosedCircuitVoltageGroup = 1;
    ui->rbDepassivation->setEnabled(true);
    ui->groupBoxCOMPort->setEnabled(true);
    ui->groupBoxDiagnosticDevice->setEnabled(true);
    ui->groupBoxDiagnosticMode->setEnabled(true);
}

// слот вызывается при изменении чекбоксов элементов списка комбобокса
void MainWindow::itemChangedDepassivation(QStandardItem* itm)
{
    itm->text(); /// чтобы небыло варнинга при компиляции на неиспользование itm
    int count = 0;
    for(int i=1; i < modelDepassivation->rowCount(); i++)
    {
        QStandardItem *sitm = modelDepassivation->item(i, 0);
        Qt::CheckState checkState = sitm->checkState();
        if (checkState == Qt::Checked)
            count++;
    }
    ui->cbDepassivation->setItemText(0, tr("Выбрано: %0 из %1").arg(count).arg(modelDepassivation->rowCount()-1));
    ui->cbDepassivation->setCurrentIndex(0);
}

/*
 * Распассивация
 */
void MainWindow::checkDepassivation()
{
    int x = 10; /// затычка
    ui->widgetDepassivation->graph(0)->clearData(); // очистить график
    qDebug() << "sender=" << ((QPushButton*)sender())->objectName() << "bState=" << bState;
    ui->tabWidget->addTab(ui->tabDepassivation, ui->rbDepassivation->text());
    ui->tabWidget->setCurrentIndex(ui->tabWidget->count()-1);
    Log(tr("Проверка начата - %1").arg(ui->rbDepassivation->text()), "blue");

    if(ui->rbModeDiagnosticManual->isChecked()) {
        if(!bState) {
            bState = true;
            ui->groupBoxCheckParams->setEnabled(bState);
            ((QPushButton*)sender())->setText("Стоп");
        } else {
            bState = false;
            ((QPushButton*)sender())->setText("Пуск");
        }
    }

    ui->groupBoxCOMPort->setDisabled(bState);
    ui->groupBoxDiagnosticDevice->setDisabled(bState);
    ui->groupBoxDiagnosticMode->setDisabled(bState);
    ui->cbParamsAutoMode->setDisabled(bState);
    ui->cbSubParamsAutoMode->setDisabled(bState);

    iCurrentStep = (ui->rbModeDiagnosticAuto->isChecked()) ? ui->cbSubParamsAutoMode->currentIndex() : 0;
    iMaxSteps = (ui->rbModeDiagnosticAuto->isChecked()) ? ui->cbSubParamsAutoMode->count() : ui->cbDepassivation->count();

    if (ui->rbModeDiagnosticManual->isChecked()) { /// для ручного режима свой максимум для прогресс бара
        int count = 0;
        for(int i = 0; i < iMaxSteps; i++) {
            QModelIndex index = ui->cbDepassivation->model()->index(i+1, 0);
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
            QModelIndex index = ui->cbDepassivation->model()->index(i+1, 0);
            if(index.data(Qt::CheckStateRole) == 2) { /// проходимся только по выбранным
                switch (i) {
                case 0:
                    delay(1000);
                    dArrayDepassivation[i] = randMToN(26, 28); //число полученное с COM-порта
                    break;
                case 1:
                    delay(1000);
                    dArrayDepassivation[i] = randMToN(26, 28); //число полученное с COM-порта
                    break;
                case 2:
                    delay(1000);
                    dArrayDepassivation[i] = randMToN(26, 28); //число полученное с COM-порта
                    break;
                case 3:
                    delay(1000);
                    dArrayDepassivation[i] = randMToN(26, 28); //число полученное с COM-порта
                    break;
                case 4:
                    delay(1000);
                    dArrayDepassivation[i] = randMToN(26, 28); //число полученное с COM-порта
                    break;
                case 5:
                    delay(1000);
                    dArrayDepassivation[i] = randMToN(26, 28); //число полученное с COM-порта
                    break;
                case 6:
                    delay(1000);
                    dArrayDepassivation[i] = randMToN(26, 28); //число полученное с COM-порта
                    break;
                case 7:
                    delay(1000);
                    dArrayDepassivation[i] = randMToN(26, 28); //число полученное с COM-порта
                    break;
                case 8:
                    delay(1000);
                    dArrayDepassivation[i] = randMToN(26, 28); //число полученное с COM-порта
                    break;
                case 9:
                    delay(1000);
                    dArrayDepassivation[i] = randMToN(26, 28); //число полученное с COM-порта
                    break;
                case 10:
                    delay(1000);
                    dArrayDepassivation[i] = randMToN(26, 28); //число полученное с COM-порта
                    break;
                case 11:
                    delay(1000);
                    dArrayDepassivation[i] = randMToN(26, 28); //число полученное с COM-порта
                    break;
                case 12:
                    delay(1000);
                    dArrayDepassivation[i] = randMToN(26, 28); //число полученное с COM-порта
                    break;
                case 13:
                    delay(1000);
                    dArrayDepassivation[i] = randMToN(26, 28); //число полученное с COM-порта
                    break;
                case 14:
                    delay(1000);
                    dArrayDepassivation[i] = randMToN(26, 28); //число полученное с COM-порта
                    break;
                case 15:
                    delay(1000);
                    dArrayDepassivation[i] = randMToN(26, 28); //число полученное с COM-порта
                    break;
                case 16:
                    delay(1000);
                    dArrayDepassivation[i] = randMToN(26, 28); //число полученное с COM-порта
                    break;
                case 17:
                    delay(1000);
                    dArrayDepassivation[i] = randMToN(26, 28); //число полученное с COM-порта
                    break;
                case 18:
                    delay(1000);
                    dArrayDepassivation[i] = randMToN(26, 28); //число полученное с COM-порта
                    break;
                case 19:
                    delay(1000);
                    dArrayDepassivation[i] = randMToN(26, 28); //число полученное с COM-порта
                    break;
                default:
                    return;
                    break;
                }
                qDebug() << "dArrayDepassivation[" << i << "]=" << dArrayDepassivation[i];
                if(ui->rbModeDiagnosticAuto->isChecked())
                    ui->cbSubParamsAutoMode->setCurrentIndex(i);

                str = tr("Напряжение цепи \"%0\" = <b>%1</b> В.").arg(battery[iBatteryIndex].circuitgroup[i]).arg(dArrayDepassivation[i]);
                QLabel * label = findChild<QLabel*>(tr("labelDepassivation%0").arg(i));
                if (dArrayDepassivation[i] > settings.closecircuitgroup_limit) {
                    str += " Не норма.";
                    color = "red";
                } else
                    color = "green";

                label->setText(str);
                label->setStyleSheet("QLabel { color : "+color+"; }");
                Log(str, color);
                /// рисуем график
                ui->widgetDepassivation->graph(0)->rescaleValueAxis(true); // для автоматического перерисовывания шкалы графика, если значения за пределами экрана
                ui->widgetDepassivation->graph(0)->addData((double)x/100, (double)dArrayDepassivation[i]);
                ui->widgetDepassivation->replot();
                x +=40; /// затычка
                ui->btnBuildReport->setEnabled(true);
                if (dArrayDepassivation[i] > settings.closecircuitgroup_limit) {
                    if (QMessageBox::question(this, "Внимание - "+ui->rbDepassivation->text(), tr("%0 Продолжить?").arg(str), tr("Да"), tr("Нет"))) {
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

    Log(tr("Проверка завершена - %1").arg(ui->rbDepassivation->text()), "blue");

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
