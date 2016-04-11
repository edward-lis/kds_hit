#include <QDebug>
#include <QMessageBox>
#include "math.h"

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "battery.h"
#include "settings.h"

extern Settings settings;

extern QVector<Battery> battery;

void MainWindow::on_btnVoltageOnTheHousing_clicked()
{
    int ret=0; // код возврата ошибки
    quint16 codeU=0; // код напряжение минус, плюс
    // код порогового напряжения = пороговое напряжение В / коэфф. (вес разряда) + смещение (в коде)
    quint16 codeLimit=0;//settings.voltage_corpus_limit/settings.coefADC2 + settings.offsetADC2; // код, пороговое напряжение.
    quint16 offset=0; // смещение, в зависимости от измерения напряжения на корпусе + или -
    float voltageU=0;
    int i=0; // номер цепи

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
        ui->cbParamsAutoMode->setCurrentIndex(0);   ///  переключаем режим комбокса на наш
    }

    /// устанавливаем стартовый шаг проверки
    iCurrentStep = (ui->rbModeDiagnosticAuto->isChecked()) ? ui->cbSubParamsAutoMode->currentIndex() : ui->cbVoltageOnTheHousing->currentIndex();
    /// устанавливаем кол-во шагов проверки
    iMaxSteps = (ui->rbModeDiagnosticAuto->isChecked()) ? ui->cbSubParamsAutoMode->count() : iCurrentStep+1;
    ui->progressBar->setMaximum(settings.voltagecase_num); /// установим максимум прогресс бара
    ui->tabWidget->addTab(ui->tabVoltageOnTheHousing, ui->rbVoltageOnTheHousing->text()); /// откроем вкладку
    ui->tabWidget->setCurrentIndex(ui->tabWidget->count()-1); /// и переходим на нее
    ui->statusBar->showMessage(tr("Проверка %0 ...").arg(ui->rbVoltageOnTheHousing->text())); /// пишем в статус бар
    Log(tr("Проверка начата - %1").arg(ui->rbVoltageOnTheHousing->text()), "blue"); /// пишем в журнал событий

    /// при наличии галки имитатора, выводим сообщение о необходимости включить источник питания
    if(ui->cbIsImitator->isChecked() and iPowerState != 1) {
        QMessageBox::information(this, tr("Внимание! - %0").arg(ui->rbVoltageOnTheHousing->text()), tr("Перед проверкой необходимо включить источник питания!"));
        iPowerState = 1; /// состояние включенного источника питания
    }

    for(i = iCurrentStep; i < iMaxSteps; i++)
    {
        /// очистить буфера
        baSendArray.clear();
        baSendCommand.clear();
        baRecvArray.clear();
        ui->progressBar->setValue(0); /// установим прогресс бар в начальное положение

        /// формируем строку и пишем на label "идет измерение..."
        sLabelText = tr("%0) \"%1\"").arg(i+1).arg(battery[iBatteryIndex].str_voltage_corpus[i]);
        label = findChild<QLabel*>(tr("labelVoltageOnTheHousing%0").arg(i));
        label->setText(sLabelText + " идет измерение...");
        label->setStyleSheet("QLabel { color : blue; }");

        for(int j=0; j<settings.voltagecase_num; j++)
        {
            // сбросить коробочку
            baSendArray = (baSendCommand="IDLE")+"#"; // подготовить буфер для передачи
            timerSend->start(settings.delay_after_request_before_next_ADC2); //sendSerialData(); // послать baSendArray в порт
            // ждём ответа. по сигналу о готовности принятых данных или по таймауту, вывалимся из цикла
            ret=loop.exec();
            if(ret) goto stop; // если не ноль (ошибка таймаута) - вывалиться из режима. если 0, то приняли данные из порта

            if(i == 1) // если выбрана в комбобоксе такая цепь
            {
                baSendArray=(baSendCommand="UcaseM")+"#";
                offset = settings.offsetADC2_minus;
            }
            else
            {
                baSendArray=(baSendCommand="UcaseP")+"#";
                offset = settings.offsetADC2_plus;
            }
            codeLimit=settings.voltage_corpus_limit/settings.coefADC2 + offset; // код, пороговое напряжение.

            if(bDeveloperState) Log(QString("Sending ") + qPrintable(baSendArray), "blue");
            timerSend->start(settings.delay_after_IDLE_before_other); // послать baSendArray в порт через некоторое время
            ret=loop.exec();
            if(ret) goto stop;

            baSendArray=baSendCommand+"?#";
            timerSend->start(settings.delay_after_start_before_request_voltagecase);
            ret=loop.exec();
            if(ret) goto stop;
            codeU = getRecvData(baRecvArray); // получить данные опроса

            voltageU = fabs((codeU-offset)*settings.coefADC2); // напряжение в вольтах
            dArrayVoltageOnTheHousing[i] = voltageU;

            ui->progressBar->setValue(ui->progressBar->value()+1);

            // если отладочный режим, напечатать отладочную инфу
            if(bDeveloperState)
            {
                Log(QString("k = ") + qPrintable(QString::number(settings.coefADC2)) + " код смещения offset =0x"+qPrintable(QString::number(offset, 16))+ " код АЦП2 = 0x" + qPrintable(QString::number(codeU, 16)) + " U=k*(code-offset) = " + QString::number(voltageU), "green");
            }
        }

        if (dArrayVoltageOnTheHousing[i] > settings.voltage_corpus_limit) {
            sResult = "Не норма!";
            color = "red";
        }
        else {
            sResult = "Норма";
            color = "green";
        }
        label->setText(tr("%0 = <b>%1</b> В. %2").arg(sLabelText).arg(dArrayVoltageOnTheHousing[i], 0, 'f', 2).arg(sResult));
        label->setStyleSheet("QLabel { color : "+color+"; }");
        Log(tr("%0 = <b>%1</b> В. %2").arg(sLabelText).arg(dArrayVoltageOnTheHousing[i], 0, 'f', 2).arg(sResult), color);

        /// заполняем массив проверок для отчета
        dateTime = QDateTime::currentDateTime();
        sArrayReportVoltageOnTheHousing.append(
                    tr("<tr>"\
                       "    <td>%0</td>"\
                       "    <td>%1</td>"\
                       "    <td>%2</td>"\
                       "    <td>%3</td>"\
                       "    <td>%4</td>"\
                       "</tr>")
                    .arg(dateTime.toString("hh:mm:ss"))
                    .arg(battery[iBatteryIndex].str_voltage_corpus[i])
                    .arg(dArrayVoltageOnTheHousing[i], 0, 'f', 2)
                    .arg(sResult)
                    .arg((ui->rbModeDiagnosticAuto->isChecked()) ? "Автоматический" : "Ручной"));

        if(codeU > codeLimit) // напряжение больше (в кодах)
        {
            if(!bModeManual)// если в автоматическом режиме
            {
                //if(!bModeManual && !bDeveloperState)QMessageBox::critical(this, "Не норма!", "Напряжение цепи "+battery[iBatteryIndex].str_voltage_corpus[ui->cbVoltageOnTheHousing->currentIndex()]+" = "+QString::number(voltageU)+" В больше нормы");// !!!
                if (QMessageBox::question(this, "Внимание - "+ui->rbVoltageOnTheHousing->text(), tr("%0 = %1 В. %2 Продолжить?").arg(sLabelText).arg(dArrayVoltageOnTheHousing[i], 0, 'f', 2).arg(sResult), tr("Да"), tr("Нет")))
                {
                    qDebug()<<"переход в ручной режим";
                    Log("Останов проверки - переход в ручной режим", "blue");
                    bState = false;
                    ui->groupBoxCOMPort->setEnabled(true); // разрешить кнопку ком-порта???
                    ui->groupBoxDiagnosticMode->setEnabled(true); // разрешить группу выбора режима диагностики
                    //ui->groupBoxCheckParamsAutoMode->setEnabled(true); // разрешить группу выбора режима диагностики
                    //ui->cbParamsAutoMode->setEnabled(true); // разрешить комбобокс пунктов автомата
                    //ui->cbSubParamsAutoMode->setEnabled(true); // разрешать комбобокс подпунктов автомата
                    ((QPushButton*)sender())->setText("Пуск");
                    // остановить текущую проверку, выход
                    bCheckInProgress = false;
                    emit ui->rbModeDiagnosticManual->setChecked(true);
                    break;
                }
            }
        }

        // если ручной режим, то выдать окно сообщения, и только потом разобрать режим измерения.
        if (ui->rbModeDiagnosticManual->isChecked())
            QMessageBox::information(this, tr("Напряжение на корпусе"), tr("Напряжение цепи ")+battery[iBatteryIndex].str_voltage_corpus[ui->cbVoltageOnTheHousing->currentIndex()]+" = "+QString::number(voltageU, 'f', 2)+" В");
        else
            ui->cbSubParamsAutoMode->setCurrentIndex(ui->cbSubParamsAutoMode->currentIndex()+1);
    } // for

stop:
    if(ret == KDS_STOP) {
        label->setText(sLabelText + " измерение прервано!");
        label->setStyleSheet("QLabel { color : red; }");
        Log(sLabelText + " измерение прервано!", "red");
    }
    // сбросить коробочку
    baSendArray = (baSendCommand="IDLE")+"#";
    timerSend->start(settings.delay_after_request_before_next_ADC2);
    loop.exec();
    ui->progressBar->setValue(ui->progressBar->value()+1);

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
        bState = false;
    }

    Log(tr("Проверка завершена - %1").arg(ui->rbVoltageOnTheHousing->text()), "blue");

    timerPing->start(delay_timerPing); // запустить пинг по выходу из режима
    baSendArray.clear();
    baSendCommand.clear();
    baRecvArray.clear();
    ui->progressBar->reset(); /// сбросим прогресс бар
}
