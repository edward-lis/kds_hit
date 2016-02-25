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
    QString fileName = QFileDialog::getSaveFileName(this, tr("Сохранение отчет проверки в PDF-файл"), tr("%0_%1.pdf").arg(ui->comboBoxBatteryList->currentText()).arg(textDateTime), "*.pdf");
    if (fileName.length() < 5) /// без имени файла не сохраняем
        return;
    QPrinter printer(QPrinter::PrinterResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setPaperSize(QPrinter::A4);
    printer.setOutputFileName(fileName);

    /// шапка отчета
    sHtml = "<h1 style=\"text-align: center;\">Отчет проверки батареи: <b>"+ui->comboBoxBatteryList->currentText()+"</b></h1>"\
    "<p align=\"center\">Дата производства: <b>"+ui->dateEditBatteryBuild->text()+"</b> Номер батареи: <b>"+ui->lineEditBatteryNumber->text()+"</b></p>";


    /// Напряжение на корпусе
    sHtml += "<table border=\"0\" cellpadding=\"0\" cellspacing=\"0\" style=\"width: 500px\">"\
             "<caption style=\"text-align: left;\"><b>"+ui->rbVoltageOnTheHousing->text()+", В.</b></caption><tbody>";
    for (int i = 0; i < dArrayVoltageOnTheHousing.size(); i++)
    {
        str = tr("Напряжение цепи \"%0\" = <b>%1</b> В.").arg(battery[iBatteryIndex].str_voltage_corpus[i]).arg(dArrayVoltageOnTheHousing[i]);
        if (dArrayVoltageOnTheHousing[i] > settings.voltage_corpus_limit) {
            str += " Не норма.";
            color = "#FF0000";
        } else
            color = "#008000";
        sHtml += "<tr><td><font color=\""+color+"\">"+str+"</font></td></tr>";
    }
    sHtml += "</tbody></table>";


    /// Сопротивление изоляции
    sHtml += "<p>&nbsp;</p><table border=\"0\" cellpadding=\"0\" cellspacing=\"0\" style=\"width: 500px\">"\
             "<caption style=\"text-align: left;\"><b>"+ui->rbInsulationResistance->text()+", МОм.</b></caption><tbody>";
    for (int i = 0; i < battery[iBatteryIndex].i_isolation_resistance_num; i++)
    {
        str = tr("Сопротивление цепи \"%0\" = <b>%1</b> МОм.").arg(battery[iBatteryIndex].str_isolation_resistance[i]).arg(dArrayInsulationResistance[i]);
        if (dArrayInsulationResistance[i] > settings.isolation_resistance_limit) {
            str += " Не норма.";
            color = "#FF0000";
        } else
            color = "#008000";
        sHtml += "<tr><td><font color=\""+color+"\">"+str+"</font></td></tr>";
    }
    sHtml += "</tbody></table>";


    /// Напряжение разомкнутой цепи группы
    sHtml += "<p>&nbsp;</p><table border=\"0\" cellpadding=\"0\" cellspacing=\"0\" style=\"width: 500px\">"\
             "<caption style=\"text-align: left;\"><b>"+ui->rbOpenCircuitVoltageGroup->text()+", В.</b></caption><tbody>";
    for (int i = 0; i < battery[iBatteryIndex].group_num; i++)
    {
        str = tr("Напряжение цепи \"%0\" = <b>%1</b> В.").arg(battery[iBatteryIndex].circuitgroup[i]).arg(dArrayOpenCircuitVoltageGroup[i]);
        if (dArrayOpenCircuitVoltageGroup[i] > settings.closecircuitgroup_limit) {
            str += " Не норма.";
            color = "#FF0000";
        } else
            color = "#008000";
        sHtml += "<tr><td><font color=\""+color+"\">"+str+"</font></td></tr>";
    }
    sHtml += "</tbody></table>";

    QTextDocument doc;
    doc.setHtml(sHtml);
    doc.setPageSize(printer.pageRect().size());
    doc.print(&printer);

    Log(tr("PDF-файл <b>\"%0.pdf\"</b> отчета проверки успешно создан.").arg(QFileInfo(fileName).baseName()), "green");
}
