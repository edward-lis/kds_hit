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
    sHtml += tr("<table border=\"0\" width=\"100%\" cellpadding=\"3\" cellspacing=\"0\">"\
                "    <tbody>"\
                "        <tr>"\
                "            <td>"\
                "                <p><b>Проверяемый параметр:</b></p>"\
                "            </td>"\
                "        </tr>"\
                "        <tr>"\
                "            <td>");

    /// проходимя по массиву проверок, формируем html-документ
    for (int i = 0; i < sArrayReport.count(); i++) {
        sHtml += tr("%0").arg(sArrayReport[i]);
    }

    /// общая таблица - низ
    sHtml += tr("            </td>"\
                "        </tr>"\
                "    </tbody>"\
                "</table>"\
                "<br/><br/>"\
                "<p align=\"left\">&nbsp;&nbsp;Ф.И.О _______________________ подпись _____________ "\
                "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"\
                "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"\
                "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"\
                "Дата: %0</p><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/>").arg(textDate);

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
