#include <QDebug>
#include <QMessageBox>
#include <QDateTime>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "battery.h"

extern QVector<Battery> battery;

/*
 * Распассивация
 */
void MainWindow::checkDepassivation()
{
    if (((QPushButton*)sender())->objectName() == "btnDepassivation") {
        iStepDepassivation = 1;
        bState = false;
        //ui->btnDepassivation_2->setEnabled(false);
    }
    if (((QPushButton*)sender())->objectName() == "btnDepassivation_2")
        bState = false;
    if (!bState) return;
    ui->groupBoxCOMPort->setEnabled(false);
    ui->groupBoxDiagnosticDevice->setEnabled(false);
    ui->groupBoxDiagnosticMode->setEnabled(false);
    //ui->tabWidget->addTab(ui->tabDepassivation, ui->rbDepassivation->text());
    Log(tr("Проверка начата - %1").arg(ui->rbDepassivation->text()), "blue");
    switch (iBatteryIndex) {
    case 0: //9ER20P-20
        ui->progressBar->setValue(iStepDepassivation-1);
        ui->progressBar->setMaximum(imDepassivation.count());
        while (iStepDepassivation <= imDepassivation.count()) {
            if (!bState) return;
            delay(1000);
            Log(tr("%1) между контактом 1 соединителя Х3 «Х3-» и контактом %1 соединителя Х4 «4»").arg(imDepassivation.at(iStepDepassivation-1)), "green");

            iStepDepassivation++;
        }
        break;
    default:
        break;
    }
    Log(tr("Проверка завершена - %1").arg(ui->rbDepassivation->text()), "blue");
    iStepDepassivation = 1;
    //ui->rbDepassivation->setEnabled(false);
    //ui->btnDepassivation_2->setEnabled(false);
    ui->groupBoxCOMPort->setEnabled(true);
    ui->groupBoxDiagnosticDevice->setEnabled(true);
    ui->groupBoxDiagnosticMode->setEnabled(true);
}
