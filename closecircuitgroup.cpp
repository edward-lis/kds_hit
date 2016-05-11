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
    quint16 codeLimit=settings.closecircuitgroup_limit/settings.coefADC1[settings.board_counter] + settings.offsetADC1[settings.board_counter]; // код, пороговое напряжение.
    int ret=0; // код возврата ошибки
    QString str_num; // номер цепи
    QDateTime starttime; // время начала измерения
    QDateTime dt; // текущее время очередного измерения
    double x; // текущая координата Х
    int cycleTimeSec=settings.time_closecircuitgroup; // длительность цикла проверки в секундах
    bool bFirstPoll=true; // первое измерение
    int i=0; // номер цепи

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
    }

    if(loop.isRunning()){qDebug()<<"loop.isRunning()!"; return;} // костыль: если цикл уже работает - выйти обратно
    timerPing->stop(); // остановить пинг
    bCheckInProgress = true; // вошли в состояние проверки

    if (ui->rbModeDiagnosticManual->isChecked()) {  /// если в ручной режиме
        setGUI(false);                              ///  отключаем интерфейс
    } else {                                        /// если в автоматическом режиме
        ui->cbParamsAutoMode->setCurrentIndex(4);   ///  переключаем режим комбокса на наш
    }

    // откроем вкладку
    ui->tabWidget->addTab(ui->tabClosedCircuitVoltageGroup, ui->rbClosedCircuitVoltageGroup->text());
    ui->tabWidget->setCurrentIndex(ui->tabWidget->count()-1);
    Log(tr("Проверка начата - %1").arg(ui->rbClosedCircuitVoltageGroup->text()), "blue");
    ui->statusBar->showMessage(tr("Проверка ")+ui->rbClosedCircuitVoltageGroup->text()+" ...");

    if(bModeManual)// если в ручном режиме
    {
        //i=ui->cbOpenCircuitVoltageGroup->currentIndex();
        iCurrentStep=0; // в ручном начнём сначала
        iMaxSteps=modelClosedCircuitVoltageGroup->rowCount()-1; // -1 с учётом первой строки в комбобоксе
    }
    else
    {
        iCurrentStep = ui->cbSubParamsAutoMode->currentIndex();
        iMaxSteps = ui->cbSubParamsAutoMode->count();
    }

    // написать про группы, в зависимости от признаков и флагов.
    for(int i = 0; i < 28; i++)
    {
        dArrayClosedCircuitVoltageGroup[i] = -1;
        label = findChild<QLabel*>(tr("labelClosedCircuitVoltageGroup%1").arg(i));
        label->setStyleSheet("QLabel { color : black; }");
        label->clear();
        if (i < battery[iBatteryIndex].group_num) {
            if(!(battery[iBatteryIndex].b_flag_circuit[i] & CIRCUIT_OCG_TESTED))
            {
                label->setText(tr("%0) НРЦг не проверялось.").arg(i+1));
            }
            else if(battery[iBatteryIndex].b_flag_circuit[i] & CIRCUIT_FAULT)
            {
                label->setText(tr("%0) НРЦг <нормы, проверка под нагрузкой запрещена.").arg(i+1));
                label->setStyleSheet("QLabel { color : red; }");
            }
            else
            {
                label->setText(tr("%0) \"%1\" не измерялось.").arg(i+1).arg(battery[iBatteryIndex].circuitgroup[i]));
            }
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
        //bState = false;
        goto stop;
    }

    /// при наличии галки имитатора, выводим сообщение о необходимости включить источник питания
    if(ui->cbIsImitator->isChecked() and iPowerState != 1) {
        QMessageBox::information(this, tr("Внимание! - %0").arg(ui->rbClosedCircuitVoltageGroup->text()), tr("Перед проверкой необходимо включить источник питания!"));
        iPowerState = 1; /// состояние включенного источника питания
    }

    /// таблица - верх
    sHtml = tr("<table border=\"1\" width=\"100%\" cellpadding=\"3\" cellspacing=\"0\" bordercolor=\"black\">"\
               "    <tbody>"\
               "        <tr>"\
               "            <td colspan=\"4\">&nbsp;<strong>%0(%1)&nbsp;</strong><br/><em>&nbsp;Предельные значения: не менее %2 В</em></td>"\
               "        </tr>"\
               "        <tr>"\
               "            <td width=\"11%\">"\
               "                <p>&nbsp;<b>Время</b>&nbsp;</p>"\
               "            </td>"\
               "            <td width=\"57%\">"\
               "                <p>&nbsp;<b>Цепь</b>&nbsp;</p>"\
               "            </td>"\
               "            <td width=\"15%\">"\
               "                <p>&nbsp;<b>Значение</b>&nbsp;</p>"\
               "            </td>"\
               "            <td width=\"17%\">"\
               "                <p>&nbsp;<b>Результат</b>&nbsp;</p>"\
               "            </td>"\
               "        </tr>")
            .arg(ui->rbClosedCircuitVoltageGroup->text())
            .arg((ui->rbModeDiagnosticAuto->isChecked()) ? "Автоматический режим" : "Ручной режим")
            .arg(settings.closecircuitgroup_limit);

    // Пробежимся по списку цепей
    for(i = iCurrentStep; i < iMaxSteps; i++)
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
            fU = ((codeADC-settings.offsetADC1[settings.board_counter])*settings.coefADC1[settings.board_counter]); // напряжение в вольтах
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

        /// заполняем массив проверок для отчета
        dateTime = QDateTime::currentDateTime();
        sHtml += tr("<tr>"\
                    "    <td><p>&nbsp;%0&nbsp;</td>"\
                    "    <td><p>&nbsp;%1&nbsp;</td>"\
                    "    <td><p>&nbsp;%2&nbsp;</td>"\
                    "    <td><p>&nbsp;%3&nbsp;</td>"\
                    "</tr>")
                    .arg(dateTime.toString("hh:mm:ss"))
                    .arg(battery[iBatteryIndex].circuitgroup[i])
                    .arg(dArrayClosedCircuitVoltageGroup[i], 0, 'f', 2)
                    .arg(sResult);

        /// добавим в массив графиков полученный график ВРЕМЕННО СКРЫТ
        /*ui->widgetClosedCircuitVoltageGroup->savePng(QDir::tempPath()+"ClosedCircuitVoltageGroupGraph.png",  699, 606, 1.0, -1 );
        img.load(QDir::tempPath()+"ClosedCircuitVoltageGroupGraph.png");
        imgArrayReportGraph.append(img);
        sArrayReportGraphDescription.append(tr("График. %0. Цепь: \"%1\". Время: %2.").arg(ui->rbClosedCircuitVoltageGroup->text()).arg(battery[iBatteryIndex].circuitgroup[i]).arg(dateTime.toString("hh:mm:ss")));*/

        /// только для ручного режима, снимаем галку с провереной
        if(bModeManual) { /// ручной режим
            item = new QStandardItem(QString("%0").arg(battery[iBatteryIndex].circuitgroup[i]));
            item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
            item->setData(Qt::Unchecked, Qt::CheckStateRole);
            modelClosedCircuitVoltageGroup->setItem(i+1, 0, item);
        } else { /// автоматический режим
            ui->cbSubParamsAutoMode->setCurrentIndex(ui->cbSubParamsAutoMode->currentIndex()+1);
        }

        // по окончанию цикла снять нагрузку, разобрать режим (!!! даже в ручном режиме)
        baSendArray = (baSendCommand="IDLE")+"#";
        timerSend->start(settings.delay_after_request_before_next_ADC1);
        ret=loop.exec();
        if(ret) goto stop;

        /// проанализировать результаты, в кодах
        if(codeADC < codeLimit) { /// напряжение меньше (не норма)
            /// добавить цепь в список распассивируемых
            switch (QMessageBox::question(this, tr("Внимание - %1").arg(ui->rbClosedCircuitVoltageGroup->text()), tr("%0 = %1 В. %2 Продолжить?").arg(sLabelText).arg(dArrayClosedCircuitVoltageGroup[i], 0, 'f', 2).arg(sResult), tr("Да"), tr("Да, необходима \"Распассивация\""), tr("Нет"))) {
            case 0: /// Да - продолжаем проверку
                break;
            case 1:
                //imDepassivation.append(iStepClosedCircuitVoltageGroup-1);
                label->setText("*" + label->text());
                battery[iBatteryIndex].b_flag_circuit[i] |= CIRCUIT_DEPASS; /// добавить признак, что группе нужна депассивация
                break;
            default: /// Нет - прекратить проверку(Все значения кроме 0 и 1)
                bState = false; /// выходим из режима проверки
                goto stop;
                break;
            }
        }
    }// конец цикла проверок цепей

stop:
    if(ret == KDS_STOP) {
        label->setText(sLabelText + " измерение прервано!");
        label->setStyleSheet("QLabel { color : red; }");
        Log(sLabelText + " измерение прервано!", "red");
        sHtml += tr("<tr><td>&nbsp;%0&nbsp;</td><td>&nbsp;%1&nbsp;</td><td colspan=\"2\"><p>&nbsp;Измерение прервано!&nbsp;</td></tr>")
                .arg(dateTime.toString("hh:mm:ss"))
                .arg(battery[iBatteryIndex].circuitgroup[i]);
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

    if (ui->rbModeDiagnosticManual->isChecked()) { /// если в ручной режиме
        setGUI(true); /// включаем интерфейс
    }

    /// таблица - низ
    sHtml +="   </tbody>"\
            "</table>"\
            "<br/>";
    sArrayReport.append(sHtml); /// добавляем таблицу в массив проверок

    Log(tr("Проверка завершена - %1").arg(ui->rbClosedCircuitVoltageGroup->text()), "blue");

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

        /*if(dArrayClosedCircuitVoltageGroup[r] < settings.closecircuitgroup_limit) // если какая-либо цепь была меньше нормы
        {
            ui->rbModeDiagnosticManual->setChecked(true); // переключить в ручной принудительно
            bState = false;
        }*/
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
