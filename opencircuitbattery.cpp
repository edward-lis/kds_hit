#include <QDebug>
#include <QMessageBox>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "battery.h"
#include "settings.h"

extern Settings settings;

extern QVector<Battery> battery;


// Нажата кнопка проверки напряжения разомкнутой цепи батареи
void MainWindow::on_btnOpenCircuitVoltageBattery_clicked()
{
    quint16 codeADC=0; // принятый код АЦП
    float fU=0; // принятое напряжение в вольтах
    // код порогового напряжения = пороговое напряжение В / коэфф. (вес разряда) + смещение (в коде)
    quint16 codeLimit=settings.opencircuitbattery_limit/settings.coefADC1[settings.board_counter] + settings.offsetADC1[settings.board_counter]; // код, пороговое напряжение.
    int ret=0; // код возврата ошибки
    int i=0; // номер цепи
    //QLabel *label; // надпись в закладке

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
        ui->cbParamsAutoMode->setCurrentIndex(3);   ///  переключаем режим комбокса на наш
    }

    // откроем вкладку
    ui->tabWidget->addTab(ui->tabOpenCircuitVoltageBattery, ui->rbOpenCircuitVoltageBattery->text());
    ui->tabWidget->setCurrentIndex(ui->tabWidget->count()-1);
    Log(tr("Проверка начата - %1").arg(ui->rbOpenCircuitVoltageBattery->text()), "blue");
    ui->statusBar->showMessage(tr("Проверка ")+ui->rbOpenCircuitVoltageBattery->text()+" ...");

    if(bModeManual)// если в ручном режиме
    {
        i=ui->cbOpenCircuitVoltageBattery->currentIndex();
        iCurrentStep=i; // чтобы цикл for выполнился только раз в ручном.
        iMaxSteps=i+1;
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
        QMessageBox::information(this, tr("Внимание! - %0").arg(ui->rbOpenCircuitVoltageBattery->text()), tr("Перед проверкой необходимо включить источник питания!"));
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
            .arg(ui->rbOpenCircuitVoltageBattery->text())
            .arg((ui->rbModeDiagnosticAuto->isChecked()) ? "Автоматический режим" : "Ручной режим")
            .arg(settings.opencircuitbattery_limit);

    /// формируем строку и пишем на label "идет измерение..."
    sLabelText = tr("1) \"%0\"").arg(battery[iBatteryIndex].circuitbattery);
    ui->labelOpenCircuitVoltageBattery0->setText(sLabelText + " идет измерение...");
    ui->labelOpenCircuitVoltageBattery0->setStyleSheet("QLabel { color : blue; }");

    // сбросить коробочку
    baSendArray = (baSendCommand="IDLE")+"#"; // подготовить буфер для передачи
    timerSend->start(settings.delay_after_request_before_next_ADC1); // послать baSendArray в порт
    // ждём ответа. по сигналу о готовности принятых данных или по таймауту, вывалимся из цикла
    ret=loop.exec();
    if(ret) goto stop; // если не ноль (ошибка таймаута) - вывалиться из режима. если 0, то приняли данные из порта
    ui->progressBar->setValue(ui->progressBar->value()+1);

    // собрать режим
    baSendArray=(baSendCommand="UocB")+"#";
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
    dArrayOpenCircuitVoltageBattery[0] = fU;

    if(bDeveloperState)
        Log("Цепь "+battery[iBatteryIndex].circuitbattery+" Receive "+qPrintable(baRecvArray)+" codeADC1=0x"+QString("%1").arg((ushort)codeADC, 0, 16), "blue");

    if (dArrayOpenCircuitVoltageBattery[0] < settings.opencircuitbattery_limit) {
        sResult = "Не норма!";
        color = "red";
    }
    else {
        sResult = "Норма";
        color = "green";
    }
    ui->labelOpenCircuitVoltageBattery0->setText(tr("%0 = <b>%1</b> В. %2").arg(sLabelText).arg(dArrayOpenCircuitVoltageBattery[0], 0, 'f', 2).arg(sResult));
    ui->labelOpenCircuitVoltageBattery0->setStyleSheet("QLabel { color : "+color+"; }");
    Log(tr("%0 = <b>%1</b> В. %2").arg(sLabelText).arg(dArrayOpenCircuitVoltageBattery[0], 0, 'f', 2).arg(sResult), color);

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
                .arg(dArrayOpenCircuitVoltageBattery[0], 0, 'f', 2)
                .arg(sResult);

    if(!bModeManual) { /// автоматический режим
        /// в автоматическом режиме пролистываем комбокс подпараметров проверки
        ui->cbSubParamsAutoMode->setCurrentIndex(ui->cbSubParamsAutoMode->currentIndex()+1);
    }

    /// проанализировать результаты
    if(codeADC < codeLimit) { /// напряжение меньше нормы (не норма)
        if (QMessageBox::question(this, "Внимание - "+ui->rbOpenCircuitVoltageBattery->text(), tr("%0 = %1 В. %2 Продолжить?").arg(sLabelText).arg(dArrayOpenCircuitVoltageBattery[0], 0, 'f', 2).arg(sResult), tr("Да"), tr("Нет"))) {
            bState = false; /// выходим из режима проверки
            //goto stop;
        }
    }

stop:
    if(ret == KDS_STOP) {
        ui->labelOpenCircuitVoltageBattery0->setText(sLabelText + " измерение прервано!");
        ui->labelOpenCircuitVoltageBattery0->setStyleSheet("QLabel { color : red; }");
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

    Log(tr("Проверка завершена - %1").arg(ui->rbOpenCircuitVoltageBattery->text()), "blue");

    timerPing->start(delay_timerPing); // запустить пинг по выходу из режима
    baSendArray.clear(); // очистить буфера команд.
    baSendCommand.clear();
    baRecvArray.clear();
    ui->progressBar->reset();
}
