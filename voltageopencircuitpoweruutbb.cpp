#include <QDebug>
#include <QMessageBox>
#include <QDateTime>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "battery.h"

extern QVector<Battery> battery;

/*
 * Напряжение разомкнутой цепи блока питания
 */
void MainWindow::checkOpenCircuitVoltagePowerSupply()
{
    //if (ui->rbModeDiagnosticAuto->isChecked() and bStop) return;
    if (!bState) return;
    ui->groupBoxCOMPort->setEnabled(false);
    ui->groupBoxDiagnosticDevice->setEnabled(false);
    ui->groupBoxDiagnosticMode->setEnabled(false);
    ui->tabWidget->addTab(ui->tabOpenCircuitVoltagePowerSupply, ui->rbOpenCircuitVoltagePowerSupply->text());
    ui->tabWidget->setCurrentIndex(ui->tabWidget->count()-1);
    Log(tr("Проверка начата - %1").arg(ui->rbOpenCircuitVoltagePowerSupply->text()), "blue");
    switch (iBatteryIndex) {
    case 0: //9ER20P-20
        while (iStepOpenCircuitVoltagePowerSupply <= 1) {
            if (!bState) return;
            switch (iStepOpenCircuitVoltagePowerSupply) {
            case 1:
                delay(1000);
                //Log(tr("1) между точкой металлизации и контактом 1 соединителя Х1 «Х1+» = <b>%1</b>").arg(QString::number(paramInsulationResistance1)), color);
                break;
            default:
                break;
            }
            iStepOpenCircuitVoltagePowerSupply++;
        }
        if (ui->rbModeDiagnosticAuto->isChecked())
            bCheckCompleteOpenCircuitVoltagePowerSupply = true;
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
    Log(tr("Проверка завершена - %1").arg(ui->rbOpenCircuitVoltagePowerSupply->text()), "blue");
    iStepOpenCircuitVoltagePowerSupply = 1;
    ui->rbClosedCircuitVoltagePowerSupply->setEnabled(true);
    ui->groupBoxCOMPort->setEnabled(true);
    ui->groupBoxDiagnosticDevice->setEnabled(true);
    ui->groupBoxDiagnosticMode->setEnabled(true);
}
