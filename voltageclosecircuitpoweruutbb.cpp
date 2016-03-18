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
    //checkClosedCircuitVoltagePowerSupply(); return;
    quint16 codeADC=0; // принятый код АЦП
    float fU=0; // принятое напряжение в вольтах
    // код порогового напряжения = пороговое напряжение В / коэфф. (вес разряда) + смещение (в коде)
    quint16 codeLimit=settings.uutbb_closecircuitpower_limit/settings.coefADC1 + settings.offsetADC1; // код, пороговое напряжение.
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

    // запретим виджеты, чтоб не нажимались
    ui->groupBoxCOMPort->setDisabled(true);
    ui->groupBoxDiagnosticDevice->setDisabled(true);
    ui->groupBoxDiagnosticMode->setDisabled(true);
    ui->cbParamsAutoMode->setDisabled(true);
    ui->cbSubParamsAutoMode->setDisabled(true);

    // откроем вкладку
    ui->tabWidget->addTab(ui->tabClosedCircuitVoltagePowerSupply, ui->rbClosedCircuitVoltagePowerSupply->text());
    ui->tabWidget->setCurrentIndex(ui->tabWidget->count()-1);
    Log(tr("Проверка начата - %1").arg(ui->rbClosedCircuitVoltagePowerSupply->text()), "blue");
    ui->statusBar->showMessage(tr("Проверка ")+ui->rbClosedCircuitVoltagePowerSupply->text()+" ...");

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
        iCurrentStep=ui->cbClosedCircuitVoltagePowerSupply->currentIndex();
        iMaxSteps=iCurrentStep+1; // чтобы цикл for выполнился только раз в ручном.
    }
    else
    {
        ui->cbParamsAutoMode->setCurrentIndex(8); // переключаем режим комбокса на наш
        iCurrentStep = ui->cbSubParamsAutoMode->currentIndex();
        iMaxSteps = ui->cbSubParamsAutoMode->count();
    }

    ui->progressBar->setMaximum(2); // установить кол-во ступеней прогресса
    ui->progressBar->reset();

    baSendArray.clear();
    baSendCommand.clear();
    baRecvArray.clear();

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
        fU = ((codeADC-settings.offsetADC1)*settings.coefADC1); // напряжение в вольтах
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

    ui->btnBuildReport->setEnabled(true);

    /// заполняем массив проверок для отчета
    dateTime = QDateTime::currentDateTime();
    sArrayReportClosedCircuitVoltagePowerSupply.append(
                tr("<tr>"\
                   "    <td>%0</td>"\
                   "    <td>%1</td>"\
                   "    <td>%2</td>"\
                   "    <td>%3</td>"\
                   "    <td>%4</td>"\
                   "</tr>")
                .arg(dateTime.toString("hh:mm:ss"))
                .arg(battery[iBatteryIndex].uutbb_closecircuitpower[0])
                .arg(dArrayClosedCircuitVoltagePowerSupply[0], 0, 'f', 2)
                .arg(sResult)
                .arg((ui->rbModeDiagnosticAuto->isChecked()) ? "Автоматический" : "Ручной"));

    /// добавим в массив графиков полученный график ВРЕМЕННО СКРЫТ
    /*ui->widgetClosedCircuitVoltagePowerUUTBB->savePng(QDir::tempPath()+"ClosedCircuitVoltagePowerUUTBBGraph.png", 413, 526, 1.0, -1);
    img.load(QDir::tempPath()+"ClosedCircuitVoltagePowerUUTBBGraph.png");
    imgArrayReportGraph.append(img);
    sArrayReportGraphDescription.append(tr("График. %0. Цепь: \"%1\". Время: %2.").arg(ui->rbClosedCircuitVoltagePowerSupply->text()).arg(battery[iBatteryIndex].uutbb_closecircuitpower[0]).arg(dateTime.toString("hh:mm:ss")));*/

    // проанализировать результаты
    if(codeADC >= codeLimit) // напряжение больше (норма)
    {
        //Log("Напряжение цепи "+battery[iBatteryIndex].circuitbattery+" = "+QString::number(fU, 'f', 2)+" В.  Норма.", "blue");
        // если ручной режим, то выдать окно сообщения, и только потом разобрать режим измерения.
        // без нагрузки показывать нет смысла if(bModeManual) QMessageBox::information(this, tr("Напряжение замкнутой цепи БП УУТББ"), tr("Напряжение цепи ")+battery[iBatteryIndex].circuitbattery+" = "+QString::number(fU, 'f', 2)+" В\nНорма");
    }
    else // напряжение меньше (не норма)
    {
        // Log("Напряжение цепи "+battery[iBatteryIndex].circuitbattery+" = "+QString::number(fU, 'f', 2)+" В.  Не норма!.", "red");
        // если ручной режим, то выдать окно сообщения, и только потом разобрать режим измерения.
        // без нагрузки показывать нет смысла if(bModeManual) QMessageBox::information(this, tr("Напряжение замкнутой цепи БП УУТББ"), tr("Напряжение цепи ")+battery[iBatteryIndex].circuitbattery+" = "+QString::number(fU, 'f', 2)+" В\nНе норма!");
        ui->rbModeDiagnosticManual->setChecked(true); // переключить в ручной принудительно
        bState = false;
    }

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
    ui->btnClosedCircuitVoltagePowerSupply->setText("Пуск");// поменять текст на кнопке

    timerPing->start(delay_timerPing); // запустить пинг по выходу из режима
    baSendArray.clear(); // очистить буфера команд.
    baSendCommand.clear();
    baRecvArray.clear();
    ui->progressBar->reset();
}

/*
 * Напряжение замкнутой цепи блока питания
 */
/*void MainWindow::checkClosedCircuitVoltagePowerSupply()
{
    qDebug() << "sender=" << ((QPushButton*)sender())->objectName() << "bState=" << bState;
    ui->tabWidget->addTab(ui->tabClosedCircuitVoltagePowerSupply, ui->rbClosedCircuitVoltagePowerSupply->text());
    ui->tabWidget->setCurrentIndex(ui->tabWidget->count()-1);
    Log(tr("Проверка начата - %1").arg(ui->rbClosedCircuitVoltagePowerSupply->text()), "blue");

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
        ui->cbParamsAutoMode->setCurrentIndex(8); // переключаем режим комбокса на наш

    ui->groupBoxCOMPort->setDisabled(bState);
    ui->groupBoxDiagnosticDevice->setDisabled(bState);
    ui->groupBoxDiagnosticMode->setDisabled(bState);
    ui->cbParamsAutoMode->setDisabled(bState);
    ui->cbSubParamsAutoMode->setDisabled(bState);

    iCurrentStep = (ui->rbModeDiagnosticAuto->isChecked()) ? ui->cbSubParamsAutoMode->currentIndex() : ui->cbClosedCircuitVoltagePowerSupply->currentIndex();
    iMaxSteps = (ui->rbModeDiagnosticAuto->isChecked()) ? ui->cbSubParamsAutoMode->count() : ui->cbClosedCircuitVoltagePowerSupply->count();
    ui->progressBar->setMaximum(iMaxSteps);
    ui->progressBar->setValue(iCurrentStep);

    switch (iBatteryIndex) {
    case 0: //9ER20P-20
        for (int i = iCurrentStep; i < iMaxSteps; i++) {
            if (!bState) return;
            switch (i) {
            case 0:
                delay(1000);
                dArrayClosedCircuitVoltagePowerSupply[i] = randMToN(5, 6); //число полученное с COM-порта
                break;
            case 1:
                delay(1000);
                dArrayClosedCircuitVoltagePowerSupply[i] = randMToN(5, 6); //число полученное с COM-порта
                break;
            default:
                return;
                break;
            }

            if(ui->rbModeDiagnosticManual->isChecked())
                ui->cbClosedCircuitVoltagePowerSupply->setCurrentIndex(i);
            else
                ui->cbSubParamsAutoMode->setCurrentIndex(i);

            str = tr("Напряжение цепи \"%0\" = <b>%1</b> В.").arg(battery[iBatteryIndex].uutbb_closecircuitpower[i]).arg(dArrayClosedCircuitVoltagePowerSupply[i]);
            QLabel * label = findChild<QLabel*>(tr("labelClosedCircuitVoltagePowerSupply%0").arg(i));
            if (dArrayClosedCircuitVoltagePowerSupply[i] > settings.uutbb_closecircuitpower_limit) {
                str += " Не норма.";
                color = "red";
            } else
                color = "green";
            label->setText(str);
            label->setStyleSheet("QLabel { color : "+color+"; }");
            Log(str, color);
            ui->btnBuildReport->setEnabled(true);
            if (dArrayClosedCircuitVoltagePowerSupply[i] > settings.uutbb_closecircuitpower_limit) {
                if (QMessageBox::question(this, "Внимание - "+ui->rbClosedCircuitVoltagePowerSupply->text(), tr("%0 Продолжить?").arg(str), tr("Да"), tr("Нет"))) {
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

    Log(tr("Проверка завершена - %1").arg(ui->rbClosedCircuitVoltagePowerSupply->text()), "blue");

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
