#include <QDebug>
#include <QMessageBox>
#include <QDateTime>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "battery.h"

extern QVector<Battery> battery;

// Нажата кнопка проверки напряжения замкнутых цепей групп
void MainWindow::on_btnClosedCircuitVoltageGroup_clicked()
{
    qDebug()<<"checkClosedCircuitVoltageGroup";
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
    bool firstMeasurement=true; // первое измерение

    if(loop.isRunning()){qDebug()<<"loop.isRunning()!"; return;} // костыль: если цикл уже работает - выйти обратно
    timerPing->stop(); // остановить пинг

    baSendArray.clear();
    baSendCommand.clear();
    baRecvArray.clear();

    // написать про группы, в зависимости от признаков и флагов. !!! если НРЦг не проверялось, то и НЗЦг не проверять, во избежание.
    for(int i=1; i<battery[iBatteryIndex].group_num+1; i++)
    {
        QLabel * label = findChild<QLabel*>(tr("labelClosedCircuitVoltageGroup%1").arg(i));
        if(!(battery[iBatteryIndex].b_flag_circuit[i-1] & CIRCUIT_OCG_TESTED))
        {
            label->setText(tr("%1) НРЦг не проверялось.").arg(i));
        }
        else if(battery[iBatteryIndex].b_flag_circuit[i-1] & CIRCUIT_FAULT)
        {
            label->setText(tr("%1) НРЦг меньше нормы, проверка под нагрузкой запрещена.").arg(i));
        }
        else
        {
            label->setText(tr("%1)").arg(i));
        }
    }
    // !!! лишние label вообще стереть.
    // подготовить график
    ui->widgetClosedCircuitVoltageGroup->addGraph(); // blue line
    ui->widgetClosedCircuitVoltageGroup->graph(0)->setPen(QPen(Qt::blue));
    ui->widgetClosedCircuitVoltageGroup->graph(0)->clearData();
    ui->widgetClosedCircuitVoltageGroup->addGraph(); // blue dot
    ui->widgetClosedCircuitVoltageGroup->graph(1)->clearData();
    ui->widgetClosedCircuitVoltageGroup->graph(1)->setLineStyle(QCPGraph::lsNone);
    //ui->widgetClosedCircuitVoltageGroup->graph(1)->setPen(QPen(Qt::green));
    ui->widgetClosedCircuitVoltageGroup->graph(1)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, Qt::blue, Qt::white, 7));
    ui->widgetClosedCircuitVoltageGroup->addGraph(); // red line
    ui->widgetClosedCircuitVoltageGroup->graph(2)->setPen(QPen(Qt::red));
    ui->widgetClosedCircuitVoltageGroup->graph(2)->setBrush(QBrush(QColor(255, 0, 0, 20)));
    ui->widgetClosedCircuitVoltageGroup->graph(2)->clearData();
    ui->widgetClosedCircuitVoltageGroup->graph(2)->addData(0, settings.closecircuitgroup_limit);
    ui->widgetClosedCircuitVoltageGroup->graph(2)->addData(cycleTimeSec+1, settings.closecircuitgroup_limit);

    ui->widgetClosedCircuitVoltageGroup->xAxis->setLabel(tr("Время, c"));
    ui->widgetClosedCircuitVoltageGroup->xAxis->setRange(0, cycleTimeSec+1);
    ui->widgetClosedCircuitVoltageGroup->yAxis->setLabel(tr("Напряжение, В"));
    ui->widgetClosedCircuitVoltageGroup->yAxis->setRange(24, 33);

    if (((QPushButton*)sender())->objectName() == "btnClosedCircuitVoltageGroup") {
        //iStepClosedCircuitVoltageGroup = 1;
        bState = false;
        //ui->btnClosedCircuitVoltageGroup_2->setEnabled(false);
    }
    if (((QPushButton*)sender())->objectName() == "btnClosedCircuitVoltageGroup_2")
        bState = false;
    // !!! спросить смысл if (!bState) return;
    ui->groupBoxCOMPort->setEnabled(false);
    ui->groupBoxDiagnosticDevice->setEnabled(false);
    ui->groupBoxDiagnosticMode->setEnabled(false);
    ui->tabWidget->addTab(ui->tabClosedCircuitVoltageGroup, ui->rbClosedCircuitVoltageGroup->text());
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
    for(int i=1; i < modelClosedCircuitVoltageGroup->rowCount(); i++)
    {
        QStandardItem *sitm = modelClosedCircuitVoltageGroup->item(i, 0);
        Qt::CheckState checkState = sitm->checkState();
        if (checkState != Qt::Checked) continue;

        QLabel * label = findChild<QLabel*>(tr("labelClosedCircuitVoltageGroup%1").arg(i));
        label->setText(tr("%1) Идёт проверка ...").arg(i));
        // собрать режим
        str_num.sprintf(" %02i %1i", i, 3); // напечатать номер цепи и номер тока по протоколу (3 в данном случае)
        baSendArray=(baSendCommand="UccG")+str_num.toLocal8Bit()+"#";
        if(bDeveloperState) Log(QString("Sending ") + qPrintable(baSendArray), "blue");
        QTimer::singleShot(settings.delay_after_IDLE_before_other, this, SLOT(sendSerialData()));
        ret=loop.exec();
        if(ret) goto stop;

        starttime = QDateTime::currentDateTime(); // время начала измерения
        dt = QDateTime::currentDateTime(); // текущее время
        ui->widgetClosedCircuitVoltageGroup->graph(0)->clearData(); // очистить график

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
            ui->widgetClosedCircuitVoltageGroup->graph(0)->rescaleValueAxis(true); // для автоматического перерисовывания шкалы графика, если значения за пределами экрана
            ui->widgetClosedCircuitVoltageGroup->graph(0)->addData((double)x/1000, (double)fU);
            ui->widgetClosedCircuitVoltageGroup->replot();
        }

        if(bDeveloperState) // если отладочный режим, написать в лог код АЦП
            Log("Цепь "+battery[iBatteryIndex].circuitgroup[i-1]+" Receive "+qPrintable(baRecvArray)+" codeADC1=0x"+QString("%1").arg((ushort)codeADC, 0, 16), "blue");

        // по окончанию цикла снять нагрузку, разобрать режим (!!! даже в ручном режиме)
        baSendArray = (baSendCommand="IDLE")+"#";
        QTimer::singleShot(settings.delay_after_request_before_next_ADC1, this, SLOT(sendSerialData()));
        ret=loop.exec();
        if(ret) goto stop;

        str = tr("%1) Напряжение цепи 1 Х3 «Х3-» - %1 Х4 «4» = <b>%2</b>").arg(i).arg(QString::number(fU, 'f', 2));
        Log(str, (fU < settings.closecircuitgroup_limit) ? "red" : "green"); // !!! анализ рез-та в вольтах. переделать в коды, как ниже.
        // проанализировать результаты, в кодах
        if(codeADC >= codeLimit) // напряжение больше (норма)
        {
            Log("Напряжение цепи "+battery[iBatteryIndex].circuitgroup[i-1]+" = "+QString::number(fU, 'f', 2)+" В. Норма.", "blue");
            label->setText(tr("%1) %2 В. Норма.").arg(i).arg(QString::number(fU, 'f', 2)));
            // если ручной режим, то выдать окно сообщения, и только потом разобрать режим измерения. !!! под нагрузкой???
            if(bModeManual) QMessageBox::information(this, tr("Напряжение замкнутой цепи группы"), tr("Напряжение цепи ")+battery[iBatteryIndex].circuitgroup[i-1]+" = "+QString::number(fU, 'f', 2)+" В\nНорма");
        }
        else // напряжение меньше (не норма)
        {
            Log("Напряжение цепи "+battery[iBatteryIndex].circuitgroup[i-1]+" = "+QString::number(fU, 'f', 2)+" В. Не норма!", "red");
            label->setText(tr("%1) %2 В. Не норма!").arg(i).arg(QString::number(fU, 'f', 2)));
            // если ручной режим, то выдать окно сообщения, и только потом разобрать режим измерения. !!! под нагрузкой???
            if(bModeManual) QMessageBox::information(this, tr("Напряжение замкнутой цепи группы"), tr("Напряжение цепи ")+battery[iBatteryIndex].circuitgroup[i-1]+" = "+QString::number(fU, 'f', 2)+" В\nНе норма!");
            // !!! добавить цепь в список распассивируемых
            //int rett = QMessageBox::question(this, "Внимание - "+ui->rbClosedCircuitVoltageGroup->text(), tr("%1 \nпродолжить?").arg(str), tr("Да"), tr("Да, необходима \"Распассивация\""), tr("Нет"));
            switch (QMessageBox::question(this, "Внимание - "+ui->rbClosedCircuitVoltageGroup->text(), tr("%1 В. Не норма! \nПродолжить?").arg(str), tr("Да"), tr("Да, необходима \"Распассивация\""), tr("Нет"))) {
            case 0:
                break;
            case 1:
                //imDepassivation.append(iStepClosedCircuitVoltageGroup-1);
                //ui->cbDepassivation->addItem(battery[iBatteryIndex].circuitgroup[iStepClosedCircuitVoltageGroup-1]);
                // !!! Log(tr("%1) %1 - Х4 «4» добавлен для распассивации.").arg(iStepClosedCircuitVoltageGroup-1), "blue");
                battery[iBatteryIndex].b_flag_circuit[i-1] |= CIRCUIT_DEPASS; // добавить признак, что группе нужна депассивация
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
            ui->rbModeDiagnosticManual->setChecked(true);
            ui->rbModeDiagnosticAuto->setEnabled(false);
        }

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

    // добавить цепи в комбобокс распассивации
    modelDepassivation = new QStandardItemModel(battery[iBatteryIndex].group_num, 1);
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
    }
    ui->cbDepassivation->setModel(modelDepassivation);
    ui->cbDepassivation->setItemData(0, "DISABLE", Qt::UserRole-1);
    ui->cbDepassivation->setItemText(0, tr("Выбрано: %0 из %1").arg(battery[iBatteryIndex].group_num).arg(battery[iBatteryIndex].group_num));


    timerPing->start(delay_timerPing); // запустить пинг по выходу из режима
    baSendArray.clear(); // очистить буфера команд.
    baSendCommand.clear();
    baRecvArray.clear();

    Log(tr("Проверка завершена - %1").arg(ui->rbClosedCircuitVoltageGroup->text()), "blue");
    //iStepClosedCircuitVoltageGroup = 1;
    ui->rbClosedCircuitVoltageBattery->setEnabled(true);
    ui->groupBoxCOMPort->setEnabled(true);
    ui->groupBoxDiagnosticDevice->setEnabled(true);
    ui->groupBoxDiagnosticMode->setEnabled(true);
}

// слот вызывается при изменении чекбоксов элементов списка комбобокса
void MainWindow::itemChangedClosedCircuitVoltageGroup(QStandardItem* itm)
{
    qDebug() << "modelClosedCircuitVoltageGroup->rowCount()=" << modelClosedCircuitVoltageGroup->rowCount();
    int count = 0;
    for(int i=1; i < modelClosedCircuitVoltageGroup->rowCount(); i++)
    {
        QStandardItem *sitm = modelClosedCircuitVoltageGroup->item(i, 0);
        Qt::CheckState checkState = sitm->checkState();
        if (checkState == Qt::Checked)
            count++;
    }
    qDebug() << "countClosedCircuitVoltageGroup=" << count;
    ui->cbClosedCircuitVoltageGroup->setItemText(0, tr("Выбрано: %0 из %1").arg(count).arg(modelClosedCircuitVoltageGroup->rowCount()-1));
    ui->cbClosedCircuitVoltageGroup->setCurrentIndex(0);
}

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
/*
 * Напряжение замкнутой цепи группы
 */
void MainWindow::checkClosedCircuitVoltageGroup()
{
    qDebug()<<"checkClosedCircuitVoltageGroup";
    int h = 45; //Шаг, с которым будем пробегать по оси Ox
    //double x, y;
    ui->widgetClosedCircuitVoltageGroup->addGraph(); // blue line
    ui->widgetClosedCircuitVoltageGroup->graph(0)->setPen(QPen(Qt::blue));
    ui->widgetClosedCircuitVoltageGroup->graph(0)->clearData();
    ui->widgetClosedCircuitVoltageGroup->addGraph(); // blue dot
    ui->widgetClosedCircuitVoltageGroup->graph(1)->clearData();
    ui->widgetClosedCircuitVoltageGroup->graph(1)->setLineStyle(QCPGraph::lsNone);
    //ui->widgetClosedCircuitVoltageGroup->graph(1)->setPen(QPen(Qt::green));
    ui->widgetClosedCircuitVoltageGroup->graph(1)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, Qt::blue, Qt::white, 7));
    ui->widgetClosedCircuitVoltageGroup->addGraph(); // red line
    ui->widgetClosedCircuitVoltageGroup->graph(2)->setPen(QPen(Qt::red));
    ui->widgetClosedCircuitVoltageGroup->graph(2)->setBrush(QBrush(QColor(255, 0, 0, 20)));
    ui->widgetClosedCircuitVoltageGroup->graph(2)->clearData();
    ui->widgetClosedCircuitVoltageGroup->graph(2)->addData(0, settings.closecircuitgroup_limit);
    ui->widgetClosedCircuitVoltageGroup->graph(2)->addData(900, settings.closecircuitgroup_limit);

    ui->widgetClosedCircuitVoltageGroup->xAxis->setLabel(tr("Время, c"));
    ui->widgetClosedCircuitVoltageGroup->xAxis->setRange(0, 900);
    ui->widgetClosedCircuitVoltageGroup->yAxis->setLabel(tr("Напряжение, В"));
    ui->widgetClosedCircuitVoltageGroup->yAxis->setRange(20, 40);

    if (((QPushButton*)sender())->objectName() == "btnClosedCircuitVoltageGroup") {
        iStepClosedCircuitVoltageGroup = 1;
        bState = false;
        //ui->btnClosedCircuitVoltageGroup_2->setEnabled(false);
    }
    if (((QPushButton*)sender())->objectName() == "btnClosedCircuitVoltageGroup_2")
        bState = false;
    if (!bState) return; !!! смысл переменой?
    ui->groupBoxCOMPort->setEnabled(false);
    ui->groupBoxDiagnosticDevice->setEnabled(false);
    ui->groupBoxDiagnosticMode->setEnabled(false);
    ui->tabWidget->addTab(ui->tabClosedCircuitVoltageGroup, ui->rbClosedCircuitVoltageGroup->text());
    ui->tabWidget->setCurrentIndex(ui->tabWidget->count()-1);
    Log(tr("Проверка начата - %1").arg(ui->rbClosedCircuitVoltageGroup->text()), "blue");
    switch (iBatteryIndex) {
    case 0: //9ER20P-20
        ui->progressBar->setValue(iStepClosedCircuitVoltageGroup-1);
        ui->progressBar->setMaximum(20);
        while (iStepClosedCircuitVoltageGroup <= 20) {
            if (!bState) return;
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
            ui->widgetClosedCircuitVoltageGroup->graph(0)->rescaleValueAxis(true);
            ui->widgetClosedCircuitVoltageGroup->graph(0)->addData(h*(iStepClosedCircuitVoltageGroup-1), param);
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
       !!! раскрутить             ui->cbDepassivation->addItem(battery[iBatteryIndex].circuitgroup[iStepClosedCircuitVoltageGroup-1]);
                    Log(tr("%1) %1 - Х4 «4» добавлен для распассивации.").arg(iStepClosedCircuitVoltageGroup-1), "blue");
                    break;
                case 2:
                    //ui->btnClosedCircuitVoltageGroup_2->setEnabled(true);
                    bState = true;
                    return;
                    break;
                default:
                    break;
                }
                ui->rbModeDiagnosticManual->setChecked(true);
                ui->rbModeDiagnosticAuto->setEnabled(false);
                //ui->rbInsulationResistance->setChecked(true);
            }

            iStepClosedCircuitVoltageGroup++;
        }
        if (imDepassivation.count() != 0)
            ui->rbDepassivation->setEnabled(true);
        //ui->btnClosedCircuitVoltageGroup_2->setEnabled(false);
        if (ui->rbModeDiagnosticAuto->isChecked())
            bCheckCompleteClosedCircuitVoltageGroup = true;
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
    Log(tr("Проверка завершена - %1").arg(ui->rbClosedCircuitVoltageGroup->text()), "blue");
    iStepClosedCircuitVoltageGroup = 1;
    ui->rbClosedCircuitVoltageBattery->setEnabled(true);
    ui->groupBoxCOMPort->setEnabled(true);
    ui->groupBoxDiagnosticDevice->setEnabled(true);
    ui->groupBoxDiagnosticMode->setEnabled(true);
}

#endif
