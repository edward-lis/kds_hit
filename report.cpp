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


    /// шапка отчета
    sHtml = tr("<h1 style=\"text-align: center;\">Отчет проверки батареи: %0%1%2</h1>"\
               "<p align=\"center\">Дата производства: <b>%3</b> Номер батареи: <b>%4</b></p>"\
               "<p align=\"center\">Дата: <b>%5</b> Время: <b>%6</b></p>")
            .arg(ui->comboBoxBatteryList->currentText())
            .arg((ui->cbIsUUTBB->isChecked()) ? "(вариант 2)" : "")
            .arg((ui->cbIsImitator->isChecked()) ? "<br/>(имитатор)" : "")
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

    /// 1. Напряжение на корпусе
    if (!sArrayReportVoltageOnTheHousing.isEmpty()) {
        sHtml += "<tr>"\
                "<td>"\
                    "<p><b>1. "+ui->rbVoltageOnTheHousing->text()+", В</b></p>"\
                    "<table border=\"1\" cellpadding=\"3\" cellspacing=\"0\" width=\"100%\" bordercolor=\"black\">"\
                        "<tbody>"\
                            "<tr>"\
                                "<td>"\
                                    "<p><b>Время</b></p>"\
                                "</td>"\
                                "<td>"\
                                    "<p><b>Цепь</b></p>"\
                                "</td>"\
                                "<td>"\
                                    "<p><b>Значение</b></p>"\
                                "</td>"\
                                "<td>"\
                                    "<p><b>Результат</b></p>"\
                                "</td>"\
                            "</tr>";
        QString value;
        foreach (value, sArrayReportVoltageOnTheHousing) {
            sHtml += value;
        }

        sHtml += "       </tbody>"\
                    "</table>"\
                "</td>"\
            "</tr>";
    }
    /// 2. Сопротивление изоляции
    if (!sArrayReportInsulationResistance.isEmpty()) {
        sHtml += "<tr>"\
                "<td>"\
                    "<p><b>2. "+ui->rbInsulationResistance->text()+", МОм</b></p>"\
                    "<table border=\"1\" cellpadding=\"3\" cellspacing=\"0\" width=\"100%\" bordercolor=\"black\">"\
                        "<tbody>"\
                            "<tr>"\
                                "<td>"\
                                    "<p><b>Время</b></p>"\
                                "</td>"\
                                "<td>"\
                                    "<p><b>Цепь</b></p>"\
                                "</td>"\
                                "<td>"\
                                    "<p><b>Значение</b></p>"\
                                "</td>"\
                                "<td>"\
                                    "<p><b>Результат</b></p>"\
                                "</td>"\
                            "</tr>";
        QString value;
        foreach (value, sArrayReportInsulationResistance) {
            sHtml += value;
        }
        sHtml += "       </tbody>"\
                    "</table>"\
                "</td>"\
            "</tr>";
    }
    /// 3. Напряжение разомкнутой цепи группы
    if (!sArrayReportOpenCircuitVoltageGroup.isEmpty()) {
        sHtml += "<tr>"\
                "<td>"\
                    "<p><b>3. "+ui->rbOpenCircuitVoltageGroup->text()+", В</b></p>"\
                    "<table border=\"1\" cellpadding=\"3\" cellspacing=\"0\" width=\"100%\" bordercolor=\"black\">"\
                        "<tbody>"\
                            "<tr>"\
                                "<td>"\
                                    "<p><b>Время</b></p>"\
                                "</td>"\
                                "<td>"\
                                    "<p><b>Цепь</b></p>"\
                                "</td>"\
                                "<td>"\
                                    "<p><b>Значение</b></p>"\
                                "</td>"\
                                "<td>"\
                                    "<p><b>Результат</b></p>"\
                                "</td>"\
                            "</tr>";
        QString value;
        foreach (value, sArrayReportOpenCircuitVoltageGroup) {
            sHtml += value;
        }
        sHtml += "       </tbody>"\
                    "</table>"\
                "</td>"\
            "</tr>";
    }
    /// 3а. Напряжение разомкнутой цепи батареи
    if (!sArrayReportOpenCircuitVoltageBattery.isEmpty()) {
        sHtml += "<tr>"\
                "<td>"\
                    "<p><b>3а. "+ui->rbOpenCircuitVoltageBattery->text()+", В</b></p>"\
                    "<table border=\"1\" cellpadding=\"3\" cellspacing=\"0\" width=\"100%\" bordercolor=\"black\">"\
                        "<tbody>"\
                            "<tr>"\
                                "<td>"\
                                    "<p><b>Время</b></p>"\
                                "</td>"\
                                "<td>"\
                                    "<p><b>Цепь</b></p>"\
                                "</td>"\
                                "<td>"\
                                    "<p><b>Значение</b></p>"\
                                "</td>"\
                                "<td>"\
                                    "<p><b>Результат</b></p>"\
                                "</td>"\
                            "</tr>";
        QString value;
        foreach (value, sArrayReportOpenCircuitVoltageBattery) {
            sHtml += value;
        }
        sHtml += "       </tbody>"\
                    "</table>"\
                "</td>"\
            "</tr>";
    }
    /// 4. Напряжение замкнутой цепи группы
    if (!sArrayReportClosedCircuitVoltageGroup.isEmpty()) {
        sHtml += "<tr>"\
                "<td>"\
                    "<p><b>4. "+ui->rbClosedCircuitVoltageGroup->text()+", В</b></p>"\
                    "<table border=\"1\" cellpadding=\"3\" cellspacing=\"0\" width=\"100%\" bordercolor=\"black\">"\
                        "<tbody>"\
                            "<tr>"\
                                "<td>"\
                                    "<p><b>Время</b></p>"\
                                "</td>"\
                                "<td>"\
                                    "<p><b>Цепь</b></p>"\
                                "</td>"\
                                "<td>"\
                                    "<p><b>Значение</b></p>"\
                                "</td>"\
                                "<td>"\
                                    "<p><b>Результат</b></p>"\
                                "</td>"\
                            "</tr>";
        QString value;
        foreach (value, sArrayReportClosedCircuitVoltageGroup) {
            sHtml += value;
        }
        sHtml += "       </tbody>"\
                    "</table>"\
                "</td>"\
            "</tr>";
    }
    /// 4а. Распассивация
    if (!sArrayReportDepassivation.isEmpty()) {
        sHtml += "<tr>"\
                "<td>"\
                    "<p><b>4а. "+ui->rbDepassivation->text()+", В</b></p>"\
                    "<table border=\"1\" cellpadding=\"3\" cellspacing=\"0\" width=\"100%\" bordercolor=\"black\">"\
                        "<tbody>"\
                            "<tr>"\
                                "<td>"\
                                    "<p><b>Время</b></p>"\
                                "</td>"\
                                "<td>"\
                                    "<p><b>Цепь</b></p>"\
                                "</td>"\
                                "<td>"\
                                    "<p><b>Значение</b></p>"\
                                "</td>"\
                                "<td>"\
                                    "<p><b>Результат</b></p>"\
                                "</td>"\
                            "</tr>";
        QString value;
        foreach (value, sArrayReportDepassivation) {
            sHtml += value;
        }
        sHtml += "       </tbody>"\
                    "</table>"\
                "</td>"\
            "</tr>";
    }
    /// 5. Напряжение замкнутой цепи батареи
    if (!sArrayReportClosedCircuitVoltageBattery.isEmpty()) {
        sHtml += "<tr>"\
                "<td>"\
                    "<p><b>5. "+ui->rbClosedCircuitVoltageBattery->text()+", В</b></p>"\
                    "<table border=\"1\" cellpadding=\"3\" cellspacing=\"0\" width=\"100%\" bordercolor=\"black\">"\
                        "<tbody>"\
                            "<tr>"\
                                "<td>"\
                                    "<p><b>Время</b></p>"\
                                "</td>"\
                                "<td>"\
                                    "<p><b>Цепь</b></p>"\
                                "</td>"\
                                "<td>"\
                                    "<p><b>Значение</b></p>"\
                                "</td>"\
                                "<td>"\
                                    "<p><b>Результат</b></p>"\
                                "</td>"\
                            "</tr>";
        QString value;
        foreach (value, sArrayReportClosedCircuitVoltageBattery) {
            sHtml += value;
        }
        sHtml += "       </tbody>"\
                    "</table>"\
                "</td>"\
            "</tr>";
    }
    /// только для батарей 9ER20P_20 или 9ER14PS_24
    if (ui->cbIsUUTBB->isChecked()) {

        /// 6. Сопротивление изоляции УУТББ
        if (!sArrayReportInsulationResistanceUUTBB.isEmpty()) {
            sHtml += "<tr>"\
                    "<td>"\
                        "<p><b>6. "+ui->rbInsulationResistanceUUTBB->text()+", МОм</b></p>"\
                        "<table border=\"1\" cellpadding=\"3\" cellspacing=\"0\" width=\"100%\" bordercolor=\"black\">"\
                            "<tbody>"\
                                "<tr>"\
                                    "<td>"\
                                        "<p><b>Время</b></p>"\
                                    "</td>"\
                                    "<td>"\
                                        "<p><b>Цепь</b></p>"\
                                    "</td>"\
                                    "<td>"\
                                        "<p><b>Значение</b></p>"\
                                    "</td>"\
                                    "<td>"\
                                        "<p><b>Результат</b></p>"\
                                    "</td>"\
                                "</tr>";
            QString value;
            foreach (value, sArrayReportInsulationResistanceUUTBB) {
                sHtml += value;
            }
            sHtml += "       </tbody>"\
                        "</table>"\
                    "</td>"\
                "</tr>";
        }
        /// 7. Напряжение разомкнутой цепи БП
        if (!sArrayReportOpenCircuitVoltagePowerSupply.isEmpty()) {
            sHtml += "<tr>"\
                    "<td>"\
                        "<p><b>7. "+ui->rbOpenCircuitVoltagePowerSupply->text()+", В</b></p>"\
                        "<table border=\"1\" cellpadding=\"3\" cellspacing=\"0\" width=\"100%\" bordercolor=\"black\">"\
                            "<tbody>"\
                                "<tr>"\
                                    "<td>"\
                                        "<p><b>Время</b></p>"\
                                    "</td>"\
                                    "<td>"\
                                        "<p><b>Цепь</b></p>"\
                                    "</td>"\
                                    "<td>"\
                                        "<p><b>Значение</b></p>"\
                                    "</td>"\
                                    "<td>"\
                                        "<p><b>Результат</b></p>"\
                                    "</td>"\
                                "</tr>";
            QString value;
            foreach (value, sArrayReportOpenCircuitVoltagePowerSupply) {
                sHtml += value;
            }
            sHtml += "       </tbody>"\
                        "</table>"\
                    "</td>"\
                "</tr>";
        }
        /// 8. Напряжение замкнутой цепи БП
        if (!sArrayReportClosedCircuitVoltagePowerSupply.isEmpty()) {
            sHtml += "<tr>"\
                    "<td>"\
                        "<p><b>8. "+ui->rbClosedCircuitVoltagePowerSupply->text()+", В</b></p>"\
                        "<table border=\"1\" cellpadding=\"3\" cellspacing=\"0\" width=\"100%\" bordercolor=\"black\">"\
                            "<tbody>"\
                                "<tr>"\
                                    "<td>"\
                                        "<p><b>Время</b></p>"\
                                    "</td>"\
                                    "<td>"\
                                        "<p><b>Цепь</b></p>"\
                                    "</td>"\
                                    "<td>"\
                                        "<p><b>Значение</b></p>"\
                                    "</td>"\
                                    "<td>"\
                                        "<p><b>Результат</b></p>"\
                                    "</td>"\
                                "</tr>";
            QString value;
            foreach (value, sArrayReportClosedCircuitVoltagePowerSupply) {
                sHtml += value;
            }
            sHtml += "       </tbody>"\
                        "</table>"\
                    "</td>"\
                "</tr>";
        }
    }

    /// общая таблица - низ
    sHtml += tr("	</tbody>"\
            "</table>"\
            "<br/><br/>"\
            "<p align=\"left\">&nbsp;&nbsp;Ф.И.О _______________________ подпись _____________ "\
            "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"\
            "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"\
            "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"\
            "Дата: %0</p>").arg(textDate);

    QTextDocument doc;
    doc.setHtml(sHtml);
    doc.setPageSize(printer.pageRect().size());
    doc.print(&printer);

    ui->btnBuildReport->setEnabled(true);
    Log(tr("PDF-файл <b>\"%0.pdf\"</b> отчета проверки успешно создан.").arg(QFileInfo(fileName).baseName()), "green");
    QMessageBox::information(this, "Отчет", "PDF-файл отчета успешно сформирован."); // выводим сообщение о завершении формирования отчета
}
