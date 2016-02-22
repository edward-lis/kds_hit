#include <QDebug>
#include <QMessageBox>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "battery.h"

extern QVector<Battery> battery;

// Нажата кнопка проверки напряжения разомкнутых цепей групп
void MainWindow::on_btnOpenCircuitVoltageGroup_clicked()
{
    quint16 codeADC=0; // принятый код АЦП
    float fU=0; // принятое напряжение в вольтах
    // код порогового напряжения = пороговое напряжение В / коэфф. (вес разряда) + смещение (в коде)
    quint16 codeLimit=settings.opencircuitgroup_limit_min/settings.coefADC1 + settings.offsetADC1; // код, пороговое напряжение.
    int ret=0; // код возврата ошибки
    QString str_num; // номер цепи

    if(loop.isRunning()){qDebug()<<"loop.isRunning()!"; return;} // костыль: если цикл уже работает - выйти обратно
    timerPing->stop(); // остановить пинг

    baSendArray.clear();
    baSendCommand.clear();
    baRecvArray.clear();

    ui->statusBar->showMessage(tr("Проверка напряжения разомкнутых цепей групп ..."));
    Log(tr("Проверка напряжения разомкнутых цепей групп"), "blue");

    // сбросить коробочку
    baSendArray = (baSendCommand="IDLE")+"#"; // подготовить буфер для передачи
    sendSerialData(); // послать baSendArray в порт
    // ждём ответа. по сигналу о готовности принятых данных или по таймауту, вывалимся из цикла
    ret=loop.exec();
    if(ret) goto stop; // если не ноль (ошибка таймаута) - вывалиться из режима. если 0, то приняли данные из порта

    // !!! если нечего проверять, список пуст - проверить алгоритм
    // Пробежимся по списку цепей
    for(int i=1; i < modelOpenCircuitVoltageGroup->rowCount(); i++)
    {
        QStandardItem *sitm = modelOpenCircuitVoltageGroup->item(i, 0);
        Qt::CheckState checkState = sitm->checkState();
        if (checkState != Qt::Checked) continue;

        // собрать режим
        str_num.sprintf(" %02i", i); // напечатать номер цепи
        baSendArray=(baSendCommand="UocG")+str_num.toLocal8Bit()+"#";
        if(bDeveloperState) Log(QString("Sending ") + qPrintable(baSendArray), "blue");
        QTimer::singleShot(settings.delay_after_IDLE_before_other, this, SLOT(sendSerialData()));
        ret=loop.exec();
        if(ret) goto stop;

        // опросить
        baSendArray=baSendCommand+"?#";
        QTimer::singleShot(settings.delay_after_start_before_request_ADC1, this, SLOT(sendSerialData()));
        ret=loop.exec();
        if(ret) goto stop;
        codeADC = getRecvData(baRecvArray);

        fU = ((codeADC-settings.offsetADC1)*settings.coefADC1); // напряжение в вольтах

        if(bDeveloperState)
            Log("Цепь "+battery[iBatteryIndex].circuitgroup[i-1]+" Receive "+qPrintable(baRecvArray)+" codeADC1=0x"+QString("%1").arg((ushort)codeADC, 0, 16), "blue");

        // проанализировать результаты
        if(codeADC >= codeLimit) // напряжение больше (норма)
        {
            Log("Напряжение цепи "+battery[iBatteryIndex].circuitgroup[i-1]+" = "+QString::number(fU, 'f', 2)+" В.  Норма.", "blue");
            // если ручной режим, то выдать окно сообщения, и только потом разобрать режим измерения.
            if(bModeManual) QMessageBox::information(this, tr("Напряжение разомкнутой цепи группы"), tr("Напряжение цепи ")+battery[iBatteryIndex].circuitgroup[i-1]+" = "+QString::number(fU, 'f', 2)+" В\nНорма");
        }
        else // напряжение меньше (не норма)
        {
            Log("Напряжение цепи "+battery[iBatteryIndex].circuitgroup[i-1]+" = "+QString::number(fU, 'f', 2)+" В.  Не норма!.", "red");
            // если ручной режим, то выдать окно сообщения, и только потом разобрать режим измерения.
            if(bModeManual) QMessageBox::information(this, tr("Напряжение разомкнутой цепи группы"), tr("Напряжение цепи ")+battery[iBatteryIndex].circuitgroup[i-1]+" = "+QString::number(fU, 'f', 2)+" В\nНе норма!");
            // !!! добавить цепь в список неисправных, запрет проверки цепи под нагрузкой
        }

        // разобрать режим
        baSendArray = (baSendCommand="IDLE")+"#";
        QTimer::singleShot(settings.delay_after_request_before_next_ADC1, this, SLOT(sendSerialData()));
        ret=loop.exec();
        if(ret) goto stop;
    }

stop:
    // если отладочный режим, напечатать отладочную инфу
    if(bDeveloperState)
    {
        if(ret==1) Log(tr("Timeout!"), "red");
        else if(ret==2) Log(tr("Incorrect reply!"), "red");
    }

    timerPing->start(delay_timerPing); // запустить пинг по выходу из режима
    baSendArray.clear(); // очистить буфера команд.
    baSendCommand.clear();
    baRecvArray.clear();

}

// слот вызывается при изменении чекбоксов элементов списка комбобокса
void MainWindow::itemChangedOpenCircuitVoltageGroup(QStandardItem* itm)
{
    qDebug() << "modelOpenCircuitVoltageGroup->rowCount()=" << modelOpenCircuitVoltageGroup->rowCount();
    int count = 0;
    for(int i=1; i < modelOpenCircuitVoltageGroup->rowCount(); i++)
    {
        QStandardItem *sitm = modelOpenCircuitVoltageGroup->item(i, 0);
        Qt::CheckState checkState = sitm->checkState();
        if (checkState == Qt::Checked)
            count++;
    }
    qDebug() << "countOpenCircuitVoltageGroup=" << count;
    ui->cbOpenCircuitVoltageGroup->setItemText(0, tr("Выбрано: %0 из %1").arg(count).arg(modelOpenCircuitVoltageGroup->rowCount()-1));
    ui->cbOpenCircuitVoltageGroup->setCurrentIndex(0);
}

/*
 * Напряжение разомкнутой цепи группы
 */
void MainWindow::checkOpenCircuitVoltageGroup()
{
    /*if (((QPushButton*)sender())->objectName() == "btnOpenCircuitVoltageGroup") {
        iStepOpenCircuitVoltageGroup = 1;
        bState = false;
        ui->btnOpenCircuitVoltageGroup_2->setEnabled(false);
    }
    if (((QPushButton*)sender())->objectName() == "btnOpenCircuitVoltageGroup_2")
        bState = false;*/
    if (!bState) return;
    ui->groupBoxCOMPort->setEnabled(false);
    ui->groupBoxDiagnosticDevice->setEnabled(false);
    ui->groupBoxDiagnosticMode->setEnabled(false);
    ui->tabWidget->addTab(ui->tabOpenCircuitVoltageGroup, ui->rbOpenCircuitVoltageGroup->text());
    ui->tabWidget->setCurrentIndex(ui->tabWidget->count()-1);
    Log(tr("Проверка начата - %1").arg(ui->rbOpenCircuitVoltageGroup->text()), "blue");
    iCurrentStep = (ui->rbModeDiagnosticAuto->isChecked()) ? ui->cbSubParamsAutoMode->currentIndex() : ui->cbOpenCircuitVoltageGroup->currentIndex();
    iMaxSteps = (ui->rbModeDiagnosticAuto->isChecked()) ? ui->cbSubParamsAutoMode->count() : ui->cbOpenCircuitVoltageGroup->count();
    ui->progressBar->setMaximum(iMaxSteps);
    ui->progressBar->setValue(iCurrentStep);
    switch (iBatteryIndex) {
    case 0: //9ER20P-20
        for (int i = iCurrentStep; i < iMaxSteps; i++) {
            if (!bState) return;
            switch (i) {
            case 0:
                delay(1000);
                param = qrand()%40+10; //число полученное с COM-порта
                break;
            case 1:
                delay(1000);
                param = qrand()%40+10; //число полученное с COM-порта
                break;
            case 2:
                delay(1000);
                param = qrand()%40+10; //число полученное с COM-порта
                break;
            case 3:
                delay(1000);
                param = qrand()%40+10; //число полученное с COM-порта
                break;
            case 4:
                delay(1000);
                param = qrand()%40+10; //число полученное с COM-порта
                break;
            case 5:
                delay(1000);
                param = qrand()%40+10; //число полученное с COM-порта
                break;
            case 6:
                delay(1000);
                param = qrand()%40+10; //число полученное с COM-порта
                break;
            case 7:
                delay(1000);
                param = qrand()%40+10; //число полученное с COM-порта
                break;
            case 8:
                delay(1000);
                param = qrand()%40+10; //число полученное с COM-порта
                break;
            case 9:
                delay(1000);
                param = qrand()%40+10; //число полученное с COM-порта
                break;
            case 10:
                delay(1000);
                param = qrand()%40+10; //число полученное с COM-порта
                break;
            case 11:
                delay(1000);
                param = qrand()%40+10; //число полученное с COM-порта
                break;
            case 12:
                delay(1000);
                param = qrand()%40+10; //число полученное с COM-порта
                break;
            case 13:
                delay(1000);
                param = qrand()%40+10; //число полученное с COM-порта
                break;
            case 14:
                delay(1000);
                param = qrand()%40+10; //число полученное с COM-порта
                break;
            case 15:
                delay(1000);
                param = qrand()%40+10; //число полученное с COM-порта
                break;
            case 16:
                delay(1000);
                param = qrand()%40+10; //число полученное с COM-порта
                break;
            case 17:
                delay(1000);
                param = qrand()%40+10; //число полученное с COM-порта
                break;
            case 18:
                delay(1000);
                param = qrand()%40+10; //число полученное с COM-порта
                break;
            case 19:
                delay(1000);
                param = qrand()%40+10; //число полученное с COM-порта
                break;
            default:
                //return;
                break;
            }
            str = tr("%0 = <b>%1</b> В").arg(battery[iBatteryIndex].circuitgroup[i]).arg(QString::number(param));
            QLabel * label = findChild<QLabel*>(tr("labelOpenCircuitVoltageGroup%0").arg(i));
            color = (param > settings.closecircuitgroup_limit) ? "red" : "green";
            label->setText(str);
            label->setStyleSheet("QLabel { color : "+color+"; }");
            Log(str, color);
            ui->btnBuildReport->setEnabled(true);
            if (param > settings.closecircuitgroup_limit) {
                if (QMessageBox::question(this, "Внимание - "+ui->rbOpenCircuitVoltageGroup->text(), tr("%1 \nпродолжить?").arg(str), tr("Да"), tr("Нет"))) {
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

    Log(tr("Проверка завершена - %1").arg(ui->rbOpenCircuitVoltageGroup->text()), "blue");
    iStepOpenCircuitVoltageGroup = 1;
    ui->rbClosedCircuitVoltageGroup->setEnabled(true);
    ui->groupBoxCOMPort->setEnabled(true);
    ui->groupBoxDiagnosticDevice->setEnabled(true);
    ui->groupBoxDiagnosticMode->setEnabled(true);
    ui->cbParamsAutoMode->setCurrentIndex(ui->cbParamsAutoMode->currentIndex()+1); // переключаем комбокс на следующий режим
}
