#include <QDebug>
#include <QMessageBox>
#include <QDateTime>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "battery.h"
#include "settings.h"

extern Settings settings;

extern QVector<Battery> battery;


// Нажата кнопка проверки напряжения замкнутой цепи БП УУТББ
void MainWindow::on_btnClosedCircuitVoltagePowerSupply_clicked()
{
    /// если НРЦбп меньше нормы то запрещаем проверку НЗЦбп
    if (dArrayOpenCircuitVoltagePowerSupply[0] < settings.uutbb_opencircuitpower_limit_min) {
        if (dArrayOpenCircuitVoltagePowerSupply[0] == -1) { /// если не проверялось
            QMessageBox::information(this, tr("Внимание - %0").arg(ui->rbClosedCircuitVoltagePowerSupply->text()), tr("%0 не проверялось. Проверка под нагрузкой запрещена.")
                .arg(ui->rbOpenCircuitVoltagePowerSupply->text()));
        } else {
            QMessageBox::information(this, tr("Внимание - %0").arg(ui->rbClosedCircuitVoltagePowerSupply->text()), tr("%0 - Не норма! %1 В. Проверка под нагрузкой запрещена.")
                .arg(ui->rbOpenCircuitVoltagePowerSupply->text())
                .arg(dArrayOpenCircuitVoltagePowerSupply[0], 0, 'f', 2));
        }
        bState = false; /// выходим из режима проверки
        return; /// дальше не идем
    }

    quint16 codeADC=0; // принятый код АЦП
    float fU=0; // принятое напряжение в вольтах
    // код порогового напряжения = пороговое напряжение В / коэфф. (вес разряда) + смещение (в коде)
    //quint16 codeLimit=settings.uutbb_closecircuitpower_limit/settings.coefADC1[settings.board_counter] + settings.offsetADC1[settings.board_counter]; // код, пороговое напряжение.
    int ret=0; // код возврата ошибки
    QDateTime starttime; // время начала измерения
    QDateTime dt; // текущее время очередного измерения
    double x; // текущая координата Х
    int cycleTimeSec=settings.uutbb_time_ccp; // длительность цикла проверки в секундах
    bool bFirstPoll=true; // первое измерение

    // Подготовка графика
    ui->widgetClosedCircuitVoltagePowerUUTBB->addGraph(); // blue line
    ui->widgetClosedCircuitVoltagePowerUUTBB->graph(0)->setPen(QPen(Qt::blue));
    ui->widgetClosedCircuitVoltagePowerUUTBB->graph(0)->clearData();
    ui->widgetClosedCircuitVoltagePowerUUTBB->addGraph(); // blue dot
    ui->widgetClosedCircuitVoltagePowerUUTBB->graph(1)->clearData();
    ui->widgetClosedCircuitVoltagePowerUUTBB->graph(1)->setLineStyle(QCPGraph::lsNone);
    //ui->widgetClosedCircuitVoltagePowerUUTBB->graph(1)->setPen(QPen(Qt::green));
    ui->widgetClosedCircuitVoltagePowerUUTBB->graph(1)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, Qt::blue, Qt::white, 7));
    ui->widgetClosedCircuitVoltagePowerUUTBB->addGraph(); // red line
    ui->widgetClosedCircuitVoltagePowerUUTBB->graph(2)->setPen(QPen(Qt::red));
    ui->widgetClosedCircuitVoltagePowerUUTBB->graph(2)->setBrush(QBrush(QColor(255, 0, 0, 20)));
    ui->widgetClosedCircuitVoltagePowerUUTBB->graph(2)->clearData();
    ui->widgetClosedCircuitVoltagePowerUUTBB->graph(2)->addData(0, settings.uutbb_closecircuitpower_limit);
    ui->widgetClosedCircuitVoltagePowerUUTBB->graph(2)->addData(cycleTimeSec+0.2, settings.uutbb_closecircuitpower_limit);

    ui->widgetClosedCircuitVoltagePowerUUTBB->xAxis->setLabel(tr("Время, c"));
    ui->widgetClosedCircuitVoltagePowerUUTBB->xAxis->setRange(0, cycleTimeSec+0.2);
    ui->widgetClosedCircuitVoltagePowerUUTBB->yAxis->setLabel(tr("Напряжение, В"));
    ui->widgetClosedCircuitVoltagePowerUUTBB->yAxis->setRange(0, 9);

    // показать закладку на экране
    //ui->tabWidget->addTab(ui->tabClosedCircuitVoltagePowerSupply, ui->rbClosedCircuitVoltagePowerSupply->text());
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
        ui->cbParamsAutoMode->setCurrentIndex(8);   ///  переключаем режим комбокса на наш
    }

    // откроем вкладку
    ui->tabWidget->addTab(ui->tabClosedCircuitVoltagePowerSupply, ui->rbClosedCircuitVoltagePowerSupply->text());
    ui->tabWidget->setCurrentIndex(ui->tabWidget->count()-1);
    Log(tr("Проверка начата - %1").arg(ui->rbClosedCircuitVoltagePowerSupply->text()), "blue");
    ui->statusBar->showMessage(tr("Проверка ")+ui->rbClosedCircuitVoltagePowerSupply->text()+" ...");

    if(bModeManual)// если в ручном режиме
    {
        iCurrentStep=ui->cbClosedCircuitVoltagePowerSupply->currentIndex();
        iMaxSteps=iCurrentStep+1; // чтобы цикл for выполнился только раз в ручном.
    }
    else
    {
        iCurrentStep = ui->cbSubParamsAutoMode->currentIndex();
        iMaxSteps = ui->cbSubParamsAutoMode->count();
    }

    ui->progressBar->setMaximum(2); // установить кол-во ступеней прогресса
    ui->progressBar->reset();

    baSendArray.clear();
    baSendCommand.clear();
    baRecvArray.clear();

    /// при наличии галки имитатора, выводим сообщение о необходимости включить источник питания
    if(ui->cbIsImitator->isChecked() and iPowerState != 1) {
        QMessageBox::information(this, tr("Внимание! - %0").arg(ui->rbClosedCircuitVoltagePowerSupply->text()), tr("Перед проверкой необходимо включить источник питания!"));
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
            .arg(ui->rbClosedCircuitVoltagePowerSupply->text())
            .arg((ui->rbModeDiagnosticAuto->isChecked()) ? "Автоматический режим" : "Ручной режим")
            .arg(settings.uutbb_closecircuitpower_limit);

    /// формируем строку и пишем на label "идет измерение..."
    sLabelText = tr("1) \"%0\"").arg(battery[iBatteryIndex].uutbb_closecircuitpower[0]);
    ui->labelClosedCircuitVoltagePowerSupply0->setText(sLabelText + " идет измерение...");
    ui->labelClosedCircuitVoltagePowerSupply0->setStyleSheet("QLabel { color : blue; }");

    // сбросить коробочку
    baSendArray = (baSendCommand="IDLE")+"#"; // подготовить буфер для передачи
    timerSend->start(settings.delay_after_request_before_next_ADC1); // послать baSendArray в порт
    // ждём ответа. по сигналу о готовности принятых данных или по таймауту, вывалимся из цикла
    ret=loop.exec();
    if(ret) goto stop; // если не ноль (ошибка таймаута) - вывалиться из режима. если 0, то приняли данные из порта
    ui->progressBar->setValue(ui->progressBar->value()+1);

    // собрать режим
    baSendArray=(baSendCommand = (iCurrentStep==0)?"UccPB":"UccPBI")+"#"; // 0 - просто НЗЦбп, 1 - НЗЦбп с контролем тока
    if(bDeveloperState) Log(QString("Sending ") + qPrintable(baSendArray), "blue");
    timerSend->start(settings.delay_after_IDLE_before_other);
    ret=loop.exec();
    if(ret) goto stop;
    ui->progressBar->setValue(ui->progressBar->value()+1);

    starttime = QDateTime::currentDateTime(); // время начала измерения
    dt = QDateTime::currentDateTime(); // текущее время
    ui->widgetClosedCircuitVoltagePowerUUTBB->graph(0)->clearData(); // очистить график

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
        ui->widgetClosedCircuitVoltagePowerUUTBB->graph(0)->rescaleValueAxis(true); // для автоматического перерисовывания шкалы графика, если значения за пределами экрана
        ui->widgetClosedCircuitVoltagePowerUUTBB->graph(0)->addData((double)x/1000, (double)fU);
        ui->widgetClosedCircuitVoltagePowerUUTBB->replot();
    }

    dArrayClosedCircuitVoltagePowerSupply[0] = fU; //iCurrentStep] = fU;

    if(bDeveloperState)
        Log("Цепь "+battery[iBatteryIndex].uutbb_closecircuitpower[iCurrentStep]+" Receive "+qPrintable(baRecvArray)+" codeADC1=0x"+QString("%1").arg((ushort)codeADC, 0, 16), "blue");

    // напечатать рез-т в закладку и в журнал
    if (dArrayClosedCircuitVoltagePowerSupply[0] < settings.uutbb_closecircuitpower_limit) {
        sResult = "Не норма!";
        color = "red";
    }
    else {
        sResult = "Норма";
        color = "green";
    }
    ui->labelClosedCircuitVoltagePowerSupply0->setText(tr("%0 = <b>%1</b> В. %2").arg(sLabelText).arg(dArrayClosedCircuitVoltagePowerSupply[0], 0, 'f', 2).arg(sResult));
    ui->labelClosedCircuitVoltagePowerSupply0->setStyleSheet("QLabel { color : "+color+"; }");
    Log(tr("%0 = <b>%1</b> В. %2").arg(sLabelText).arg(dArrayClosedCircuitVoltagePowerSupply[0], 0, 'f', 2).arg(sResult), color);

    /// заполняем массив проверок для отчета
    dateTime = QDateTime::currentDateTime();
    sHtml += tr("<tr>"\
                "    <td><p>&nbsp;%0&nbsp;</td>"\
                "    <td><p>&nbsp;%1&nbsp;</td>"\
                "    <td><p>&nbsp;%2&nbsp;</td>"\
                "    <td><p>&nbsp;%3&nbsp;</td>"\
                "</tr>")
                .arg(dateTime.toString("hh:mm:ss"))
                .arg(battery[iBatteryIndex].uutbb_closecircuitpower[0])
                .arg(dArrayClosedCircuitVoltagePowerSupply[0], 0, 'f', 2)
                .arg(sResult);

    if(!bModeManual) { /// автоматический режим
        /// в автоматическом режиме пролистываем комбокс подпараметров проверки
        ui->cbSubParamsAutoMode->setCurrentIndex(ui->cbSubParamsAutoMode->currentIndex()+1);

        /// проанализировать результаты
        /*if(codeADC < codeLimit) { /// напряжение меньше (не норма)
            bState = false; /// выходим из режима проверки
        }*/
    }

    /// добавим в массив графиков полученный график ВРЕМЕННО СКРЫТ
    /*ui->widgetClosedCircuitVoltagePowerUUTBB->savePng(QDir::tempPath()+"ClosedCircuitVoltagePowerUUTBBGraph.png", 413, 526, 1.0, -1);
    img.load(QDir::tempPath()+"ClosedCircuitVoltagePowerUUTBBGraph.png");
    imgArrayReportGraph.append(img);
    sArrayReportGraphDescription.append(tr("График. %0. Цепь: \"%1\". Время: %2.").arg(ui->rbClosedCircuitVoltagePowerSupply->text()).arg(battery[iBatteryIndex].uutbb_closecircuitpower[0]).arg(dateTime.toString("hh:mm:ss")));*/

stop:
    if(ret == KDS_STOP) {
        label->setText(sLabelText + " измерение прервано!");
        label->setStyleSheet("QLabel { color : red; }");
        Log(sLabelText + " измерение прервано!", "red");
        sHtml += tr("<tr><td>&nbsp;%0&nbsp;</td><td>&nbsp;%1&nbsp;</td><td colspan=\"2\"><p>&nbsp;Измерение прервано!&nbsp;</td></tr>")
                .arg(dateTime.toString("hh:mm:ss"))
                .arg(battery[iBatteryIndex].uutbb_closecircuitpower[0]);
    }
    // сбросить коробочку
    baSendArray = (baSendCommand="IDLE")+"#";
    timerSend->start(settings.delay_after_request_before_next_ADC1);
    ret=loop.exec();

    bState = false; /// выходим из режима проверки
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

    Log(tr("Проверка завершена - %1").arg(ui->rbClosedCircuitVoltagePowerSupply->text()), "blue");

    timerPing->start(delay_timerPing); // запустить пинг по выходу из режима
    baSendArray.clear(); // очистить буфера команд.
    baSendCommand.clear();
    baRecvArray.clear();
    ui->progressBar->reset();
}
