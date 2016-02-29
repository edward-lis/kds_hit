#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "report.h"

extern QVector<Battery> battery;

/*!
 * \brief MainWindow::on_btnBuildReport_clicked
 */
void MainWindow::on_btnBuildReport_clicked()
{
    qDebug() << "on_btnBuildReport_clicked()";
    QDateTime dateTime = QDateTime::currentDateTime();
    QString textDateTime = dateTime.toString("yyyy-MM-dd-hh-mm-ss-zzz");
    QString textDate = dateTime.toString("dd.MM.yyyy");
    QString textTime = dateTime.toString("hh:mm:ss");
    QString fileName = QFileDialog::getSaveFileName(this, tr("Сохранение отчет проверки в PDF-файл"), tr("%0_%1.pdf").arg(ui->comboBoxBatteryList->currentText()).arg(textDateTime), "*.pdf");
    if (QFileInfo(fileName).baseName().length() <= 0) /// без имени файла не сохраняем
        return;
    if(QFileInfo(fileName).suffix().isEmpty())
        fileName.append(".pdf");
    ui->btnBuildReport->setDisabled(true);
    QPrinter printer(QPrinter::PrinterResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setPaperSize(QPrinter::A4);
    printer.setOutputFileName(fileName);

    ui->progressBar->setMaximum((ui->cbIsUUTBB->isChecked()) ? 11 : 8);
    ui->progressBar->setValue(0);

    /// шапка отчета
    sHtml = tr("<h1 style=\"text-align: center;\">Отчет проверки батареи: <b>%0</b></h1>"\
    "<p align=\"center\">Дата производства: <b>%1</b> Номер батареи: <b>%2</b></p>"\
    "<p align=\"center\">Дата: <b>%3</b> Время: <b>%4</b></p>")
            .arg((ui->cbIsImitator->isChecked()) ? ui->comboBoxBatteryList->currentText()+"(имитатор)" : ui->comboBoxBatteryList->currentText())
            .arg(ui->dateEditBatteryBuild->text())
            .arg(ui->lineEditBatteryNumber->text())
            .arg(textDate)
            .arg(textTime);

    /// общая таблица - верх
    sHtml += "<table border=\"0\" cellpadding=\"3\" cellspacing=\"0\" width=\"100%\">"\
            "<tbody>"\
                "<tr>"\
                    "<td>"\
                        "<p><b>Проверяемый параметр:</b></p>"\
                    "</td>"\
                "</tr>";
    ui->progressBar->setValue(ui->progressBar->value()+1);

    /// 1. Напряжение на корпусе
    sHtml += "<tr>"\
            "<td>"\
                "<p><b>1. "+ui->rbVoltageOnTheHousing->text()+", В</b></p>"\
                "<table border=\"1\" cellpadding=\"3\" cellspacing=\"0\" width=\"100%\" bordercolor=\"black\">"\
                    "<tbody>"\
                        "<tr>"\
                            "<td>"\
                                "<p><b>№</b></p>"\
                            "</td>"\
                            "<td>"\
                                "<p><b>Напряжение цепи</b></p>"\
                            "</td>"\
                            "<td>"\
                                "<p><b>Значение</b></p>"\
                            "</td>"\
                            "<td>"\
                                "<p><b>Результат</b></p>"\
                            "</td>"\
                        "</tr>";
    for (int i = 0; i < 2; i++) {
        sHtml += tr("    <tr>"\
                            "<td>%0.</td>"\
                            "<td>%1</td>"\
                            "<td>%2</td>"\
                            "<td>%3</td>"\
                        "</tr>").arg(i+1)
                                .arg(battery[iBatteryIndex].str_voltage_corpus[i])
                                .arg(dArrayVoltageOnTheHousing[i])
                                .arg((dArrayVoltageOnTheHousing[i] > settings.voltage_corpus_limit) ? "Не норма!": "Норма");
    }
    sHtml += "       </tbody>"\
                "</table>"\
            "</td>"\
        "</tr>";
    ui->progressBar->setValue(ui->progressBar->value()+1);

    /// 2. Сопротивление изоляции
    sHtml += "<tr>"\
            "<td>"\
                "<p><b>2. "+ui->rbInsulationResistance->text()+", МОм</b></p>"\
                "<table border=\"1\" cellpadding=\"3\" cellspacing=\"0\" width=\"100%\" bordercolor=\"black\">"\
                    "<tbody>"\
                        "<tr>"\
                            "<td>"\
                                "<p><b>№</b></p>"\
                            "</td>"\
                            "<td>"\
                                "<p><b>Сопротивление цепи</b></p>"\
                            "</td>"\
                            "<td>"\
                                "<p><b>Значение</b></p>"\
                            "</td>"\
                            "<td>"\
                                "<p><b>Результат</b></p>"\
                            "</td>"\
                        "</tr>";
    for (int i = 0; i < battery[iBatteryIndex].i_isolation_resistance_num; i++) {
        sHtml += tr("    <tr>"\
                            "<td>%0.</td>"\
                            "<td>%1</td>"\
                            "<td>%2</td>"\
                            "<td>%3</td>"\
                        "</tr>").arg(i+1)
                                .arg(battery[iBatteryIndex].str_isolation_resistance[i])
                                .arg(dArrayInsulationResistance[i])
                                .arg((dArrayInsulationResistance[i] > settings.isolation_resistance_limit) ? "Не норма!": "Норма");
    }
    sHtml += "       </tbody>"\
                "</table>"\
            "</td>"\
        "</tr>";
    ui->progressBar->setValue(ui->progressBar->value()+1);

    /// 3. Напряжение разомкнутой цепи группы
    sHtml += "<tr>"\
            "<td>"\
                "<p><b>3. "+ui->rbOpenCircuitVoltageGroup->text()+", В</b></p>"\
                "<table border=\"1\" cellpadding=\"3\" cellspacing=\"0\" width=\"100%\" bordercolor=\"black\">"\
                    "<tbody>"\
                        "<tr>"\
                            "<td>"\
                                "<p><b>№</b></p>"\
                            "</td>"\
                            "<td>"\
                                "<p><b>Напряжение цепи</b></p>"\
                            "</td>"\
                            "<td>"\
                                "<p><b>Значение</b></p>"\
                            "</td>"\
                            "<td>"\
                                "<p><b>Результат</b></p>"\
                            "</td>"\
                        "</tr>";
    for (int i = 0; i < battery[iBatteryIndex].group_num; i++) {
        sHtml += tr("    <tr>"\
                            "<td>%0.</td>"\
                            "<td>%1</td>"\
                            "<td>%2</td>"\
                            "<td>%3</td>"\
                        "</tr>").arg(i+1)
                                .arg(battery[iBatteryIndex].circuitgroup[i])
                                .arg(dArrayOpenCircuitVoltageGroup[i])
                                .arg((dArrayOpenCircuitVoltageGroup[i] > settings.closecircuitgroup_limit) ? "Не норма!": "Норма");
    }
    sHtml += "       </tbody>"\
                "</table>"\
            "</td>"\
        "</tr>";
    ui->progressBar->setValue(ui->progressBar->value()+1);

    /// 3а. Напряжение разомкнутой цепи батареи
    sHtml += "<tr>"\
            "<td>"\
                "<p><b>3а. "+ui->rbOpenCircuitVoltageBattery->text()+", В</b></p>"\
                "<table border=\"1\" cellpadding=\"3\" cellspacing=\"0\" width=\"100%\" bordercolor=\"black\">"\
                    "<tbody>"\
                        "<tr>"\
                            "<td>"\
                                "<p><b>№</b></p>"\
                            "</td>"\
                            "<td>"\
                                "<p><b>Напряжение цепи</b></p>"\
                            "</td>"\
                            "<td>"\
                                "<p><b>Значение</b></p>"\
                            "</td>"\
                            "<td>"\
                                "<p><b>Результат</b></p>"\
                            "</td>"\
                        "</tr>";
    for (int i = 0; i < 1; i++) {
        sHtml += tr("    <tr>"\
                            "<td>%0.</td>"\
                            "<td>%1</td>"\
                            "<td>%2</td>"\
                            "<td>%3</td>"\
                        "</tr>").arg(i+1)
                                .arg(battery[iBatteryIndex].circuitbattery)
                                .arg(dArrayOpenCircuitVoltageBattery[i])
                                .arg((dArrayOpenCircuitVoltageBattery[i] > settings.closecircuitbattery_limit) ? "Не норма!": "Норма");
    }
    sHtml += "       </tbody>"\
                "</table>"\
            "</td>"\
        "</tr>";
    ui->progressBar->setValue(ui->progressBar->value()+1);

    /// 4. Напряжение замкнутой цепи группы
    sHtml += "<tr>"\
            "<td>"\
                "<p><b>4. "+ui->rbClosedCircuitVoltageGroup->text()+", В</b></p>"\
                "<table border=\"1\" cellpadding=\"3\" cellspacing=\"0\" width=\"100%\" bordercolor=\"black\">"\
                    "<tbody>"\
                        "<tr>"\
                            "<td>"\
                                "<p><b>№</b></p>"\
                            "</td>"\
                            "<td>"\
                                "<p><b>Напряжение цепи</b></p>"\
                            "</td>"\
                            "<td>"\
                                "<p><b>Значение</b></p>"\
                            "</td>"\
                            "<td>"\
                                "<p><b>Результат</b></p>"\
                            "</td>"\
                        "</tr>";
    for (int i = 0; i < battery[iBatteryIndex].group_num; i++) {
        sHtml += tr("    <tr>"\
                            "<td>%0.</td>"\
                            "<td>%1</td>"\
                            "<td>%2</td>"\
                            "<td>%3</td>"\
                        "</tr>").arg(i+1)
                                .arg(battery[iBatteryIndex].circuitgroup[i])
                                .arg(dArrayClosedCircuitVoltageGroup[i])
                                .arg((dArrayClosedCircuitVoltageGroup[i] > settings.closecircuitgroup_limit) ? "Не норма!": "Норма");
    }
    sHtml += "       </tbody>"\
                "</table>"\
            "</td>"\
        "</tr>";
    ui->progressBar->setValue(ui->progressBar->value()+1);

    /// 4а. Распассивация
    sHtml += "<tr>"\
            "<td>"\
                "<p><b>4а. "+ui->rbDepassivation->text()+", В</b></p>"\
                "<table border=\"1\" cellpadding=\"3\" cellspacing=\"0\" width=\"100%\" bordercolor=\"black\">"\
                    "<tbody>"\
                        "<tr>"\
                            "<td>"\
                                "<p><b>№</b></p>"\
                            "</td>"\
                            "<td>"\
                                "<p><b>Напряжение цепи</b></p>"\
                            "</td>"\
                            "<td>"\
                                "<p><b>Значение</b></p>"\
                            "</td>"\
                            "<td>"\
                                "<p><b>Результат</b></p>"\
                            "</td>"\
                        "</tr>";
    for (int i = 0; i < battery[iBatteryIndex].group_num; i++) {
        sHtml += tr("    <tr>"\
                            "<td>%0.</td>"\
                            "<td>%1</td>"\
                            "<td>%2</td>"\
                            "<td>%3</td>"\
                        "</tr>").arg(i+1)
                                .arg(battery[iBatteryIndex].circuitgroup[i])
                                .arg(dArrayDepassivation[i])
                                .arg((dArrayDepassivation[i] > settings.closecircuitgroup_limit) ? "Не норма!": "Норма");
    }
    sHtml += "       </tbody>"\
                "</table>"\
            "</td>"\
        "</tr>";
    ui->progressBar->setValue(ui->progressBar->value()+1);

    /// 5. Напряжение замкнутой цепи батареи
    sHtml += "<tr>"\
            "<td>"\
                "<p><b>5. "+ui->rbClosedCircuitVoltageBattery->text()+", В</b></p>"\
                "<table border=\"1\" cellpadding=\"3\" cellspacing=\"0\" width=\"100%\" bordercolor=\"black\">"\
                    "<tbody>"\
                        "<tr>"\
                            "<td>"\
                                "<p><b>№</b></p>"\
                            "</td>"\
                            "<td>"\
                                "<p><b>Напряжение цепи</b></p>"\
                            "</td>"\
                            "<td>"\
                                "<p><b>Значение</b></p>"\
                            "</td>"\
                            "<td>"\
                                "<p><b>Результат</b></p>"\
                            "</td>"\
                        "</tr>";
    for (int i = 0; i < 1; i++) {
        sHtml += tr("    <tr>"\
                            "<td>%0.</td>"\
                            "<td>%1</td>"\
                            "<td>%2</td>"\
                            "<td>%3</td>"\
                        "</tr>").arg(i+1)
                                .arg(battery[iBatteryIndex].circuitbattery)
                                .arg(dArrayClosedCircuitVoltageBattery[i])
                                .arg((dArrayClosedCircuitVoltageBattery[i] > settings.opencircuitbattery_limit) ? "Не норма!": "Норма");
    }
    sHtml += "       </tbody>"\
                "</table>"\
            "</td>"\
        "</tr>";

    /// только для батарей 9ER20P_20 или 9ER14PS_24
    if (ui->cbIsUUTBB->isChecked()) {
        ui->progressBar->setValue(ui->progressBar->value()+1);

        /// 6. Сопротивление изоляции УУТББ
        sHtml += "<tr>"\
                "<td>"\
                    "<p><b>6. "+ui->rbInsulationResistanceUUTBB->text()+", МОм</b></p>"\
                    "<table border=\"1\" cellpadding=\"3\" cellspacing=\"0\" width=\"100%\" bordercolor=\"black\">"\
                        "<tbody>"\
                            "<tr>"\
                                "<td>"\
                                    "<p><b>№</b></p>"\
                                "</td>"\
                                "<td>"\
                                    "<p><b>Сопротивление цепи</b></p>"\
                                "</td>"\
                                "<td>"\
                                    "<p><b>Значение</b></p>"\
                                "</td>"\
                                "<td>"\
                                    "<p><b>Результат</b></p>"\
                                "</td>"\
                            "</tr>";
        for (int i = 0; i < battery[iBatteryIndex].i_uutbb_resist_num; i++) {
            sHtml += tr("    <tr>"\
                                "<td>%0.</td>"\
                                "<td>%1</td>"\
                                "<td>%2</td>"\
                                "<td>%3</td>"\
                            "</tr>").arg(i+1)
                                    .arg(battery[iBatteryIndex].uutbb_resist[i])
                                    .arg(dArrayInsulationResistanceUUTBB[i])
                                    .arg((dArrayInsulationResistanceUUTBB[i] > settings.uutbb_isolation_resist_limit) ? "Не норма!": "Норма");
        }
        sHtml += "       </tbody>"\
                    "</table>"\
                "</td>"\
            "</tr>";
        ui->progressBar->setValue(ui->progressBar->value()+1);

        /// 7. Напряжение разомкнутой цепи БП
        sHtml += "<tr>"\
                "<td>"\
                    "<p><b>7. "+ui->rbOpenCircuitVoltagePowerSupply->text()+", В</b></p>"\
                    "<table border=\"1\" cellpadding=\"3\" cellspacing=\"0\" width=\"100%\" bordercolor=\"black\">"\
                        "<tbody>"\
                            "<tr>"\
                                "<td>"\
                                    "<p><b>№</b></p>"\
                                "</td>"\
                                "<td>"\
                                    "<p><b>Напряжение цепи</b></p>"\
                                "</td>"\
                                "<td>"\
                                    "<p><b>Значение</b></p>"\
                                "</td>"\
                                "<td>"\
                                    "<p><b>Результат</b></p>"\
                                "</td>"\
                            "</tr>";
        for (int i = 0; i < 1; i++) {
            sHtml += tr("    <tr>"\
                                "<td>%0.</td>"\
                                "<td>%1</td>"\
                                "<td>%2</td>"\
                                "<td>%3</td>"\
                            "</tr>").arg(i+1)
                                    .arg(battery[iBatteryIndex].uutbb_closecircuitpower[i])
                                    .arg(dArrayOpenCircuitVoltagePowerSupply[i])
                                    .arg((dArrayOpenCircuitVoltagePowerSupply[i] < settings.uutbb_opencircuitpower_limit_min or dArrayOpenCircuitVoltagePowerSupply[i] > settings.uutbb_opencircuitpower_limit_max) ? "Не норма!": "Норма");
        }
        sHtml += "       </tbody>"\
                    "</table>"\
                "</td>"\
            "</tr>";
        ui->progressBar->setValue(ui->progressBar->value()+1);

        /// 8. Напряжение замкнутой цепи БП
        sHtml += "<tr>"\
                "<td>"\
                    "<p><b>8. "+ui->rbClosedCircuitVoltagePowerSupply->text()+", В</b></p>"\
                    "<table border=\"1\" cellpadding=\"3\" cellspacing=\"0\" width=\"100%\" bordercolor=\"black\">"\
                        "<tbody>"\
                            "<tr>"\
                                "<td>"\
                                    "<p><b>№</b></p>"\
                                "</td>"\
                                "<td>"\
                                    "<p><b>Напряжение цепи</b></p>"\
                                "</td>"\
                                "<td>"\
                                    "<p><b>Значение</b></p>"\
                                "</td>"\
                                "<td>"\
                                    "<p><b>Результат</b></p>"\
                                "</td>"\
                            "</tr>";
        for (int i = 0; i < 2; i++) {
            sHtml += tr("    <tr>"\
                                "<td>%0.</td>"\
                                "<td>%1</td>"\
                                "<td>%2</td>"\
                                "<td>%3</td>"\
                            "</tr>").arg(i+1)
                                    .arg(battery[iBatteryIndex].uutbb_closecircuitpower[i])
                                    .arg(dArrayClosedCircuitVoltagePowerSupply[i])
                                    .arg((dArrayClosedCircuitVoltagePowerSupply[i] > settings.uutbb_closecircuitpower_limit) ? "Не норма!": "Норма");
        }
        sHtml += "       </tbody>"\
                    "</table>"\
                "</td>"\
            "</tr>";
    }

    /// общая таблица - низ
    sHtml += "	</tbody>"\
            "</table>"\
            "<p>&nbsp;</p>";

    QTextDocument doc;
    doc.setHtml(sHtml);
    doc.setPageSize(printer.pageRect().size());
    doc.print(&printer);

    ui->progressBar->setValue(ui->progressBar->value()+1);
    qDebug() << "ui->progressBar->value()=" << ui->progressBar->value();
    ui->btnBuildReport->setEnabled(true);
    Log(tr("PDF-файл <b>\"%0.pdf\"</b> отчета проверки успешно создан.").arg(QFileInfo(fileName).baseName()), "green");
}
