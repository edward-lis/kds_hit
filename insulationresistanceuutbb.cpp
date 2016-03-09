#include <QDebug>
#include <QMessageBox>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "battery.h"
#include "settings.h"

extern Settings settings;

extern QVector<Battery> battery;

// Нажата кнопка проверки сопротивления изоляции УУТББ
void MainWindow::on_btnInsulationResistanceUUTBB_clicked()
{
    //checkInsulationResistanceUUTBB(); return;
    QString str_num; // номер цепи
    quint16 u=0; // полученный код АЦП
    float resist=0; // получившееся сопротивление
    int j=0;
    int ret=0; // код возврата ошибки
    QString strResist; // строка с получившимся значением
//    int i=0; // номер цепи
    QLabel *label; // надпись в закладке

    /// по началу проверки очистим все label'ы и полученные результаты
    for (int i = 0; i < battery[iBatteryIndex].uutbb_resist.size(); i++) {
        dArrayInsulationResistanceUUTBB[i] = -1;
        label = findChild<QLabel*>(tr("labelInsulationResistanceUUTBB%0").arg(i));
        label->setStyleSheet("QLabel { color : black; }");
        label->clear();
        //??? if (i < battery[iBatteryIndex].uutbb_resist.size())
            label->setText(tr("%0) \"%1\"").arg(i+1).arg(battery[iBatteryIndex].uutbb_resist[i]));
    }

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
    ui->groupBoxCheckParamsAutoMode->setDisabled(bState);

    // откроем вкладку
    ui->tabWidget->addTab(ui->tabInsulationResistanceUUTBB, ui->rbInsulationResistanceUUTBB->text());
    ui->tabWidget->setCurrentIndex(ui->tabWidget->count()-1);
    Log(tr("Проверка начата - %1").arg(ui->rbInsulationResistanceUUTBB->text()), "blue");
    ui->statusBar->showMessage(tr("Проверка ")+ui->rbInsulationResistanceUUTBB->text()+" ...");

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

        //i=ui->cbOpenCircuitVoltageGroup->currentIndex();
        iCurrentStep=0; // в ручном начнём сначала
        iMaxSteps=modelInsulationResistanceUUTBB->rowCount()-1; // -1 с учётом первой строки в комбобоксе
    }
    else
    {
        ui->cbParamsAutoMode->setCurrentIndex(6); // переключаем режим комбокса на наш
        iCurrentStep = ui->cbSubParamsAutoMode->currentIndex();
        iMaxSteps = ui->cbSubParamsAutoMode->count();
    }

    // Пробежимся по списку точек измерения сопротивлений изоляции
    for(int i=iCurrentStep; i < iMaxSteps; i++)
    {
        if(bModeManual) // в ручном будем идти по чекбоксам
        {
            QStandardItem *sitm = modelInsulationResistanceUUTBB->item(i+1, 0); // взять очередной номер
            Qt::CheckState checkState = sitm->checkState(); // и его состояние
            if (checkState != Qt::Checked) continue; // если не отмечено, то следующий.
        }

        ui->progressBar->setMaximum(3); // установить кол-во ступеней прогресса
        ui->progressBar->reset();

        // очистить массивы посылки/приёма
        baSendArray.clear();
        baSendCommand.clear();
        baRecvArray.clear();

        // сбросить коробочку
        baSendArray = (baSendCommand="IDLE")+"#"; // подготовить буфер для передачи
        timerSend->start(settings.delay_after_request_before_next_ADC2); // послать baSendArray в порт
        // ждём ответа. по сигналу о готовности принятых данных или по таймауту, вывалимся из цикла
        ret=loop.exec();
        if(ret) goto stop; // если не ноль (ошибка таймаута) - вывалиться из режима. если 0, то приняли данные из порта
        ui->progressBar->setValue(ui->progressBar->value()+1);

        // собрать режим
        str_num.sprintf(" %02i", battery[iBatteryIndex].uutbb_resist_nn[i]); // напечатать номер точки измерения изоляции
        baSendArray=(baSendCommand="Rins")+str_num.toLocal8Bit()+"#";
        if(bDeveloperState) Log(QString("Sending ") + qPrintable(baSendArray), "blue");
        timerSend->start(settings.delay_after_IDLE_before_other);
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
        dArrayInsulationResistanceUUTBB[i] = resist;

        // если отладочный режим, напечатать отладочную инфу
        if(bDeveloperState)
        {
            Log("Сопротивление изоляции: " + battery[iBatteryIndex].uutbb_resist[i] + "=" + strResist + "код АЦП= 0x" + QString("%1").arg((ushort)u, 0, 16), "green");
        }
        if(settings.verbose) qDebug()<<" u=0x"<<qPrintable(QString::number(u, 16))<<" resist="<<resist;

        // напечатать рез-т в закладку и в журнал
        str = tr("%0) \"%1\" = <b>%2</b> МОм.").arg(i+1).arg(battery[iBatteryIndex].uutbb_resist[i]).arg(dArrayInsulationResistanceUUTBB[i], 0, 'f', 0);
        label = findChild<QLabel*>(tr("labelInsulationResistanceUUTBB%0").arg(i));
        if (dArrayInsulationResistanceUUTBB[i] < settings.uutbb_isolation_resist_limit) {
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

        if (dArrayInsulationResistanceUUTBB[i] < settings.uutbb_isolation_resist_limit) {
            if(!bModeManual)// если в автоматическом режиме
            {
                if (QMessageBox::question(this, "Внимание - "+ui->rbInsulationResistanceUUTBB->text(), tr("%0 Продолжить?").arg(str+" "+sResult), tr("Да"), tr("Нет"))) {
                    bState = false;
                    ui->groupBoxCOMPort->setDisabled(bState);
                    ui->groupBoxDiagnosticMode->setDisabled(bState);
                    ui->cbParamsAutoMode->setDisabled(bState);
                    ui->cbSubParamsAutoMode->setDisabled(bState);
                    ((QPushButton*)sender())->setText("Пуск");
                    // остановить текущую проверку, выход
                    bCheckInProgress = false;
                    ui->rbModeDiagnosticManual->setChecked(true);
                    break;
                }
            }
        }
    }// конец цикла обхода всех точек измерения сопротивления изоляции
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

// слот вызывается при изменении чекбоксов элементов списка комбобокса
void MainWindow::itemChangedInsulationResistanceUUTBB(QStandardItem* itm)
{
    itm->text(); /// чтобы небыло варнинга при компиляции на неиспользование itm
    int count = 0;
    for(int i=1; i < modelInsulationResistanceUUTBB->rowCount(); i++)
    {
        QStandardItem *sitm = modelInsulationResistanceUUTBB->item(i, 0);
        Qt::CheckState checkState = sitm->checkState();
        if (checkState == Qt::Checked)
            count++;
    }
    ui->cbInsulationResistanceUUTBB->setItemText(0, tr("Выбрано: %0 из %1").arg(count).arg(modelInsulationResistanceUUTBB->rowCount()-1));
    ui->cbInsulationResistanceUUTBB->setCurrentIndex(0);
}

/*
 * Сопротивление изоляции платы измерительной УУТББ
 */
void MainWindow::checkInsulationResistanceUUTBB()
{
    qDebug() << "sender=" << ((QPushButton*)sender())->objectName() << "bState=" << bState;
    ui->tabWidget->addTab(ui->tabInsulationResistanceUUTBB, ui->rbInsulationResistanceUUTBB->text());
    ui->tabWidget->setCurrentIndex(ui->tabWidget->count()-1);
    Log(tr("Проверка начата - %1").arg(ui->rbInsulationResistanceUUTBB->text()), "blue");

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
        ui->cbParamsAutoMode->setCurrentIndex(6); // переключаем режим комбокса на наш

    ui->groupBoxCOMPort->setDisabled(bState);
    ui->groupBoxDiagnosticDevice->setDisabled(bState);
    ui->groupBoxDiagnosticMode->setDisabled(bState);
    ui->cbParamsAutoMode->setDisabled(bState);
    ui->cbSubParamsAutoMode->setDisabled(bState);

    iCurrentStep = (ui->rbModeDiagnosticAuto->isChecked()) ? ui->cbSubParamsAutoMode->currentIndex() : 0;
    iMaxSteps = (ui->rbModeDiagnosticAuto->isChecked()) ? ui->cbSubParamsAutoMode->count() : ui->cbInsulationResistanceUUTBB->count();

    if (ui->rbModeDiagnosticManual->isChecked()) { /// для ручного режима свой максимум для прогресс бара
        int count = 0;
        for(int i = 0; i < iMaxSteps; i++) {
            QModelIndex index = ui->cbInsulationResistanceUUTBB->model()->index(i+1, 0);
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
            QModelIndex index = ui->cbInsulationResistanceUUTBB->model()->index(i+1, 0);
            if(index.data(Qt::CheckStateRole) == 2) { /// проходимся только по выбранным
                switch (i) {
                case 0:
                    delay(1000);
                    dArrayInsulationResistanceUUTBB[i] = randMToN(4, 6); //число полученное с COM-порта
                    break;
                case 1:
                    delay(1000);
                    dArrayInsulationResistanceUUTBB[i] = randMToN(4, 6); //число полученное с COM-порта
                    break;
                case 2:
                    delay(1000);
                    dArrayInsulationResistanceUUTBB[i] = randMToN(4, 6); //число полученное с COM-порта
                    break;
                case 3:
                    delay(1000);
                    dArrayInsulationResistanceUUTBB[i] = randMToN(4, 6); //число полученное с COM-порта
                    break;
                case 4:
                    delay(1000);
                    dArrayInsulationResistanceUUTBB[i] = randMToN(4, 6); //число полученное с COM-порта
                    break;
                case 5:
                    delay(1000);
                    dArrayInsulationResistanceUUTBB[i] = randMToN(4, 6); //число полученное с COM-порта
                    break;
                case 6:
                    delay(1000);
                    dArrayInsulationResistanceUUTBB[i] = randMToN(4, 6); //число полученное с COM-порта
                    break;
                case 7:
                    delay(1000);
                    dArrayInsulationResistanceUUTBB[i] = randMToN(4, 6); //число полученное с COM-порта
                    break;
                case 8:
                    delay(1000);
                    dArrayInsulationResistanceUUTBB[i] = randMToN(4, 6); //число полученное с COM-порта
                    break;
                case 9:
                    delay(1000);
                    dArrayInsulationResistanceUUTBB[i] = randMToN(4, 6); //число полученное с COM-порта
                    break;
                case 10:
                    delay(1000);
                    dArrayInsulationResistanceUUTBB[i] = randMToN(4, 6); //число полученное с COM-порта
                    break;
                case 11:
                    delay(1000);
                    dArrayInsulationResistanceUUTBB[i] = randMToN(4, 6); //число полученное с COM-порта
                    break;
                case 12:
                    delay(1000);
                    dArrayInsulationResistanceUUTBB[i] = randMToN(4, 6); //число полученное с COM-порта
                    break;
                case 13:
                    delay(1000);
                    dArrayInsulationResistanceUUTBB[i] = randMToN(4, 6); //число полученное с COM-порта
                    break;
                case 14:
                    delay(1000);
                    dArrayInsulationResistanceUUTBB[i] = randMToN(4, 6); //число полученное с COM-порта
                    break;
                case 15:
                    delay(1000);
                    dArrayInsulationResistanceUUTBB[i] = randMToN(4, 6); //число полученное с COM-порта
                    break;
                case 16:
                    delay(1000);
                    dArrayInsulationResistanceUUTBB[i] = randMToN(4, 6); //число полученное с COM-порта
                    break;
                case 17:
                    delay(1000);
                    dArrayInsulationResistanceUUTBB[i] = randMToN(4, 6); //число полученное с COM-порта
                    break;
                case 18:
                    delay(1000);
                    dArrayInsulationResistanceUUTBB[i] = randMToN(4, 6); //число полученное с COM-порта
                    break;
                case 19:
                    delay(1000);
                    dArrayInsulationResistanceUUTBB[i] = randMToN(4, 6); //число полученное с COM-порта
                    break;
                case 20:
                    delay(1000);
                    dArrayInsulationResistanceUUTBB[i] = randMToN(4, 6); //число полученное с COM-порта
                    break;
                case 21:
                    delay(1000);
                    dArrayInsulationResistanceUUTBB[i] = randMToN(4, 6); //число полученное с COM-порта
                    break;
                case 22:
                    delay(1000);
                    dArrayInsulationResistanceUUTBB[i] = randMToN(4, 6); //число полученное с COM-порта
                    break;
                case 23:
                    delay(1000);
                    dArrayInsulationResistanceUUTBB[i] = randMToN(4, 6); //число полученное с COM-порта
                    break;
                case 24:
                    delay(1000);
                    dArrayInsulationResistanceUUTBB[i] = randMToN(4, 6); //число полученное с COM-порта
                    break;
                case 25:
                    delay(1000);
                    dArrayInsulationResistanceUUTBB[i] = randMToN(4, 6); //число полученное с COM-порта
                    break;
                case 26:
                    delay(1000);
                    dArrayInsulationResistanceUUTBB[i] = randMToN(4, 6); //число полученное с COM-порта
                    break;
                case 27:
                    delay(1000);
                    dArrayInsulationResistanceUUTBB[i] = randMToN(4, 6); //число полученное с COM-порта
                    break;
                case 28:
                    delay(1000);
                    dArrayInsulationResistanceUUTBB[i] = randMToN(4, 6); //число полученное с COM-порта
                    break;
                case 29:
                    delay(1000);
                    dArrayInsulationResistanceUUTBB[i] = randMToN(4, 6); //число полученное с COM-порта
                    break;
                case 30:
                    delay(1000);
                    dArrayInsulationResistanceUUTBB[i] = randMToN(4, 6); //число полученное с COM-порта
                    break;
                case 31:
                    delay(1000);
                    dArrayInsulationResistanceUUTBB[i] = randMToN(4, 6); //число полученное с COM-порта
                    break;
                case 32:
                    delay(1000);
                    dArrayInsulationResistanceUUTBB[i] = randMToN(4, 6); //число полученное с COM-порта
                    break;
                default:
                    return;
                    break;
                }
                qDebug() << "dArrayInsulationResistanceUUTBB[" << i << "]=" << dArrayInsulationResistanceUUTBB[i];
                if(ui->rbModeDiagnosticAuto->isChecked())
                    ui->cbSubParamsAutoMode->setCurrentIndex(i);

                str = tr("\"%0\" = <b>%1</b> МОм.").arg(battery[iBatteryIndex].uutbb_resist[i]).arg(dArrayInsulationResistanceUUTBB[i]);
                QLabel * label = findChild<QLabel*>(tr("labelInsulationResistanceUUTBB%0").arg(i));
                if (dArrayInsulationResistanceUUTBB[i] > settings.uutbb_isolation_resist_limit) {
                    str += " Не норма.";
                    color = "red";
                } else
                    color = "green";

                label->setText(str);
                label->setStyleSheet("QLabel { color : "+color+"; }");
                Log(str, color);
                ui->btnBuildReport->setEnabled(true);
                if (dArrayInsulationResistanceUUTBB[i] > settings.uutbb_isolation_resist_limit) {
                    switch (QMessageBox::question(this, "Внимание - "+ui->rbInsulationResistanceUUTBB->text(), tr("Сопротивление цепи %0 Продолжить?").arg(str), tr("Да"), tr("Нет"))) {
                    case 2:
                        bState = false;
                        ui->groupBoxCOMPort->setDisabled(bState);
                        ui->groupBoxDiagnosticMode->setDisabled(bState);
                        ui->cbParamsAutoMode->setDisabled(bState);
                        ui->cbSubParamsAutoMode->setDisabled(bState);
                        ((QPushButton*)sender())->setText("Пуск");
                        return;
                        break;
                    default:
                        break;
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

    Log(tr("Проверка завершена - %1").arg(ui->rbInsulationResistanceUUTBB->text()), "blue");

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
