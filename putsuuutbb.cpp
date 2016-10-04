#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "settings.h"

extern Settings settings;


/*!
 * \brief MainWindow::on_actionPUTSUOn_triggered
 */
void MainWindow::on_actionPUTSUOn_triggered()
{
    qDebug() << "PUTSU Send StubX6#";
    ui->statusBar->showMessage(tr("Включение ПУ ТСУ ..."));

    if(loop.isRunning()){qDebug()<<"loop.isRunning()!"; return;} // если цикл уже работает - выйти обратно
    timerPing->stop(); // остановить пинг

    // сбросить коробочку
    baSendArray = (baSendCommand="IDLE")+"#"; // подготовить буфер для передачи
    sendSerialData(); // послать baSendArray в порт
    // ждём ответа. по сигналу о готовности принятых данных или по таймауту, вывалимся из цикла
    if(loop.exec()) goto stop; // если не ноль (ошибка таймаута) - вывалиться из режима. если 0, то приняли данные из порта

    baSendArray=(baSendCommand="StubX6")+"#";
    QTimer::singleShot(settings.delay_after_IDLE_before_other, this, SLOT(sendSerialData())); // послать baSendArray в порт через некоторое время
    if(loop.exec()) goto stop; // если ошибка - вывалиться из режима

    Log("[ПУ ТСУ]: включен.", "green");
    ui->actionPUTSUOn->setDisabled(true);
    ui->actionPUTSUOff->setEnabled(true);

stop:
    timerPing->start(delay_timerPing); // запустить пинг по выходу из режима
    baSendArray.clear(); // надо ли?
    baSendCommand.clear();
    baRecvArray.clear();

}

/*!
 * \brief MainWindow::on_actionPUTSUOff_triggered
 */
void MainWindow::on_actionPUTSUOff_triggered()
{
    qDebug() << "PUTSU Send Idle#";
    ui->statusBar->showMessage(tr("Отключение ПУ ТСУ ..."));

    if(loop.isRunning()){qDebug()<<"loop.isRunning()!"; return;} // если цикл уже работает - выйти обратно
    timerPing->stop(); // остановить пинг

    // сбросить коробочку
    baSendArray = (baSendCommand="IDLE")+"#"; // подготовить буфер для передачи
    sendSerialData(); // послать baSendArray в порт
    Log("[ПУ ТСУ]: отключен.", "green");
    ui->actionPUTSUOn->setEnabled(true);
    ui->actionPUTSUOff->setDisabled(true);
    // ждём ответа. по сигналу о готовности принятых данных или по таймауту, вывалимся из цикла
    if(loop.exec()) goto stop; // если не ноль (ошибка таймаута) - вывалиться из режима. если 0, то приняли данные из порта

stop:
    timerPing->start(delay_timerPing); // запустить пинг по выходу из режима
    baSendArray.clear(); // надо ли?
    baSendCommand.clear();
    baRecvArray.clear();
    Log("[ПУ ТСУ]: отключен.", "red");
}

