#include <QDebug>
#include <QMessageBox>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "battery.h"
#include "settings.h"

extern Settings settings;

extern QVector<Battery> battery;

/* Алгоритм определения типа подключенной батареи описан в файле протокола информационного обмена с коробочкой.

Матрица типов подключенных батарей, в зависимости от электрических цепей:

                            Полярность прямая   обратная
Напряжение БП УУТББ есть    9ER20P-20 в2        9ЕR14PS-24 в2
Напряжение БП УУТББ нет     9ER20P-20           9ЕR14PS-24 или 9ER14P-24 - неоднозначность
Напряжение цепи 28 есть     9ER20P-28           х

*/

//QString type[2][3]={{"9ER20P-20 УУТББ", "9ER20P-20", "9ER20P-28"},{"9ЕR14PS-24 УУТББ", "9ЕR14PS-24 или 9ER14P-24", "???"}};
//QString type[2][3]={{battery[0].str_type_name+" УУТББ", battery[0].str_type_name, battery[3].str_type_name},{battery[1].str_type_name+" УУТББ", battery[1].str_type_name+" или "+battery[2].str_type_name, "???"}};

void MainWindow::on_btnCheckConnectedBattery_clicked()
{
    quint16 U1=settings.voltage_circuit_type/settings.coefADC1[settings.board_counter] + settings.offsetADC1[settings.board_counter]; // код приличного напряжения цепи.
    quint16 U2=settings.voltage_power_uutbb/settings.coefADC1[settings.board_counter] + settings.offsetADC1[settings.board_counter]; // код наличия напряжения БП.
    int x=0, y=0; // индексы в матрице
    quint16 polar; // полярность батареи
    quint16 typeb=0; // напряжение цепи 28
    quint16 uocpb; // напряжение БП УУТББ
    //int ret=0;
    iPowerState = 0; /// сбрасываем состояние включенного/отключенного источника питания

    ui->menuBar->setDisabled(true); /// запрещаем верхнее меню
    ui->groupBoxCOMPort->setDisabled(true); /// скрываем выбор сом-порта
    ui->groupBoxDiagnosticDevice->setDisabled(true); /// скрываем параметры проверяемой батареи
    ui->groupBoxDiagnosticMode->setDisabled(true); /// скрываем выбор режима проверки
    ui->groupBoxCheckParams->setDisabled(true); /// скрываем проверяемые параметры ручного режима
    ui->widgetCheckParamsButtons->setDisabled(true); /// скрываем кнопки ручного режима
    ui->groupBoxCheckParamsAutoMode->setDisabled(true); /// скрываем проверяемые параметры автоматического режима
    //qDebug()<<U1<<U2;

    if(loop.isRunning()){qDebug()<<"loop.isRunning()!"; return;} //  костыль: если цикл уже работает - выйти обратно
    timerPing->stop(); // остановить пинг

    /// при наличии галки имитатора, выводим сообщение о необходимости включить источник питания
    if(ui->cbIsImitator->isChecked() and iPowerState != 1) {
        QMessageBox::information(this, tr("Внимание! - %0").arg("Проверка батареи"), tr("Перед проверкой необходимо включить источник питания!"));
        iPowerState = 1; /// состояние включенного источника питания
    }

    ui->statusBar->showMessage(tr("Проверка типа подключенной батареи ..."));
    ui->progressBar->setRange(0,0);

    // сбросить коробочку
    baSendArray = (baSendCommand="IDLE")+"#"; // подготовить буфер для передачи
    sendSerialData(); // послать baSendArray в порт
    // ждём ответа. по сигналу о готовности принятых данных или по таймауту, вывалимся из цикла
    if(loop.exec()) goto stop; // если не ноль (ошибка таймаута) - вывалиться из режима. если 0, то приняли данные из порта

    baSendArray=(baSendCommand="Polar")+"#";
    QTimer::singleShot(settings.delay_after_IDLE_before_other, this, SLOT(sendSerialData())); // послать baSendArray в порт через некоторое время
    if(loop.exec()) goto stop; // если ошибка - вывалиться из режима

    baSendArray=baSendCommand+"?#";
    QTimer::singleShot(settings.delay_after_start_before_request_ADC1, this, SLOT(sendSerialData()));
    if(loop.exec()) goto stop; // если ошибка - вывалиться из режима
    polar = getRecvData(baRecvArray); // получить данные опроса

    baSendArray = (baSendCommand="IDLE")+"#";
    QTimer::singleShot(settings.delay_after_request_before_next_ADC1, this, SLOT(sendSerialData()));
    if(loop.exec()) goto stop; // если ошибка - вывалиться из режима

    if(polar == 0) // полярность прямая
    {
        x=0;
        baSendArray=(baSendCommand="TypeB")+" 28#";
        QTimer::singleShot(settings.delay_after_IDLE_before_other, this, SLOT(sendSerialData()));
        if(loop.exec()) goto stop;

        baSendArray=baSendCommand+"?#";
        QTimer::singleShot(settings.delay_after_start_before_request_ADC1, this, SLOT(sendSerialData()));
        if(loop.exec()) goto stop;
        typeb = getRecvData(baRecvArray);
        //qDebug()<<"onstateCheckTypeBPoll" << typeb<<(float)(typeb*coefADC1)<<"U";

        baSendArray = (baSendCommand="IDLE")+"#";
        QTimer::singleShot(settings.delay_after_request_before_next_ADC1, this, SLOT(sendSerialData()));
        if(loop.exec()) goto stop;
    }
    if(polar == 1) // полярность обратная
    {
        x=1;
    }

    baSendArray=(baSendCommand="UocPB")+"#";
    QTimer::singleShot(settings.delay_after_IDLE_before_other, this, SLOT(sendSerialData()));
    if(loop.exec()) goto stop;

    baSendArray=baSendCommand+"?#";
    QTimer::singleShot(settings.delay_after_start_before_request_ADC1, this, SLOT(sendSerialData()));
    if(loop.exec()) goto stop;
    uocpb = getRecvData(baRecvArray);

    // сбросить коробочку
    baSendArray = (baSendCommand="IDLE")+"#";
    QTimer::singleShot(settings.delay_after_request_before_next_ADC1, this, SLOT(sendSerialData()));
    if(loop.exec()) goto stop; // если ошибка - вывалиться из режима

    if(uocpb > U2) // если есть напряжение, то УУТББ
    {
        y=0;
    }
    else // иначе УУТББ нету
    {
        y=1;
    }
    if(typeb > U1) // если есть приличное напряжение, Battery 9ER20P-28
    {
        y=2;
    }

    //qDebug()<<x<<y;
    if((0==x) && (0==y)) // 9ER20P-20 УУТББ
    {
        if(!((ui->comboBoxBatteryList->currentIndex()==0) && ui->cbIsUUTBB->isChecked()))
        {
            Log("Подключена батарея "+battery[0].str_type_name+" УУТББ, но выбрана "+ui->comboBoxBatteryList->currentText()+(ui->cbIsUUTBB->isChecked()?" УУТББ":""), "red");
            QMessageBox::information(this, "Проверка подключенной батареи", "Подключенная батарея "+battery[0].str_type_name+" УУТББ не соответствует выбранной");
        }
        else
        {
            Log("Подключена батарея "+battery[0].str_type_name+" УУТББ", "blue");
            ui->menuBar->setEnabled(true); /// запретить/разрешить верхнее меню
            ui->menuPUTSU->setEnabled(true); /// разрешаем меню ПУ ТСУ
            ui->groupBoxDiagnosticMode->setEnabled(true); /// разрешаем выбрать режим диагностики
            if (ui->rbModeDiagnosticAuto->isChecked())
                ui->groupBoxCheckParamsAutoMode->setEnabled(true); /// разрешаем выбрать начальный параметр проверки автоматического режима
            else {
                ui->groupBoxCheckParams->setEnabled(true); /// разрешить выбрать параметр проверки ручного режима
                ui->widgetCheckParamsButtons->setEnabled(true); /// скрываем кнопки ручного режима
            }
        }
    }
    if((0==x) && (1==y)) // 9ER20P-20
    {
        if(!((ui->comboBoxBatteryList->currentIndex()==0) && !ui->cbIsUUTBB->isChecked()))
        {
            Log("Подключена батарея "+battery[0].str_type_name+", но выбрана "+ui->comboBoxBatteryList->currentText()+(ui->cbIsUUTBB->isChecked()?" УУТББ":""), "red");
            QMessageBox::information(this, "Проверка подключенной батареи", "Подключенная батарея "+battery[0].str_type_name+" не соответствует выбранной");
        }
        else
        {
            Log("Подключена батарея "+battery[0].str_type_name, "blue");
            ui->groupBoxDiagnosticMode->setEnabled(true); /// разрешаем выбрать режим диагностики
            if (ui->rbModeDiagnosticAuto->isChecked())
                ui->groupBoxCheckParamsAutoMode->setEnabled(true); /// разрешаем выбрать начальный параметр проверки автоматического режима
            else {
                ui->groupBoxCheckParams->setEnabled(true); /// разрешить выбрать параметр проверки ручного режима
                ui->widgetCheckParamsButtons->setEnabled(true); /// скрываем кнопки ручного режима
            }
        }
    }
    if((1==x) && (0==y)) // 9ЕR14PS-24 УУТББ
    {
        if(!((ui->comboBoxBatteryList->currentIndex()==1) && ui->cbIsUUTBB->isChecked()))
        {
            Log("Подключена батарея "+battery[1].str_type_name+" УУТББ, но выбрана "+ui->comboBoxBatteryList->currentText()+(ui->cbIsUUTBB->isChecked()?" УУТББ":""), "red");
            QMessageBox::information(this, "Проверка подключенной батареи", "Подключенная батарея "+battery[1].str_type_name+" УУТББ не соответствует выбранной");
        }
        else
        {
            Log("Подключена батарея "+battery[1].str_type_name+" УУТББ", "blue");
            ui->menuBar->setEnabled(true); /// разрешить верхнее меню
            ui->menuPUTSU->setEnabled(true); /// разрешаем меню ПУ ТСУ
            ui->groupBoxDiagnosticMode->setEnabled(true); /// разрешаем выбрать режим диагностики
            if (ui->rbModeDiagnosticAuto->isChecked())
                ui->groupBoxCheckParamsAutoMode->setEnabled(true); /// разрешаем выбрать начальный параметр проверки автоматического режима
            else {
                ui->groupBoxCheckParams->setEnabled(true); /// разрешить выбрать параметр проверки ручного режима
                ui->widgetCheckParamsButtons->setEnabled(true); /// скрываем кнопки ручного режима
            }
        }
    }
    if((1==x) && (1==y)) //9ЕR14PS-24 или 9ER14P-24
    {
        if(!(((ui->comboBoxBatteryList->currentIndex()==1) || (ui->comboBoxBatteryList->currentIndex()==2)) && !ui->cbIsUUTBB->isChecked()))
        {
            Log("Подключена батарея "+battery[1].str_type_name+" или "+battery[2].str_type_name+", но выбрана "+ui->comboBoxBatteryList->currentText()+(ui->cbIsUUTBB->isChecked()?" УУТББ":""), "red");
            QMessageBox::information(this, "Проверка подключенной батареи", "Подключенная батарея "+battery[1].str_type_name+" или "+battery[2].str_type_name+" не соответствует выбранной");
        }
        else
        {
            Log("Подключена батарея "+battery[1].str_type_name+" или "+battery[2].str_type_name, "blue");
            ui->groupBoxDiagnosticMode->setEnabled(true); /// разрешаем выбрать режим диагностики
            if (ui->rbModeDiagnosticAuto->isChecked())
                ui->groupBoxCheckParamsAutoMode->setEnabled(true); /// разрешаем выбрать начальный параметр проверки автоматического режима
            else {
                ui->groupBoxCheckParams->setEnabled(true); /// разрешить выбрать параметр проверки ручного режима
                ui->widgetCheckParamsButtons->setEnabled(true); /// скрываем кнопки ручного режима
            }
            QMessageBox::information(this, "Проверка подключенной батареи", "Подключена батарея "+battery[1].str_type_name+" или "+battery[2].str_type_name+"!");
        }
    }
    if(2==y)
    {
        if(ui->comboBoxBatteryList->currentIndex()!=3) // если выбранная батарея не соответствует подключенной
        {
            Log("Подключена батарея "+battery[3].str_type_name+", но выбрана "+ui->comboBoxBatteryList->currentText()+(ui->cbIsUUTBB->isChecked()?" УУТББ":""), "red");
            QMessageBox::information(this, "Проверка подключенной батареи", "Подключенная батарея "+battery[3].str_type_name+" не соответствует выбранной");
        }
        else
        {
            Log("Подключена батарея "+battery[3].str_type_name, "blue");
            ui->groupBoxDiagnosticMode->setEnabled(true); /// разрешаем выбрать режим диагностики
            if (ui->rbModeDiagnosticAuto->isChecked())
                ui->groupBoxCheckParamsAutoMode->setEnabled(true); /// разрешаем выбрать начальный параметр проверки автоматического режима
            else {
                ui->groupBoxCheckParams->setEnabled(true); /// разрешить выбрать параметр проверки ручного режима
                ui->widgetCheckParamsButtons->setEnabled(true); /// скрываем кнопки ручного режима
            }
        }
    }
stop:
    ui->groupBoxCOMPort->setEnabled(true); /// разрешаем выбор сом-порта
    ui->groupBoxDiagnosticDevice->setEnabled(true); /// разрешаем параметры проверяемой батареи
    timerPing->start(delay_timerPing); // запустить пинг по выходу из режима
    baSendArray.clear(); // надо ли?
    baSendCommand.clear();
    baRecvArray.clear();

    ui->progressBar->setMaximum(1); // stop progress
    ui->progressBar->reset();
}
