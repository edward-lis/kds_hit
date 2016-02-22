#include <QDebug>
#include <QMessageBox>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "battery.h"

extern QVector<Battery> battery;

// Нажата кнопка проверки сопротивления изоляции
void MainWindow::on_btnInsulationResistance_clicked()
{
    QString str_num; // номер цепи
    quint16 u=0; // полученный код АЦП
    float resist=0; // получившееся сопротивление
    int j=0;
    int ret=0; // код возврата ошибки
    QString strResist; // строка с получившимся значением

    if(loop.isRunning()){qDebug()<<"loop.isRunning()!"; return;} // костыль: если цикл уже работает - выйти обратно
    ui->btnInsulationResistance->setEnabled(false); // на время проверки запретить кнопку
    timerPing->stop(); // остановить пинг

    baSendArray.clear();
    baSendCommand.clear();
    baRecvArray.clear();

    ui->statusBar->showMessage(tr("Проверка сопротивления изоляции ..."));
//    ui->progressBar->setValue(ui->progressBar->value()+1);
    Log(tr("Проверка сопротивления изоляции"), "blue");

    // сбросить коробочку
    baSendArray = (baSendCommand="IDLE")+"#"; // подготовить буфер для передачи
    sendSerialData(); // послать baSendArray в порт
    // ждём ответа. по сигналу о готовности принятых данных или по таймауту, вывалимся из цикла
    ret=loop.exec();
    if(ret) goto stop; // если не ноль (ошибка таймаута) - вывалиться из режима. если 0, то приняли данные из порта

    str_num.sprintf(" %02i", battery[iBatteryIndex].isolation_resistance_nn[ui->cbInsulationResistance->currentIndex()]); // напечатать номер точки измерения изоляции
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
    if(resist > 1000000)
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
        resist = resist/1000;
        strResist = QString::number(resist, 'f', 2) + "Ом, ";
    }


    qDebug()<<" u=0x"<<qPrintable(QString::number(u, 16))<<" resist="<<resist;

    // сбросить коробочку
    baSendArray = (baSendCommand="IDLE")+"#";
    QTimer::singleShot(settings.delay_after_request_before_next_ADC2, this, SLOT(sendSerialData()));
    ret=loop.exec();
    if(ret) goto stop; // если ошибка - вывалиться из режима

    // если отладочный режим, напечатать отладочную инфу
    if(bDeveloperState)
    {
        Log("Сопротивление изоляции: " + ui->cbInsulationResistance->currentText() + "=" + strResist + "код АЦП= 0x" + QString("%1").arg((ushort)u, 0, 16), "green");
    }
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

/*
 * Сопротивление изоляции
 */
void MainWindow::checkInsulationResistance()
{
    /*if (((QPushButton*)sender())->objectName() == "btnInsulationResistance") {
        iStepInsulationResistance = 1;
        bState = false;
        ui->btnInsulationResistance_2->setEnabled(false);
    }
    if (((QPushButton*)sender())->objectName() == "btnInsulationResistance_2")
        bState = false;*/
    if (!bState) return;
    ui->groupBoxCOMPort->setEnabled(false);
    ui->groupBoxDiagnosticDevice->setEnabled(false);
    ui->groupBoxDiagnosticMode->setEnabled(false);
    ui->tabWidget->addTab(ui->tabInsulationResistance, ui->rbInsulationResistance->text());
    ui->tabWidget->setCurrentIndex(ui->tabWidget->count()-1);
    Log(tr("Проверка начата - %1").arg(ui->rbInsulationResistance->text()), "blue");
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
                param = qrand()%25; //число полученное с COM-порта
                break;
            case 1:
                delay(1000);
                param = qrand()%25;; //число полученное с COM-порта
                break;
            case 2:
                delay(1000);
                param = qrand()%25; //число полученное с COM-порта
                break;
            case 3:
                delay(1000);
                param = qrand()%25;; //число полученное с COM-порта
                break;
            default:
                return;
                break;
            }
            str = tr("%0 = <b>%1</b> В").arg(battery[iBatteryIndex].str_isolation_resistance[i]).arg(QString::number(param));
            QLabel * label = findChild<QLabel*>(tr("labelInsulationResistance%0").arg(i));
            color = (param > settings.isolation_resistance_limit) ? "red" : "green";
            label->setText(str);
            label->setStyleSheet("QLabel { color : "+color+"; }");
            Log(str, color);
            ui->btnBuildReport->setEnabled(true);
            if (param > settings.isolation_resistance_limit) {
                if (QMessageBox::question(this, "Внимание - "+ui->rbInsulationResistance->text(), tr("%1 \nпродолжить?").arg(str), tr("Да"), tr("Нет"))) {
                    bState = false;
                    return;
                } /*else {
                    ui->rbModeDiagnosticManual->setChecked(true);
                    ui->rbModeDiagnosticAuto->setEnabled(false);
                    ui->btnVoltageOnTheHousing_2->setEnabled(true);
                }*/
            }
            ui->cbSubParamsAutoMode->setCurrentIndex(i+1);
            ui->progressBar->setValue(i+1);
            //iStepVoltageOnTheHousing++;
        }
        if (ui->rbModeDiagnosticAuto->isChecked())
             bCheckCompleteInsulationResistance = true;
        break;
    case 1:
        if (!bState) return;
        Log("Действия проверки.", "green");
        delay(1000);
        break;
    case 2:
        if (!bState) return;
        Log("Действия проверки.", "green");
        delay(1000);
        break;
    case 3:
        if (!bState) return;
        Log("Действия проверки.", "green");
        delay(1000);
        break;
    default:
        break;
    }
    Log(tr("Проверка завершена - %1").arg(ui->rbInsulationResistance->text()), "blue");
    //iStepInsulationResistance = 1;
    ui->rbOpenCircuitVoltageGroup->setEnabled(true);
    ui->groupBoxCOMPort->setEnabled(true);
    ui->groupBoxDiagnosticDevice->setEnabled(true);
    ui->groupBoxDiagnosticMode->setEnabled(true);
    ui->cbParamsAutoMode->setCurrentIndex(ui->cbParamsAutoMode->currentIndex()+1); // переключаем комбокс на следующий режим
}
