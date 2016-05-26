#include <QDebug>
#include <QMessageBox>
#include <QDateTime>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "battery.h"
#include "settings.h"

extern Settings settings;

extern QVector<Battery> battery;

// Нажата кнопка проверки напряжения замкнутой цепи батареи
void MainWindow::on_btnClosedCircuitVoltageBattery_clicked()
{
    quint16 codeADC=0; // принятый код АЦП
    float fU=0; // принятое напряжение в вольтах
    // код порогового напряжения = пороговое напряжение В / коэфф. (вес разряда) + смещение (в коде)
    //quint16 codeLimit=settings.closecircuitbattery_limit/settings.coefADC1[settings.board_counter] + settings.offsetADC1[settings.board_counter]; // код, пороговое напряжение.
    int ret=0; // код возврата ошибки
    QDateTime starttime; // время начала измерения
    QDateTime dt; // текущее время очередного измерения
    double x; // текущая координата Х
    bool bFirstPoll=true; // первое измерение

    // Подготовка графика !!! перетащить в mainwindow
    /*ui->widgetClosedCircuitBattery->addGraph(); // blue line
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
    ui->widgetClosedCircuitBattery->graph(2)->addData(settings.time_closecircuitbattery+2, settings.closecircuitbattery_limit);

    ui->widgetClosedCircuitBattery->xAxis->setLabel(tr("Время, c"));
    ui->widgetClosedCircuitBattery->xAxis->setRange(0, settings.time_closecircuitbattery+2);
    ui->widgetClosedCircuitBattery->yAxis->setLabel(tr("Напряжение, В"));
    ui->widgetClosedCircuitBattery->yAxis->setRange(24, 33);*/
    // показать закладку на экране
    //ui->tabWidget->addTab(ui->tabClosedCircuitVoltageBattery, ui->rbClosedCircuitVoltageBattery->text());
    //ui->tabWidget->setCurrentIndex(ui->tabWidget->count()-1);

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

    if (ui->rbModeDiagnosticManual->isChecked()) {  /// если в ручной режиме
        setGUI(false);                              ///  отключаем интерфейс
    } else {                                        /// если в автоматическом режиме
        ui->cbParamsAutoMode->setCurrentIndex(5);   ///  переключаем режим комбокса на наш
    }
    ui->progressBar->setMaximum(2); /// установим максимум прогресс бара
    // откроем вкладку
    ui->tabWidget->addTab(ui->tabClosedCircuitVoltageBattery, ui->rbClosedCircuitVoltageBattery->text());
    ui->tabWidget->setCurrentIndex(ui->tabWidget->count()-1);
    Log(tr("Проверка начата - %1").arg(ui->rbClosedCircuitVoltageBattery->text()), "blue");
    ui->statusBar->showMessage(tr("Проверка ")+ui->rbClosedCircuitVoltageBattery->text()+" ...");

    if(ui->rbModeDiagnosticAuto->isChecked()) /// если в автоматическом режиме
    {
        /// если все цепи меньше нормы, или не проверялись - в автомате батарею под нагрузкой не проверять
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
            QMessageBox::information(this, "Внимание!", "Все цепи меньше нормы или не проверялись под нагрузкой.\nПроверка батареи под нагрузкой запрещена.");
            //bState = false; /// выходим из режима проверки
            goto stop;
        }
    }

    /// очистить буфера
    baSendArray.clear();
    baSendCommand.clear();
    baRecvArray.clear();
    ui->progressBar->setValue(0); /// установим прогресс бар в начальное положение

    /// при наличии галки имитатора, выводим сообщение о необходимости включить источник питания
    if(ui->cbIsImitator->isChecked() and iPowerState != 1) {
        QMessageBox::information(this, tr("Внимание! - %0").arg(ui->rbClosedCircuitVoltageBattery->text()), tr("Перед проверкой необходимо включить источник питания!"));
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
            .arg(ui->rbClosedCircuitVoltageBattery->text())
            .arg((ui->rbModeDiagnosticAuto->isChecked()) ? "Автоматический режим" : "Ручной режим")
            .arg((ui->cbIsImitator->isChecked()) ? settings.closecircuitbattery_imitator_limit : settings.closecircuitbattery_limit);

    /// формируем строку и пишем на label "идет измерение..."
    sLabelText = tr("1) \"%0\"").arg(battery[iBatteryIndex].circuitbattery);
    ui->labelClosedCircuitVoltageBattery0->setText(sLabelText + " идет измерение...");
    ui->labelClosedCircuitVoltageBattery0->setStyleSheet("QLabel { color : blue; }");

    // сбросить коробочку
    baSendArray = (baSendCommand="IDLE")+"#"; // подготовить буфер для передачи
    timerSend->start(settings.delay_after_request_before_next_ADC1); // послать baSendArray в порт
    // ждём ответа. по сигналу о готовности принятых данных или по таймауту, вывалимся из цикла
    ret=loop.exec();
    if(ret) goto stop; // если не ноль (ошибка таймаута) - вывалиться из режима. если 0, то приняли данные из порта

    // собрать режим
    baSendArray=(baSendCommand="UccB")+"#";
    if(bDeveloperState) Log(QString("Sending ") + qPrintable(baSendArray), "blue");
    timerSend->start(settings.delay_after_IDLE_before_other);
    ret=loop.exec();
    if(ret) goto stop;
    ui->progressBar->setValue(ui->progressBar->value()+1);

    starttime = QDateTime::currentDateTime(); // время начала измерения
    dt = QDateTime::currentDateTime(); // текущее время
    ui->widgetClosedCircuitBattery->graph(0)->clearData(); // очистить график

    bFirstPoll=true;// после сбора режима первый опрос

    while(-dt.msecsTo(starttime) < settings.time_closecircuitbattery*1000) // пока время цикла проверки не вышло, продолжим измерять
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
        ui->widgetClosedCircuitBattery->graph(0)->rescaleValueAxis(true); // для автоматического перерисовывания шкалы графика, если значения за пределами экрана
        ui->widgetClosedCircuitBattery->graph(0)->addData((double)x/1000, (double)fU);
        ui->widgetClosedCircuitBattery->replot();
    }

    ui->progressBar->setValue(ui->progressBar->value()+1);

    dArrayClosedCircuitVoltageBattery[0] = fU + settings.closecircuitbattery_loss; /// добавляем к результату потери на кабеле

    if(bDeveloperState)
        Log("Цепь "+battery[iBatteryIndex].circuitbattery+" Receive "+qPrintable(baRecvArray)+" codeADC1=0x"+QString("%1").arg((ushort)codeADC, 0, 16), "blue");

    // напечатать рез-т в закладку и в журнал
    if (dArrayClosedCircuitVoltageBattery[0] < ((ui->cbIsImitator->isChecked()) ? settings.closecircuitbattery_imitator_limit : settings.closecircuitbattery_limit)) {
        sResult = "Не норма!";
        color = "red";
    }
    else {
        sResult = "Норма";
        color = "green";
    }
    ui->labelClosedCircuitVoltageBattery0->setText(tr("%0 = <b>%1</b> В. %2").arg(sLabelText).arg(dArrayClosedCircuitVoltageBattery[0], 0, 'f', 2).arg(sResult));
    ui->labelClosedCircuitVoltageBattery0->setStyleSheet("QLabel { color : "+color+"; }");
    Log(tr("%0 = <b>%1</b> В. %2").arg(sLabelText).arg(dArrayClosedCircuitVoltageBattery[0], 0, 'f', 2).arg(sResult), color);

    /// заполняем массив проверок для отчета
    dateTime = QDateTime::currentDateTime();
    sHtml += tr("<tr>"\
                "    <td><p>&nbsp;%0&nbsp;</td>"\
                "    <td><p>&nbsp;%1&nbsp;</td>"\
                "    <td><p>&nbsp;%2&nbsp;</td>"\
                "    <td><p>&nbsp;%3&nbsp;</td>"\
                "</tr>")
                .arg(dateTime.toString("hh:mm:ss"))
                .arg(battery[iBatteryIndex].circuitbattery)
                .arg(dArrayClosedCircuitVoltageBattery[0], 0, 'f', 2)
                .arg(sResult);

    /// добавим в массив графиков полученный график ВРЕМЕННО СКРЫТ
    /*ui->widgetClosedCircuitBattery->savePng(QDir::tempPath()+"ClosedCircuitBatteryGraph.png",  699, 504, 1.0, -1);
    img.load(QDir::tempPath()+"ClosedCircuitBatteryGraph.png");
    imgArrayReportGraph.append(img);
    sArrayReportGraphDescription.append(tr("График. %0. Цепь: \"%1\". Время: %2.").arg(ui->rbClosedCircuitVoltageBattery->text()).arg(battery[iBatteryIndex].circuitbattery).arg(dateTime.toString("hh:mm:ss")));*/

    /// проанализировать результаты ПО ТЗ НЕ НУЖНО ПРЕКРАЩАТЬ РЕЖИМ!
    /*if(codeADC < codeLimit) /// напряжение меньше (не норма)
    {
        bState = false;
    }*/

stop:
    if(ret == KDS_STOP) {
        ui->labelClosedCircuitVoltageBattery0->setText(sLabelText + " измерение прервано!");
        ui->labelClosedCircuitVoltageBattery0->setStyleSheet("QLabel { color : red; }");
        Log(sLabelText + " измерение прервано!", "red");
        sHtml += tr("<tr><td>&nbsp;%0&nbsp;</td><td>&nbsp;%1&nbsp;</td><td colspan=\"2\"><p>&nbsp;Измерение прервано!&nbsp;</td></tr>")
                .arg(dateTime.toString("hh:mm:ss"))
                .arg(battery[iBatteryIndex].circuitbattery);
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

    Log(tr("Проверка завершена - %1").arg(ui->rbClosedCircuitVoltageBattery->text()), "blue");

    timerPing->start(delay_timerPing); // запустить пинг по выходу из режима
    baSendArray.clear(); // очистить буфера команд.
    baSendCommand.clear();
    baRecvArray.clear();
    ui->progressBar->reset(); /// сбросим прогресс бар
}
