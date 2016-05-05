#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "report.h"

extern Settings settings;

extern QVector<Battery> battery;

/*!
 * \brief MainWindow::on_btnBuildReport_clicked
 */
void MainWindow::on_btnBuildReport_clicked()
{
    dateTime = QDateTime::currentDateTime();
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
               "<p align=\"center\">Дата производства: <b>%3%4</p>"\
               "<p align=\"center\">Дата: <b>%5</b> Время: <b>%6</b></p>")
            .arg(ui->comboBoxBatteryList->currentText())
            .arg((ui->cbIsUUTBB->isChecked()) ? "(вариант 2)" : "")
            .arg((ui->cbIsImitator->isChecked()) ? "<br/>(имитатор)" : "")
            .arg(ui->dateEditBatteryBuild->text())
            .arg((ui->lineEditBatteryNumber->text().length() > 0) ? tr("</b> Номер батареи: <b>%0</b>").arg(ui->lineEditBatteryNumber->text()) : "")
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
        sHtml += tr("<tr>"\
                "<td>"\
                    "<p><b>1. %0, В</b><br/>"\
                    "Предельные значения: Не более %1 В.</p>"\
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
                                "<td>"\
                                    "<p><b>Режим</b></p>"\
                                "</td>"\
                            "</tr>").arg(ui->rbVoltageOnTheHousing->text()).arg(settings.voltage_corpus_limit);
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
        sHtml += tr("<tr>"\
                "<td>"\
                    "<p><b>2. %0, МОм</b><br/>"\
                    "Предельные значения: Не менее %1 МОм.</p>"\
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
                                "<td>"\
                                    "<p><b>Режим</b></p>"\
                                "</td>"\
                            "</tr>").arg(ui->rbInsulationResistance->text()).arg(settings.isolation_resistance_limit/1000000);
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
        sHtml += tr("<tr>"\
                "<td>"\
                    "<p><b>3. %0, В</b><br/>"\
                    "Предельные значения: Не менее %1 и не более %2 В.</p>"\
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
                                "<td>"\
                                    "<p><b>Режим</b></p>"\
                                "</td>"\
                            "</tr>").arg(ui->rbOpenCircuitVoltageGroup->text()).arg(settings.opencircuitgroup_limit_min).arg(settings.opencircuitgroup_limit_max);
        QString value;
        foreach (value, sArrayReportOpenCircuitVoltageGroup) {
            sHtml += value;
        }
        sHtml += "       </tbody>"\
                    "</table>"\
                "</td>"\
            "</tr>";
    }
    /// 4. Напряжение разомкнутой цепи батареи
    if (!sArrayReportOpenCircuitVoltageBattery.isEmpty()) {
        sHtml += tr("<tr>"\
                "<td>"\
                    "<p><b>4. %0, В</b><br>"\
                    "Предельные значения: Не менее %1 В.</p>"\
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
                                "<td>"\
                                    "<p><b>Режим</b></p>"\
                                "</td>"\
                            "</tr>").arg(ui->rbOpenCircuitVoltageBattery->text()).arg(settings.opencircuitbattery_limit);
        QString value;
        foreach (value, sArrayReportOpenCircuitVoltageBattery) {
            sHtml += value;
        }
        sHtml += "       </tbody>"\
                    "</table>"\
                "</td>"\
            "</tr>";
    }
    /// 5. Напряжение замкнутой цепи группы
    if (!sArrayReportClosedCircuitVoltageGroup.isEmpty()) {
        sHtml += tr("<tr>"\
                "<td>"\
                    "<p><b>5. %0, В</b><br/>"\
                    "Предельные значения: Не менее %1 В.</p>"\
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
                                "<td>"\
                                    "<p><b>Режим</b></p>"\
                                "</td>"\
                            "</tr>").arg(ui->rbClosedCircuitVoltageGroup->text()).arg(settings.closecircuitgroup_limit);
        QString value;
        foreach (value, sArrayReportClosedCircuitVoltageGroup) {
            sHtml += value;
        }
        sHtml += "       </tbody>"\
                    "</table>"\
                "</td>"\
            "</tr>";
    }
    /// 6. Распассивация
    if (!sArrayReportDepassivation.isEmpty()) {
        sHtml += "<tr>"\
                "<td>"\
                    "<p><b>6. "+ui->rbDepassivation->text()+", В</b></p>"\
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
    /// 7. Напряжение замкнутой цепи батареи
    if (!sArrayReportClosedCircuitVoltageBattery.isEmpty()) {
        sHtml += tr("<tr>"\
                "<td>"\
                    "<p><b>7. %0, В</b><br/>"\
                    "Предельные значения: Не менее %1 В.</p>"\
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
                                "<td>"\
                                    "<p><b>Режим</b></p>"\
                                "</td>"\
                            "</tr>").arg(ui->rbClosedCircuitVoltageBattery->text()).arg(settings.closecircuitbattery_limit);
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

        /// 8. Сопротивление изоляции УУТББ
        if (!sArrayReportInsulationResistanceUUTBB.isEmpty()) {
            sHtml += tr("<tr>"\
                    "<td>"\
                        "<p><b>8. %0, МОм</b><br/>"\
                        "Предельные значения: Не менее %1 МОм.</p>"\
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
                                    "<td>"\
                                        "<p><b>Режим</b></p>"\
                                    "</td>"\
                                "</tr>").arg(ui->rbInsulationResistanceUUTBB->text()).arg(settings.uutbb_isolation_resist_limit/1000000);
            QString value;
            foreach (value, sArrayReportInsulationResistanceUUTBB) {
                sHtml += value;
            }
            sHtml += "       </tbody>"\
                        "</table>"\
                    "</td>"\
                "</tr>";
        }
        /// 9. Напряжение разомкнутой цепи БП
        if (!sArrayReportOpenCircuitVoltagePowerSupply.isEmpty()) {
            sHtml += tr("<tr>"\
                    "<td>"\
                        "<p><b>9. %0, В</b><br/>"\
                        "Предельные значения: Не менее %1 и не более %2 В.</p>"\
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
                                    "<td>"\
                                        "<p><b>Режим</b></p>"\
                                    "</td>"\
                                "</tr>").arg(ui->rbOpenCircuitVoltagePowerSupply->text()).arg(settings.uutbb_opencircuitpower_limit_min).arg(settings.uutbb_opencircuitpower_limit_max);
            QString value;
            foreach (value, sArrayReportOpenCircuitVoltagePowerSupply) {
                sHtml += value;
            }
            sHtml += "       </tbody>"\
                        "</table>"\
                    "</td>"\
                "</tr>";
        }
        /// 10. Напряжение замкнутой цепи БП
        if (!sArrayReportClosedCircuitVoltagePowerSupply.isEmpty()) {
            sHtml += tr("<tr>"\
                    "<td>"\
                        "<p><b>10. %0, В</b><br/>"\
                        "Предельные значения: Не менее %1 В.</p>"\
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
                                    "<td>"\
                                        "<p><b>Режим</b></p>"\
                                    "</td>"\
                                "</tr>").arg(ui->rbClosedCircuitVoltagePowerSupply->text()).arg(settings.uutbb_closecircuitpower_limit);
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

    /// проходимя по массиву с гарфиками и выводим их
    for (int i = 0; i < imgArrayReportGraph.count(); i++) {
        imgArrayReportGraph[i].save(QDir::tempPath()+tr("img%0.png").arg(i));
        sHtml += tr("<p><img src=\"%0\"/><br/>%1</p>").arg(QDir::tempPath()+tr("img%0.png").arg(i)).arg(sArrayReportGraphDescription[i]);
    }

    QTextDocument doc;
    doc.setHtml(sHtml);
    doc.setPageSize(printer.pageRect().size());
    doc.print(&printer);

    ui->btnBuildReport->setEnabled(true);
    Log(tr("PDF-файл <b>\"%0.pdf\"</b> отчета проверки успешно создан.").arg(QFileInfo(fileName).baseName()), "green");
    QMessageBox::information(this, "Отчет", "PDF-файл отчета успешно сформирован."); // выводим сообщение о завершении формирования отчета
}
