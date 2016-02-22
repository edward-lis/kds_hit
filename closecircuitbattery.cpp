#include <QDebug>
#include <QMessageBox>
#include <QDateTime>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "battery.h"

extern QVector<Battery> battery;

/*
 * Напряжение замкнутой цепи батареи
 */
void MainWindow::checkClosedCircuitVoltageBattery()
{
    if (((QPushButton*)sender())->objectName() == "btnClosedCircuitVoltageBattery") {
        iStepClosedCircuitVoltageBattery = 1;
        bState = false;
        //ui->btnClosedCircuitVoltageBattery_2->setEnabled(false);
    }
    if (((QPushButton*)sender())->objectName() == "btnClosedCircuitVoltageBattery_2")
        bState = false;
    if (!bState) return;
    ui->groupBoxCOMPort->setEnabled(false);
    ui->groupBoxDiagnosticDevice->setEnabled(false);
    ui->groupBoxDiagnosticMode->setEnabled(false);
    ui->tabWidget->addTab(ui->tabClosedCircuitVoltageBattery, ui->rbClosedCircuitVoltageBattery->text());
    ui->tabWidget->setCurrentIndex(ui->tabWidget->count()-1);
    Log(tr("Проверка начата - %1").arg(ui->rbClosedCircuitVoltageBattery->text()), "blue");
    switch (iBatteryIndex) {
    case 0: //9ER20P-20
        if (!bState) return;
        delay(1000);
        ui->labelClosedCircuitVoltageBattery->setText(tr("1) %2").arg(QString::number(param)));
        str = tr("1) между контактом 1 соединителя Х1 «1+» и контактом 1 соединителя Х3 «3-» = <b>%2</b>").arg(QString::number(param));
        Log(str, (param > 30.0) ? "red" : "green");

        if (param > 30.0) {
            ui->rbModeDiagnosticManual->setChecked(true);
            ui->rbModeDiagnosticAuto->setEnabled(false);
            if (QMessageBox::question(this, "Внимание - "+ui->rbClosedCircuitVoltageBattery->text(), tr("%1 \nпродолжить?").arg(str), tr("Да"), tr("Нет"))) {
                //ui->btnClosedCircuitVoltageBattery_2->setEnabled(true);
                bState = true;
                return;
            }
        }
        //ui->btnClosedCircuitVoltageBattery_2->setEnabled(false);
        if (ui->rbModeDiagnosticAuto->isChecked())
            bCheckCompleteClosedCircuitVoltageBattery = true;
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
    Log(tr("Проверка завершена - %1").arg(ui->rbClosedCircuitVoltageBattery->text()), "blue");
    iStepClosedCircuitVoltageBattery = 1;
    ui->rbInsulationResistanceMeasuringBoardUUTBB->setEnabled(true);
    ui->groupBoxCOMPort->setEnabled(true);
    ui->groupBoxDiagnosticDevice->setEnabled(true);
    ui->groupBoxDiagnosticMode->setEnabled(true);
}
