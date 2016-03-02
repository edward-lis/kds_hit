#include <QDebug>
#include <QMessageBox>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "battery.h"

extern QVector<Battery> battery;

// Нажата кнопка проверки сопротивления изоляции
void MainWindow::on_btnInsulationResistance_clicked()
{
    //checkInsulationResistance(); return;
    QString str_num; // номер цепи
    quint16 u=0; // полученный код АЦП
    float resist=0; // получившееся сопротивление
    int j=0;
    int ret=0; // код возврата ошибки
    QString strResist; // строка с получившимся значением
    int i=0; // номер цепи
    QLabel *label; // надпись в закладке

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
    ui->groupBoxCOMPort->setDisabled(bState);
    ui->groupBoxDiagnosticDevice->setDisabled(bState);
    ui->groupBoxDiagnosticMode->setDisabled(bState);
    ui->cbParamsAutoMode->setDisabled(bState);
    ui->cbSubParamsAutoMode->setDisabled(bState);

    // откроем вкладку
    ui->tabWidget->addTab(ui->tabInsulationResistance, ui->rbInsulationResistance->text());
    ui->tabWidget->setCurrentIndex(ui->tabWidget->count()-1);
    Log(tr("Проверка начата - %1").arg(ui->rbInsulationResistance->text()), "blue");
    ui->statusBar->showMessage(tr("Проверка ")+ui->rbInsulationResistance->text()+" ...");

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

        i=ui->cbInsulationResistance->currentIndex();
        iCurrentStep=i; // чтобы цикл for выполнился только раз в ручном.
        iMaxSteps=i+1;
    }
    else
    {
        ui->cbParamsAutoMode->setCurrentIndex(1); // переключаем режим комбокса на наш
        iCurrentStep = (ui->rbModeDiagnosticAuto->isChecked()) ? ui->cbSubParamsAutoMode->currentIndex() : ui->cbVoltageOnTheHousing->currentIndex();
        iMaxSteps = (ui->rbModeDiagnosticAuto->isChecked()) ? ui->cbSubParamsAutoMode->count() : ui->cbVoltageOnTheHousing->count();
    }

    for(i=iCurrentStep; i<iMaxSteps; i++)
    {
        ui->progressBar->setMaximum(3); // установить кол-во ступеней прогресса
        ui->progressBar->reset();

        baSendArray.clear();
        baSendCommand.clear();
        baRecvArray.clear();

        // сбросить коробочку
        baSendArray = (baSendCommand="IDLE")+"#"; // подготовить буфер для передачи
        timerSend->start(settings.delay_after_request_before_next_ADC2); //sendSerialData(); // послать baSendArray в порт
        // ждём ответа. по сигналу о готовности принятых данных или по таймауту, вывалимся из цикла
        ret=loop.exec();
        if(ret) goto stop; // если не ноль (ошибка таймаута) - вывалиться из режима. если 0, то приняли данные из порта
        ui->progressBar->setValue(ui->progressBar->value()+1);

        // собрать режим /// i???
        str_num.sprintf(" %02i", bModeManual?battery[iBatteryIndex].isolation_resistance_nn[ui->cbInsulationResistance->currentIndex()]:i+1); // напечатать номер точки измерения изоляции
        baSendArray=(baSendCommand="Rins")+str_num.toLocal8Bit()+"#";
        if(bDeveloperState) Log(QString("Sending ") + qPrintable(baSendArray), "blue");
        timerSend->start(settings.delay_after_IDLE_before_other); // послать baSendArray в порт через некоторое время
        ret=loop.exec();
        if(ret) goto stop;
        ui->progressBar->setValue(ui->progressBar->value()+1);

        // послать опрос
        baSendArray=baSendCommand+"?#";
        timerSend->start(settings.delay_after_start_before_request_ADC2);
        ret=loop.exec();
        if(ret) goto stop; // если ошибка - вывалиться из режима
        u = getRecvData(baRecvArray); // получить данные опроса
        ui->progressBar->setValue(ui->progressBar->value()+1);

        // будем щщитать сразу в коде, без перехода в вольты
        // пробежимся по точкам ф-ии, и высчитаем сопротивление согласно напряжению
        for(j=0; j<settings.functionResist.size()-2; j++)
        {
            //qDebug()<<"u"<<qPrintable(QString::number(u, 16))<<qPrintable(QString::number(settings.functionResist[j].codeADC, 16))<<qPrintable(QString::number(settings.functionResist[j+1].codeADC, 16));
            if((u > settings.functionResist[j].codeADC) && (u <= settings.functionResist[j+1].codeADC))
            {
                //qDebug()<<"resist=(-("<<settings.functionResist[j].codeADC<<"*"<<settings.functionResist[j+1].resist<<"-"<<settings.functionResist[j+1].codeADC<<"*"<<settings.functionResist[j].resist<<")-("<<settings.functionResist[j].resist<<"-"<<settings.functionResist[j+1].resist<<")*"<<u<<")/("<<settings.functionResist[j+1].codeADC<<"-"<<settings.functionResist[j].codeADC<<")";
                resist = ( -(settings.functionResist[j].codeADC * settings.functionResist[j+1].resist - settings.functionResist[j+1].codeADC * settings.functionResist[j].resist) - (settings.functionResist[j].resist - settings.functionResist[j+1].resist) * u) / (settings.functionResist[j+1].codeADC - settings.functionResist[j].codeADC);
                break;
            }
        }
        // если напряжение меньше меньшего, то будем щщитать как первом отрезке ф-ии
        if(u <= settings.functionResist[0].codeADC)
        {
            j=0;
            resist = ( -(settings.functionResist[j].codeADC * settings.functionResist[j+1].resist - settings.functionResist[j+1].codeADC * settings.functionResist[j].resist) - (settings.functionResist[j].resist - settings.functionResist[j+1].resist) * u) / (settings.functionResist[j+1].codeADC - settings.functionResist[j].codeADC);
        }
        // если напряжение больше большего, то будем щщитать как на последнем отрезке ф-ии
        if(u>settings.functionResist[settings.functionResist.size()-1].codeADC)
        {
            j=settings.functionResist.size()-2;
            resist = ( -(settings.functionResist[j].codeADC * settings.functionResist[j+1].resist - settings.functionResist[j+1].codeADC * settings.functionResist[j].resist) - (settings.functionResist[j].resist - settings.functionResist[j+1].resist) * u) / (settings.functionResist[j+1].codeADC - settings.functionResist[j].codeADC);
        }
        // если меньше нуля, то обнулим
        if(resist<0) resist=0; // не бывает отрицательного сопротивления
        /*if(resist > 1000000)
        {
            // переведём в мегаомы
            resist = resist/1000000;
            strResist = QString::number(resist, 'f', 2) + "МОм, ";
        }
        else if(resist > 1000)
        {
            // переведём в килоомы
            resist = resist/1000;
            strResist = QString::number(resist, 'f', 2) + "кОм, ";
        }
        else
        {
            // омы
            strResist = QString::number(resist, 'f', 0) + "Ом, ";
        }*/
        strResist = QString::number(resist, 'f', 0) + " Ом, ";
        dArrayInsulationResistance[i] = resist;

        // если отладочный режим, напечатать отладочную инфу
        if(bDeveloperState)
        {
            Log("Сопротивление изоляции: " + ui->cbInsulationResistance->currentText() + "=" + strResist + "код АЦП= 0x" + QString("%1").arg((ushort)u, 0, 16), "green");
        }
        if(settings.verbose) qDebug()<<" u=0x"<<qPrintable(QString::number(u, 16))<<" resist="<<resist;

        // напечатать рез-т в закладку и в журнал
        str = tr("Сопротивление цепи \"%0\" = <b>%1</b> МОм.").arg(battery[iBatteryIndex].str_isolation_resistance[i]).arg(dArrayInsulationResistance[i], 0, 'f', 0);
        label = findChild<QLabel*>(tr("labelInsulationResistance%0").arg(i));
        if (dArrayInsulationResistance[i] < settings.isolation_resistance_limit) {
            sResult = "Не норма!";
            color = "red";
        }
        else {
            sResult = "Норма";
            color = "green";
        }
        label->setText(str+" "+sResult);
        label->setStyleSheet("QLabel { color : "+color+"; }");
        Log(str+" "+sResult, color);

        ui->btnBuildReport->setEnabled(true); // разрешить кнопку отчёта

        /// заполняем массив проверок для отчета
        QDateTime dateTime = QDateTime::currentDateTime();
        QString textTime = dateTime.toString("hh:mm:ss");
        sArrayReportInsulationResistance.append(
                    tr("<tr>"\
                       "    <td>%0</td>"\
                       "    <td>%1</td>"\
                       "    <td>%2</td>"\
                       "    <td>%3</td>"\
                       "</tr>")
                    .arg(textTime)
                    .arg(battery[iBatteryIndex].str_isolation_resistance[i])
                    .arg(dArrayInsulationResistance[i], 0, 'g', 0)
                    .arg(sResult));

        if (dArrayInsulationResistance[i] < settings.isolation_resistance_limit) {
            if(!bModeManual)// если в автоматическом режиме
            {
                if (QMessageBox::question(this, "Внимание - "+ui->rbInsulationResistance->text(), tr("%0 Продолжить?").arg(str+" "+sResult), tr("Да"), tr("Нет"))) {
                    bState = false;
                    ui->groupBoxCOMPort->setDisabled(bState);
                    ui->groupBoxDiagnosticMode->setDisabled(bState);
                    ui->cbParamsAutoMode->setDisabled(bState);
                    ui->cbSubParamsAutoMode->setDisabled(bState);
                    ((QPushButton*)sender())->setText("Пуск");
                    // остановить текущую проверку, выход
                    bCheckInProgress = false;
                    emit ui->rbModeDiagnosticManual->setChecked(true);
                    break;
                }
            }
        }
    } // for

stop:
    // сбросить коробочку
    baSendArray = (baSendCommand="IDLE")+"#";
    timerSend->start(settings.delay_after_request_before_next_ADC2);
    ret=loop.exec();

    bCheckInProgress = false; // вышли из состояния проверки

    // если отладочный режим, напечатать отладочную инфу
    if(bDeveloperState)
    {
        if(ret == KDS_TIMEOUT) Log(tr("Timeout!"), "red");
        else if(ret == KDS_INCORRECT_REPLY) Log(tr("Incorrect reply!"), "red");
        else if(ret == KDS_STOP) Log(tr("Stop checking!"), "red");
    }
    if(ret == KDS_STOP) Log(tr("Останов оператором!"), "red");

    if(bModeManual)
    {
        bState = false;
        //ui->groupBoxCOMPort->setEnabled(bState);          // кнопка последовательного порта
        ui->groupBoxDiagnosticDevice->setDisabled(bState);  // открыть группу выбора батареи
        ui->groupBoxDiagnosticMode->setDisabled(bState);    // окрыть группу выбора режима
        ui->cbParamsAutoMode->setDisabled(bState);          // открыть комбобокс выбора пункта начала автоматического режима
        ui->cbSubParamsAutoMode->setDisabled(bState);       // открыть комбобокс выбора подпункта начала автоматического режима
        ((QPushButton*)sender())->setText("Пуск");         // поменять текст на кнопке
    }

    timerPing->start(delay_timerPing); // запустить пинг по выходу из режима
    baSendArray.clear(); // очистить буфера команд.
    baSendCommand.clear();
    baRecvArray.clear();
    ui->progressBar->reset();
}

/*
 * Сопротивление изоляции
 */
void MainWindow::checkInsulationResistance()
{
    qDebug() << "sender=" << ((QPushButton*)sender())->objectName() << "bState=" << bState;
    ui->tabWidget->addTab(ui->tabInsulationResistance, ui->rbInsulationResistance->text());
    ui->tabWidget->setCurrentIndex(ui->tabWidget->count()-1);
    Log(tr("Проверка начата - %1").arg(ui->rbInsulationResistance->text()), "blue");

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
        ui->cbParamsAutoMode->setCurrentIndex(1); // переключаем режим комбокса на наш

    ui->groupBoxCOMPort->setDisabled(bState);
    ui->groupBoxDiagnosticDevice->setDisabled(bState);
    ui->groupBoxDiagnosticMode->setDisabled(bState);
    ui->cbParamsAutoMode->setDisabled(bState);
    ui->cbSubParamsAutoMode->setDisabled(bState);

    iCurrentStep = (ui->rbModeDiagnosticAuto->isChecked()) ? ui->cbSubParamsAutoMode->currentIndex() : ui->cbInsulationResistance->currentIndex();
    iMaxSteps = (ui->rbModeDiagnosticAuto->isChecked()) ? ui->cbSubParamsAutoMode->count() : ui->cbInsulationResistance->count();
    ui->progressBar->setMaximum(iMaxSteps);
    ui->progressBar->setValue(iCurrentStep);

    switch (iBatteryIndex) {
    case 0: //9ER20P-20
        for (int i = iCurrentStep; i < iMaxSteps; i++) {
            if (!bState) return;
            switch (i) {
            case 0:
                delay(1000);
                dArrayInsulationResistance[i] = randMToN(19, 21); //число полученное с COM-порта
                break;
            case 1:
                delay(1000);
                dArrayInsulationResistance[i] = randMToN(19, 21); //число полученное с COM-порта
                break;
            case 2:
                delay(1000);
                dArrayInsulationResistance[i] = randMToN(19, 21); //число полученное с COM-порта
                break;
            case 3:
                delay(1000);
                dArrayInsulationResistance[i] = randMToN(19, 21); //число полученное с COM-порта
                break;
            default:
                return;
                break;
            }

            if(ui->rbModeDiagnosticManual->isChecked())
                ui->cbInsulationResistance->setCurrentIndex(i);
            else
                ui->cbSubParamsAutoMode->setCurrentIndex(i);

            str = tr("Сопротивление цепи \"%0\" = <b>%1</b> МОм.").arg(battery[iBatteryIndex].str_isolation_resistance[i]).arg(dArrayInsulationResistance[i]);
            QLabel * label = findChild<QLabel*>(tr("labelInsulationResistance%0").arg(i));
            if (dArrayInsulationResistance[i] > settings.isolation_resistance_limit) {
                str += " Не норма.";
                color = "red";
            } else
                color = "green";
            label->setText(str);
            label->setStyleSheet("QLabel { color : "+color+"; }");
            Log(str, color);
            ui->btnBuildReport->setEnabled(true);
            if (dArrayInsulationResistance[i] > settings.isolation_resistance_limit) {
                if (QMessageBox::question(this, "Внимание - "+ui->rbInsulationResistance->text(), tr("%0 Продолжить?").arg(str), tr("Да"), tr("Нет"))) {
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

    Log(tr("Проверка завершена - %1").arg(ui->rbInsulationResistance->text()), "blue");

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
