#include <QDebug>
#include <QMessageBox>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "battery.h"

extern QVector<Battery> battery;

// Нажата кнопка проверки сопротивления изоляции УУТББ
void MainWindow::on_btnInsulationResistanceUUTBB_clicked()
{
    checkInsulationResistanceUUTBB(); return;
    QString str_num; // номер цепи
    quint16 u=0; // полученный код АЦП
    float resist=0; // получившееся сопротивление
    int j=0;
    int ret=0; // код возврата ошибки

    if(loop.isRunning()){qDebug()<<"loop.isRunning()!"; return;} // костыль: если цикл уже работает - выйти обратно
    ui->btnInsulationResistance->setEnabled(false); // на время проверки запретить кнопку
    timerPing->stop(); // остановить пинг

    baSendArray.clear();
    baSendCommand.clear();
    baRecvArray.clear();

    ui->statusBar->showMessage(tr("Проверка сопротивления изоляции УУТББ ..."));
//    ui->progressBar->setValue(ui->progressBar->value()+1);
    Log(tr("Проверка сопротивления изоляции"), "blue");

    // Пробежимся по списку точек измерения сопротивлений изоляции
    for(int i=1; i < modelInsulationResistanceUUTBB->rowCount(); i++)
    {
        QStandardItem *sitm = modelInsulationResistanceUUTBB->item(i, 0);
        Qt::CheckState checkState = sitm->checkState();
        if (checkState != Qt::Checked) continue;

    // сбросить коробочку
    baSendArray = (baSendCommand="IDLE")+"#"; // подготовить буфер для передачи
    sendSerialData(); // послать baSendArray в порт
    // ждём ответа. по сигналу о готовности принятых данных или по таймауту, вывалимся из цикла
    ret=loop.exec();
    if(ret) goto stop; // если не ноль (ошибка таймаута) - вывалиться из режима. если 0, то приняли данные из порта

    str_num.sprintf(" %02i", battery[iBatteryIndex].uutbb_resist_nn[i-1]); // напечатать номер точки измерения изоляции
    baSendArray=(baSendCommand="Rins")+str_num.toLocal8Bit()+"#";
    if(bDeveloperState) Log(QString("Sending ") + qPrintable(baSendArray), "blue");
    QTimer::singleShot(settings.delay_after_IDLE_before_other, this, SLOT(sendSerialData()));
    ret=loop.exec();
    if(ret) goto stop;

    baSendArray=baSendCommand+"?#";
    QTimer::singleShot(settings.delay_after_start_before_request_ADC2, this, SLOT(sendSerialData()));
    ret=loop.exec();
    if(ret) goto stop; // если ошибка - вывалиться из режима
    u = getRecvData(baRecvArray); // получить данные опроса

    //qDebug()<<"u"<<qPrintable(QString::number(u, 16));

    // будем щщитать сразу в коде, без перехода в вольты
    // пробежимся по точкам ф-ии, и высчитаем сопротивление согласно напряжению
    for(j=0; j<settings.functionResist.size()-2; j++)
    {
        //qDebug()<<"u"<<qPrintable(QString::number(u, 16))<<qPrintable(QString::number(settings.functionResist[j].codeADC, 16))<<qPrintable(QString::number(settings.functionResist[j+1].codeADC, 16));
        if((u > settings.functionResist[j].codeADC) && (u <= settings.functionResist[j+1].codeADC))
        {
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
    // переведём в мегаомы
    resist = resist/1000000;

    //qDebug()<<" u=0x"<<qPrintable(QString::number(u, 16))<<" resist="<<resist;

    // сбросить коробочку
    baSendArray = (baSendCommand="IDLE")+"#";
    QTimer::singleShot(settings.delay_after_request_before_next_ADC2, this, SLOT(sendSerialData()));
    ret=loop.exec();
    if(ret) goto stop; // если ошибка - вывалиться из режима

    // если отладочный режим, напечатать отладочную инфу
    if(bDeveloperState)
    {
        Log("Сопротивление изоляции: " + battery[iBatteryIndex].uutbb_resist[i-1] + "=" + QString::number(resist, 'f', 2) + "Мом, " + "код АЦП= 0x" + QString("%1").arg((ushort)u, 0, 16), "green");
    }
    }// конец цикла обхода всех точек измерения сопротивления изоляции
stop:
    // если отладочный режим, напечатать отладочную инфу
    if(bDeveloperState)
    {
        if(ret==1) Log(tr("Timeout!"), "red");
        else if(ret==2) Log(tr("Incorrect reply!"), "red");
    }
    ui->btnInsulationResistance->setEnabled(true); // разрешить кнопку
    timerPing->start(delay_timerPing); // запустить пинг по выходу из режима
    baSendArray.clear(); // очистить буфера команд.
    baSendCommand.clear();
    baRecvArray.clear();
// !!! сбросить прогрессбар
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
