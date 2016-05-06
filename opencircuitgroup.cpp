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
    quint16 codeLimit=settings.opencircuitgroup_limit_min/settings.coefADC1[settings.board_counter] + settings.offsetADC1[settings.board_counter]; // код, пороговое напряжение.
    int ret=0; // код возврата ошибки
    QString str_num; // номер цепи
    int i=0; // номер цепи

    if(bCheckInProgress) { // если зашли в эту ф-ию по нажатию кнопки btnVoltageOnTheHousing ("Стоп"), будучи уже в состоянии проверки, значит стоп режима
        // остановить текущую проверку, выход
        bCheckInProgress = false;
        timerSend->stop(); // остановить посылку очередной команды в порт
        timeoutResponse->stop(); // остановить предыдущий таймаут (если был, конечно)
        qDebug()<<"loop.isRunning()"<<loop.isRunning();
        if(loop.isRunning()) {
            loop.exit(KDS_STOP); // прекратить цикл ожидания посылки/ожидания ответа от коробочки
        }
        return;
    } else {
        /// по началу проверки очистим все label'ы и полученные результаты
        for (int i = 0; i < 28; i++) {
            dArrayOpenCircuitVoltageGroup[i] = -1;
            label = findChild<QLabel*>(tr("labelOpenCircuitVoltageGroup%0").arg(i));
            label->setStyleSheet("QLabel { color : black; }");
            label->clear();
            if (i < battery[iBatteryIndex].group_num)
                label->setText(tr("%0) \"%1\" не измерялось.").arg(i+1).arg(battery[iBatteryIndex].circuitgroup[i]));
        }
    }

    if(loop.isRunning()){qDebug()<<"loop.isRunning()!"; return;} // костыль: если цикл уже работает - выйти обратно
    timerPing->stop(); // остановить пинг
    bCheckInProgress = true; // вошли в состояние проверки

    if (ui->rbModeDiagnosticManual->isChecked()) {  /// если в ручной режиме
        setGUI(false);                              ///  отключаем интерфейс
    } else {                                        /// если в автоматическом режиме
        ui->cbParamsAutoMode->setCurrentIndex(2);   ///  переключаем режим комбокса на наш
    }

    // откроем вкладку
    ui->tabWidget->addTab(ui->tabOpenCircuitVoltageGroup, ui->rbOpenCircuitVoltageGroup->text());
    ui->tabWidget->setCurrentIndex(ui->tabWidget->count()-1);
    Log(tr("Проверка начата - %1").arg(ui->rbOpenCircuitVoltageGroup->text()), "blue");
    ui->statusBar->showMessage(tr("Проверка ")+ui->rbOpenCircuitVoltageGroup->text()+" ...");

    if(bModeManual) { // если в ручном режиме
        //i=ui->cbOpenCircuitVoltageGroup->currentIndex();
        iCurrentStep=0; // в ручном начнём сначала
        iMaxSteps=modelOpenCircuitVoltageGroup->rowCount()-1; // -1 с учётом первой строки в комбобоксе
    } else {
        iCurrentStep = ui->cbSubParamsAutoMode->currentIndex();
        iMaxSteps = ui->cbSubParamsAutoMode->count();
    }

    /// при наличии галки имитатора, выводим сообщение о необходимости включить источник питания
    if(ui->cbIsImitator->isChecked() and iPowerState != 1) {
        QMessageBox::information(this, tr("Внимание! - %0").arg(ui->rbOpenCircuitVoltageGroup->text()), tr("Перед проверкой необходимо включить источник питания!"));
        iPowerState = 1; /// состояние включенного источника питания
    }

    /// таблица - верх
    sHtml = tr("<table border=\"1\" width=\"100%\" cellpadding=\"3\" cellspacing=\"0\" bordercolor=\"black\">"\
               "    <tbody>"\
               "        <tr>"\
               "            <td colspan=\"4\">&nbsp;<strong>%0(%1)&nbsp;</strong><br/><em>&nbsp;Предельные значения: не менее %2 и не более %3 В</em></td>"\
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
            .arg(ui->rbOpenCircuitVoltageGroup->text())
            .arg((ui->rbModeDiagnosticAuto->isChecked()) ? "Автоматический режим" : "Ручной режим")
            .arg(settings.opencircuitgroup_limit_min)
            .arg(settings.opencircuitgroup_limit_max);

    // Пробежимся по списку цепей
    for(i = iCurrentStep; i < iMaxSteps; i++) {
        if(bModeManual) { // в ручном будем идти по чекбоксам
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

        /// формируем строку и пишем на label "идет измерение..."
        sLabelText = tr("%0) \"%1\"").arg(i+1).arg(battery[iBatteryIndex].circuitgroup[i]);
        label = findChild<QLabel*>(tr("labelOpenCircuitVoltageGroup%0").arg(i));
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

        fU = ((codeADC-settings.offsetADC1[settings.board_counter])*settings.coefADC1[settings.board_counter]); // напряжение в вольтах
        dArrayOpenCircuitVoltageGroup[i] = fU;

        battery[iBatteryIndex].b_flag_circuit[i] |= CIRCUIT_OCG_TESTED; // установить флаг - цепь проверялась

        if(bDeveloperState)
            Log("Цепь "+battery[iBatteryIndex].circuitgroup[i]+" Receive "+qPrintable(baRecvArray)+" codeADC1=0x"+QString("%1").arg((ushort)codeADC, 0, 16), "blue");

        // напечатать рез-т в закладку и в журнал
        if (dArrayOpenCircuitVoltageGroup[i] < settings.opencircuitgroup_limit_min) {
            sResult = "Не норма!";
            color = "red";
            //iFlagsCircuitGroup[i] = -1; /// устанавливаем флаг проверки запрещена
        } else {
            sResult = "Норма";
            color = "green";
            //iFlagsCircuitGroup[i] = 0; /// устанавливаем флаг нормальной выполненой проверки
        }
        label->setText(tr("%0 = <b>%1</b> В. %2").arg(sLabelText).arg(dArrayOpenCircuitVoltageGroup[i], 0, 'f', 2).arg(sResult));
        label->setStyleSheet("QLabel { color : "+color+"; }");
        Log(tr("%0 = <b>%1</b> В. %2").arg(sLabelText).arg(dArrayOpenCircuitVoltageGroup[i], 0, 'f', 2).arg(sResult), color);

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
                    .arg(dArrayOpenCircuitVoltageGroup[i], 0, 'f', 2)
                    .arg(sResult);

        if(bModeManual) { /// ручной режим
            /// снимаем галку с провереной
            item = new QStandardItem(QString("%0").arg(battery[iBatteryIndex].circuitgroup[i]));
            item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
            item->setData(Qt::Unchecked, Qt::CheckStateRole);
            modelOpenCircuitVoltageGroup->setItem(i+1, 0, item);
        } else { /// автоматический режим
            /// в автоматическом режиме пролистываем комбокс подпараметров проверки
            ui->cbSubParamsAutoMode->setCurrentIndex(ui->cbSubParamsAutoMode->currentIndex()+1);
        }

        /// проанализировать результаты
        if(codeADC >= codeLimit) { /// напряжение больше (норма)
            /// добавить цепь в список исправных
            battery[iBatteryIndex].b_flag_circuit[i] &= ~CIRCUIT_FAULT; /// снять флаг - цепь неисправна
        } else { /// напряжение меньше (не норма)
            /// установить флаг - цепь неисправна, запрет проверки цепи под нагрузкой
            battery[iBatteryIndex].b_flag_circuit[i] |= CIRCUIT_FAULT;

            if (!bModeManual and QMessageBox::question(this, tr("Внимание - %0").arg(ui->rbOpenCircuitVoltageGroup->text()), tr("%0 = %1 В. %2 Продолжить?").arg(sLabelText).arg(dArrayOpenCircuitVoltageGroup[i], 0, 'f', 2).arg(sResult), tr("Да"), tr("Нет"))) {
                bState = false; /// выходим из режима проверки
                break;
            }
        }
    }//for

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

    Log(tr("Проверка завершена - %1").arg(ui->rbOpenCircuitVoltageGroup->text()), "blue");

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
    QStandardItem *sitm;

    /// Выбрать все/Отменить все
    /*if (itm->row() == 1) {
        sitm = modelOpenCircuitVoltageGroup->item(1, 0);
        Qt::CheckState checkState = sitm->checkState();
        for(int i=2; i < modelOpenCircuitVoltageGroup->rowCount(); i++)
        {
            sitm = modelOpenCircuitVoltageGroup->item(i, 0);
            sitm->setCheckState(checkState);
        }
        if (checkState == Qt::Checked) {
            ui->cbOpenCircuitVoltageGroup->setItemText(0, tr("Выбрано: %0 из %1").arg(modelOpenCircuitVoltageGroup->rowCount()-2).arg(modelOpenCircuitVoltageGroup->rowCount()-2));
            itm->setText("Отменить все");
        } else {
            ui->cbOpenCircuitVoltageGroup->setItemText(0, tr("Выбрано: 0 из %0").arg(modelOpenCircuitVoltageGroup->rowCount()-2));
            itm->setText("Выбрать все");
        }
        return;
    }*/

    for(int i=1; i < modelOpenCircuitVoltageGroup->rowCount(); i++)
    {
        sitm = modelOpenCircuitVoltageGroup->item(i, 0);
        Qt::CheckState checkState = sitm->checkState();
        if (checkState == Qt::Checked)
            count++;
    }

    ui->cbOpenCircuitVoltageGroup->setItemText(0, tr("Выбрано: %0 из %1").arg(count).arg(modelOpenCircuitVoltageGroup->rowCount()-1));
    ui->cbOpenCircuitVoltageGroup->setCurrentIndex(0);
}
