#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "report.h"

/*!
 * \brief MainWindow::on_btnBuildReport_clicked
 */
void MainWindow::on_btnBuildReport_clicked()
{
    qDebug() << "on_btnBuildReport_clicked()";
    QDateTime dateTime = QDateTime::currentDateTime();
    QString textDateTime = dateTime.toString("yyyy-MM-dd-hh-mm-ss-zzz");
    QString fileName = QFileDialog::getSaveFileName(this, tr("Сохранение отчет проверки в PDF-файл"), tr("%0_%1.pdf").arg(ui->comboBoxBatteryList->currentText()).arg(textDateTime), "*.pdf");
    if (QFileInfo(fileName).suffix().isEmpty())
        fileName.append(".pdf");
    if (fileName.length() < 5) /// без имени файла не сохраняем
        return;

    QPrinter printer(QPrinter::PrinterResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setPaperSize(QPrinter::A4);
    printer.setOutputFileName(fileName);

    sHtml = "<h1>Отчет проверки батареи: \""+ui->comboBoxBatteryList->currentText()+"\"</h1>\n";
    for (int i = 0; i < ui->cbParamsAutoMode->count(); i++)
    {
        sHtml += "<p>"+ui->cbParamsAutoMode->itemText(i)+"</p>\n";
    }

    QTextDocument doc;
    doc.setHtml(sHtml);
    doc.setPageSize(printer.pageRect().size());
    doc.print(&printer);

    Log(tr("PDF-файл <b>\"%0.pdf\"</b> отчета проверки успешно создан.").arg(QFileInfo(fileName).baseName()), "green");
}
