#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "datafile.h"
#include "settings.h"

extern Settings settings;

extern QVector<Battery> battery;

QDataStream &operator<<( QDataStream &stream, const dataBattery &data )
{
    return stream
            << data.iBatteryIndex
            << data.bIsUTTBB
            << data.bIsImitator
            << data.dateBuild
            << data.sNumber
            << data.bModeDiagnosticAuto
            << data.bModeDiagnosticManual
            << data.icbParamsAutoMode
            << data.icbSubParamsAutoMode
            << data.itemsVoltageOnTheHousing
            << data.itemsInsulationResistance
            << data.itemsOpenCircuitVoltageGroup
            << data.itemsClosedCircuitVoltageGroup
            << data.itemsDepassivation
            << data.itemsInsulationResistanceUUTBB
            << data.icbClosedCircuitVoltagePowerSupply
            << data.dArrayVoltageOnTheHousing
            << data.dArrayInsulationResistance
            << data.dArrayOpenCircuitVoltageGroup
            << data.dArrayOpenCircuitVoltageBattery
            << data.dArrayDepassivation
            << data.dArrayClosedCircuitVoltageGroup
            << data.dArrayClosedCircuitVoltageBattery
            << data.dArrayInsulationResistanceUUTBB
            << data.dArrayOpenCircuitVoltagePowerSupply
            << data.dArrayClosedCircuitVoltagePowerSupply
            << data.sArrayReport
            << data.sArrayReportGraphDescription
            << data.imgArrayReportGraph;
}

QDataStream &operator>>( QDataStream &stream, dataBattery &data )
{
    return stream
            >> data.iBatteryIndex
            >> data.bIsUTTBB
            >> data.bIsImitator
            >> data.dateBuild
            >> data.sNumber
            >> data.bModeDiagnosticAuto
            >> data.bModeDiagnosticManual
            >> data.icbParamsAutoMode
            >> data.icbSubParamsAutoMode
            >> data.itemsVoltageOnTheHousing
            >> data.itemsInsulationResistance
            >> data.itemsOpenCircuitVoltageGroup
            >> data.itemsClosedCircuitVoltageGroup
            >> data.itemsDepassivation
            >> data.itemsInsulationResistanceUUTBB
            >> data.icbClosedCircuitVoltagePowerSupply
            >> data.dArrayVoltageOnTheHousing
            >> data.dArrayInsulationResistance
            >> data.dArrayOpenCircuitVoltageGroup
            >> data.dArrayOpenCircuitVoltageBattery
            >> data.dArrayDepassivation
            >> data.dArrayClosedCircuitVoltageGroup
            >> data.dArrayClosedCircuitVoltageBattery
            >> data.dArrayInsulationResistanceUUTBB
            >> data.dArrayOpenCircuitVoltagePowerSupply
            >> data.dArrayClosedCircuitVoltagePowerSupply
            >> data.sArrayReport
            >> data.sArrayReportGraphDescription
            >> data.imgArrayReportGraph;
}

/*!
 * \brief MainWindow::on_actionCheckSave_triggered
 */
void MainWindow::on_actionCheckSave_triggered()
{
    qDebug() << "on_actionCheckSave_triggered()";
    QList<dataBattery> list;
    dataBattery data;
    data.iBatteryIndex = iBatteryIndex;
    data.bIsUTTBB = ui->cbIsUUTBB->checkState();
    data.bIsImitator = ui->cbIsImitator->checkState();
    data.dateBuild = ui->dateEditBatteryBuild->date();
    data.sNumber = ui->lineEditBatteryNumber->text();
    data.bModeDiagnosticAuto = ui->rbModeDiagnosticAuto->isChecked();
    data.bModeDiagnosticManual = ui->rbModeDiagnosticManual->isChecked();
    data.icbParamsAutoMode = ui->cbParamsAutoMode->currentIndex();
    data.icbSubParamsAutoMode = ui->cbSubParamsAutoMode->currentIndex();

    /// 1. Напряжение на корпусе
    for (int r = 0; r < ui->cbVoltageOnTheHousing->count(); r++)
    {
        index = ui->cbVoltageOnTheHousing->model()->index(r, 0);
        data.itemsVoltageOnTheHousing.append(index.data(Qt::CheckStateRole));
    }
    data.dArrayVoltageOnTheHousing = dArrayVoltageOnTheHousing;

    /// 2. Сопротивление изоляции
    for (int r = 0; r < ui->cbInsulationResistance->count(); r++)
    {
        index = ui->cbInsulationResistance->model()->index(r, 0);
        data.itemsInsulationResistance.append(index.data(Qt::CheckStateRole));
    }
    data.dArrayInsulationResistance = dArrayInsulationResistance;

    /// 3. Напряжение разомкнутой цепи группы
    for (int r = 0; r < ui->cbOpenCircuitVoltageGroup->count(); r++)
    {
        index = ui->cbOpenCircuitVoltageGroup->model()->index(r, 0);
        data.itemsOpenCircuitVoltageGroup.append(index.data(Qt::CheckStateRole));
    }
    data.dArrayOpenCircuitVoltageGroup = dArrayOpenCircuitVoltageGroup;

    /// 4 .Напряжение разомкнутой цепи батареи
    data.dArrayOpenCircuitVoltageBattery = dArrayOpenCircuitVoltageBattery;

    /// 5. Напряжение замкнутой цепи группы
    for (int r = 0; r < ui->cbClosedCircuitVoltageGroup->count(); r++)
    {
        index = ui->cbClosedCircuitVoltageGroup->model()->index(r, 0);
        data.itemsClosedCircuitVoltageGroup.append(index.data(Qt::CheckStateRole));
    }
    data.dArrayClosedCircuitVoltageGroup = dArrayClosedCircuitVoltageGroup;

    /// 6. Распассивация /*переделать!*/
    for (int i = 0; i < ui->cbDepassivation->count(); i++)
    {
        index = ui->cbDepassivation->model()->index(i, 0);
        data.itemsDepassivation.append(index.data(Qt::CheckStateRole));
    }
    data.dArrayDepassivation = dArrayDepassivation;

    /// 7. Напряжение замкнутой цепи батареи
    data.dArrayClosedCircuitVoltageBattery = dArrayClosedCircuitVoltageBattery;

    /// только для батарей 9ER20P_20 или 9ER14PS_24
    if (iBatteryIndex == 0 or iBatteryIndex == 1) {
        /// 8. Сопротивление изоляции УУТББ
        for (int r = 0; r < ui->cbInsulationResistanceUUTBB->count(); r++)
        {
            index = ui->cbInsulationResistanceUUTBB->model()->index(r, 0);
            data.itemsInsulationResistanceUUTBB.append(index.data(Qt::CheckStateRole));
        }
        data.dArrayInsulationResistanceUUTBB = dArrayInsulationResistanceUUTBB;

        /// 9. Напряжение разомкнутой цепи БП
        data.dArrayOpenCircuitVoltagePowerSupply = dArrayOpenCircuitVoltagePowerSupply;

        /// 10. Напряжение замкнутой цепи БП
        data.icbClosedCircuitVoltagePowerSupply = ui->cbClosedCircuitVoltagePowerSupply->currentIndex();
        data.dArrayClosedCircuitVoltagePowerSupply = dArrayClosedCircuitVoltagePowerSupply;
    }

    /// выполненые проверки для отчета
    data.sArrayReport = sArrayReport;
    data.sArrayReportGraphDescription = sArrayReportGraphDescription;
    data.imgArrayReportGraph = imgArrayReportGraph;

    list << data;

    /// открывает диалог для размещения файла сохраненния проверки и задания его имени
    QDateTime dateTime = QDateTime::currentDateTime();
    QString textDateTime = dateTime.toString("yyyy-MM-dd-hh-mm-ss-zzz");
    QString fileName = QFileDialog::getSaveFileName(this, tr("Сохранить проверку"), tr("%0_%1.dat").arg(ui->comboBoxBatteryList->currentText()).arg(textDateTime), tr("Проверка (*.dat)"));
    QFile file(fileName);
    if(!file.open(QIODevice::WriteOnly))
        return;
    if (QFileInfo(fileName).baseName().length() <= 0) /// без имени файла не сохраняем
        return;
    if(QFileInfo(fileName).suffix().isEmpty())
        fileName.append(".dat");

    QDataStream stream(&file);
    stream.setVersion(QDataStream::Qt_DefaultCompiledVersion); /// устанвливаем версию нашего компилятора
    stream << list; /// передаем массив в поток
    file.close(); /// закрываем файл

    Log(tr("Проверка успешно сохранена в файл <b>\"%0.dat\"</b>.").arg(QFileInfo(fileName).baseName()), "green");
    QMessageBox::information(this, "Проверка", "Проверка успешно сохранена в файл!"); // выводим сообщение о завершении сохранения проверки в файла
}


/*!
 * \brief MainWindow::on_actionCheckLoad_triggered
 */
void MainWindow::on_actionCheckLoad_triggered()
{
    qDebug() << "on_actionCheckLoad_triggered()";
    QList<dataBattery> list;

    QString fileName = QFileDialog::getOpenFileName(this, tr("Загрузить проверку"), "", tr("Проверка (*.dat)"));
    QFile file(fileName);
    if(!file.open(QIODevice::ReadOnly))
      return;

    QDataStream stream(&file);
    stream.setVersion(QDataStream::Qt_DefaultCompiledVersion);
    stream >> list;
    file.close();

    foreach( dataBattery data, list ) {
        ui->comboBoxBatteryList->setCurrentIndex(QString::number(data.iBatteryIndex).toInt());
        ui->cbIsUUTBB->setChecked(data.bIsUTTBB);
        ui->cbIsImitator->setChecked(data.bIsImitator);
        ui->dateEditBatteryBuild->setDate(data.dateBuild);
        ui->lineEditBatteryNumber->setText(data.sNumber);
        ui->rbModeDiagnosticAuto->setChecked(data.bModeDiagnosticAuto);
        ui->rbModeDiagnosticManual->setChecked(data.bModeDiagnosticManual);
        ui->cbParamsAutoMode->setCurrentIndex(data.icbParamsAutoMode);
        ui->cbSubParamsAutoMode->setCurrentIndex(data.icbSubParamsAutoMode);

        /// выполненые проверки для отчета
        sArrayReport = data.sArrayReport;
        sArrayReportGraphDescription = data.sArrayReportGraphDescription;
        imgArrayReportGraph = data.imgArrayReportGraph;

        /// 1. Напряжения на корпусе
        for (int i = 0; i < 2; i++) {
            dArrayVoltageOnTheHousing[i] = data.dArrayVoltageOnTheHousing[i];
            item = new QStandardItem(QString("%0").arg(battery[iBatteryIndex].str_voltage_corpus[i]));
            item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
            item->setData(data.itemsVoltageOnTheHousing[i+1], Qt::CheckStateRole);
            modelVoltageOnTheHousing->setItem(i+1, 0, item);

            if (dArrayVoltageOnTheHousing[i] != -1) {
                str = tr("%0) \"%1\" = <b>%2</b> В.").arg(i+1).arg(battery[iBatteryIndex].str_voltage_corpus[i]).arg(dArrayVoltageOnTheHousing[i], 0, 'f', 2);
                label = findChild<QLabel*>(tr("labelVoltageOnTheHousing%0").arg(i));
                if (dArrayVoltageOnTheHousing[i] > settings.voltage_corpus_limit) {
                    sResult = "Не норма!";
                    color = "red";
                }
                else {
                    sResult = "Норма";
                    color = "green";
                }
                label->setText(str+" "+sResult);
                label->setStyleSheet("QLabel { color : "+color+"; }");
            }
        }
        if (!sArrayReport.isEmpty())
            ui->tabWidget->addTab(ui->tabVoltageOnTheHousing, ui->rbVoltageOnTheHousing->text());

        /// 2. Сопротивление изоляции
        for (int i = 0; i < battery[iBatteryIndex].i_isolation_resistance_num; i++) {
            dArrayInsulationResistance[i] = data.dArrayInsulationResistance[i];
            item = new QStandardItem(QString("%0").arg(battery[iBatteryIndex].str_isolation_resistance[i]));
            item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
            item->setData(data.itemsInsulationResistance[i+1], Qt::CheckStateRole);
            modelInsulationResistance->setItem(i+1, 0, item);

            if (dArrayInsulationResistance[i] != -1) {
                str = tr("%0) \"%1\" = <b>%2</b> МОм.").arg(i+1).arg(battery[iBatteryIndex].str_isolation_resistance[i]).arg(dArrayInsulationResistance[i]/1000000, 0, 'f', 1);
                label = findChild<QLabel*>(tr("labelInsulationResistance%0").arg(i));
                if (dArrayInsulationResistance[i] < settings.isolation_resistance_limit) {
                    sResult = "Не норма!";
                    color = "red";
                }
                else {
                    sResult = "Норма";
                    color = "green";
                }
                label->setText(str+" "+sResult);
                label->setStyleSheet("QLabel { color : "+color+"; }");
            }
        }
        if (!sArrayReport.isEmpty())
            ui->tabWidget->addTab(ui->tabInsulationResistance, ui->rbInsulationResistance->text());


        /// 3. Напряжение разомкнутой цепи группы
        for (int i = 0; i < battery[iBatteryIndex].group_num; i++)
        {
            dArrayOpenCircuitVoltageGroup[i] = data.dArrayOpenCircuitVoltageGroup[i];
            item = new QStandardItem(QString("%0").arg(battery[iBatteryIndex].circuitgroup[i]));
            item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
            item->setData(data.itemsOpenCircuitVoltageGroup[i+1], Qt::CheckStateRole);
            modelOpenCircuitVoltageGroup->setItem(i+1, 0, item);

            if (dArrayOpenCircuitVoltageGroup[i] != -1) {
                if (dArrayOpenCircuitVoltageGroup[i] < settings.opencircuitgroup_limit_min) {
                    sResult = "Не норма!";
                    color = "red";

                    /// пишем на label'ах НЗЦг что проверка под нагрузкой запрещена
                    sLabelText = tr("%0) \"%1\"").arg(i+1).arg(battery[iBatteryIndex].circuitgroup[i]);
                    label = findChild<QLabel*>(tr("labelClosedCircuitVoltageGroup%0").arg(i));
                    label->setText(tr("%0) НРЦг <нормы, проверка под нагрузкой запрещена.").arg(i+1));
                    label->setStyleSheet("QLabel { color : "+color+"; }");
                } else {
                    sResult = "Норма";
                    color = "green";
                }
                sLabelText = tr("%0) \"%1\"").arg(i+1).arg(battery[iBatteryIndex].circuitgroup[i]);
                label = findChild<QLabel*>(tr("labelOpenCircuitVoltageGroup%0").arg(i));
                label->setText(str+" "+sResult);
                label->setText(tr("%0 = <b>%1</b> В. %2").arg(sLabelText).arg(dArrayOpenCircuitVoltageGroup[i], 0, 'f', 2).arg(sResult));
                label->setStyleSheet("QLabel { color : "+color+"; }");
            }
        }
        if (!sArrayReport.isEmpty())
            ui->tabWidget->addTab(ui->tabOpenCircuitVoltageGroup, ui->rbOpenCircuitVoltageGroup->text());


        /// 4. Напряжение разомкнутой цепи батареи
        for (int i = 0; i < 1; i++) {
            dArrayOpenCircuitVoltageBattery[i] = data.dArrayOpenCircuitVoltageBattery[i];
            if (dArrayOpenCircuitVoltageBattery[i] != -1) {
                str = tr("%0) \"%1\" = <b>%2</b> В.").arg(i+1).arg(battery[iBatteryIndex].circuitbattery).arg(dArrayOpenCircuitVoltageBattery[i], 0, 'f', 2);
                label = findChild<QLabel*>(tr("labelOpenCircuitVoltageBattery%0").arg(i));
                if (dArrayOpenCircuitVoltageBattery[i] < settings.opencircuitbattery_limit) {
                    sResult = "Не норма!";
                    color = "red";
                }
                else {
                    sResult = "Норма";
                    color = "green";
                }
                label->setText(str+" "+sResult);
                label->setStyleSheet("QLabel { color : "+color+"; }");
            }
        }
        if (!sArrayReport.isEmpty())
            ui->tabWidget->addTab(ui->tabOpenCircuitVoltageBattery, ui->rbOpenCircuitVoltageBattery->text());


        /// 5. Напряжение замкнутой цепи группы
        for (int i = 0; i < battery[iBatteryIndex].group_num; i++)
        {
            dArrayClosedCircuitVoltageGroup[i] = data.dArrayClosedCircuitVoltageGroup[i];

            if (dArrayClosedCircuitVoltageGroup[i] != -1) {
                /// разрешаем чекбоксы и ставим галки
                item = new QStandardItem(QString("%0").arg(battery[iBatteryIndex].circuitgroup[i]));
                item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
                item->setData(data.itemsClosedCircuitVoltageGroup[i+1], Qt::CheckStateRole);
                modelClosedCircuitVoltageGroup->setItem(i+1, 0, item);

                if (dArrayClosedCircuitVoltageGroup[i] < settings.closecircuitgroup_limit) {
                    sResult = "Не норма!";
                    color = "red";
                }
                else {
                    sResult = "Норма";
                    color = "green";
                }
                sLabelText = tr("%0) \"%1\"").arg(i+1).arg(battery[iBatteryIndex].circuitgroup[i]);
                label = findChild<QLabel*>(tr("labelClosedCircuitVoltageGroup%0").arg(i));
                label->setText(tr("%0 = <b>%1</b> В. %2").arg(sLabelText).arg(dArrayClosedCircuitVoltageGroup[i], 0, 'f', 2).arg(sResult));
                label->setStyleSheet("QLabel { color : "+color+"; }");
            }
        }
        if (!sArrayReport.isEmpty())
            ui->tabWidget->addTab(ui->tabClosedCircuitVoltageGroup, ui->rbClosedCircuitVoltageGroup->text());


        /// 6. Распассивация /*переделать!*/
        for (int i = 0; i < battery[iBatteryIndex].group_num; i++)
        {
            dArrayDepassivation[i] = data.dArrayDepassivation[i];
            if (dArrayDepassivation[i] != -1) {
                item = new QStandardItem(QString("%0").arg(battery[iBatteryIndex].circuitgroup[i]));
                if (data.itemsDepassivation[i+1] == 2)
                    item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
                else
                    item->setFlags(Qt::NoItemFlags);
                item->setData(data.itemsDepassivation[i+1], Qt::CheckStateRole);
                modelDepassivation->setItem(i+1, 0, item);

                str = tr("%0) \"%1\" = <b>%2</b> В.").arg(i+1).arg(battery[iBatteryIndex].circuitgroup[i]).arg(dArrayDepassivation[i]);
                label = findChild<QLabel*>(tr("labelDepassivation%0").arg(i));
                if (dArrayDepassivation[i] > settings.closecircuitgroup_limit) {
                    sResult = "Не норма!";
                    color = "red";
                }
                else {
                    sResult = "Норма";
                    color = "green";
                }
                label->setText(str+" "+sResult);
                label->setStyleSheet("QLabel { color : "+color+"; }");
            }
        }
        if (!sArrayReport.isEmpty())
            ui->tabWidget->addTab(ui->tabDepassivation, ui->rbDepassivation->text());


        /// 7. Напряжение замкнутой цепи батареи
        dArrayClosedCircuitVoltageBattery[0] = data.dArrayClosedCircuitVoltageBattery[0];
        if (dArrayClosedCircuitVoltageBattery[0] != -1) {
            str = tr("1) \"%0\" = <b>%1</b> В.").arg(battery[iBatteryIndex].circuitbattery).arg(dArrayClosedCircuitVoltageBattery[0], 0, 'f', 2);
            if (dArrayClosedCircuitVoltageBattery[0] < settings.closecircuitbattery_limit) {
                sResult = "Не норма!";
                color = "red";
            }
            else {
                sResult = "Норма";
                color = "green";
            }
            ui->labelClosedCircuitVoltageBattery0->setText(str+" "+sResult);
            ui->labelClosedCircuitVoltageBattery0->setStyleSheet("QLabel { color : "+color+"; }");
        }
        if (!sArrayReport.isEmpty())
            ui->tabWidget->addTab(ui->tabClosedCircuitVoltageBattery, ui->rbClosedCircuitVoltageBattery->text());


        /// только для батарей 9ER20P_20 или 9ER14PS_24
        if (iBatteryIndex == 0 or iBatteryIndex == 1) {
            /// 8. Сопротивление изоляции УУТББ
            for (int i = 0; i < battery[iBatteryIndex].i_uutbb_resist_num; i++)
            {
                dArrayInsulationResistanceUUTBB[i] = data.dArrayInsulationResistanceUUTBB[i];
                item = new QStandardItem(QString("%0").arg(battery[iBatteryIndex].uutbb_resist[i]));
                item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
                item->setData(data.itemsInsulationResistanceUUTBB[i+1], Qt::CheckStateRole);
                modelInsulationResistanceUUTBB->setItem(i+1, 0, item);

                if (dArrayInsulationResistanceUUTBB[i] != -1) {
                    if (dArrayInsulationResistanceUUTBB[i] < settings.uutbb_isolation_resist_limit) {
                        sResult = "Не норма!";
                        color = "red";
                    }
                    else {
                        sResult = "Норма";
                        color = "green";
                    }
                    sLabelText = tr("%0) \"%1\"").arg(i+1).arg(battery[iBatteryIndex].uutbb_resist[i]);
                    label = findChild<QLabel*>(tr("labelInsulationResistanceUUTBB%0").arg(i));
                    label->setText(tr("%0 = <b>%1</b> МОм. %2").arg(sLabelText).arg(dArrayInsulationResistanceUUTBB[i]/1000000, 0, 'f', 1).arg(sResult));
                    label->setStyleSheet("QLabel { color : "+color+"; }");
                }
            }
            if (!sArrayReport.isEmpty())
                ui->tabWidget->addTab(ui->tabInsulationResistanceUUTBB, ui->rbInsulationResistanceUUTBB->text());


            /// 9. Напряжение разомкнутой цепи БП
            dArrayOpenCircuitVoltagePowerSupply[0] = data.dArrayOpenCircuitVoltagePowerSupply[0];
            if (dArrayOpenCircuitVoltagePowerSupply[0] != -1) {
                if (dArrayOpenCircuitVoltagePowerSupply[0] < settings.uutbb_opencircuitpower_limit_min) {
                    sResult = "Не норма!";
                    color = "red";
                }
                else {
                    sResult = "Норма";
                    color = "green";
                }
                sLabelText = tr("1) \"%0\"").arg(battery[iBatteryIndex].uutbb_closecircuitpower[0]);
                ui->labelOpenCircuitVoltagePowerSupply0->setText(tr("%0 = <b>%1</b> В. %2").arg(sLabelText).arg(dArrayOpenCircuitVoltagePowerSupply[0], 0, 'f', 2).arg(sResult));
                ui->labelOpenCircuitVoltagePowerSupply0->setStyleSheet("QLabel { color : "+color+"; }");
                ui->tabWidget->addTab(ui->tabOpenCircuitVoltagePowerSupply, ui->rbOpenCircuitVoltagePowerSupply->text());
            }


            /// 10. Напряжение замкнутой цепи БП
            ui->cbClosedCircuitVoltagePowerSupply->setCurrentIndex(data.icbClosedCircuitVoltagePowerSupply);

            dArrayClosedCircuitVoltagePowerSupply[0] = data.dArrayClosedCircuitVoltagePowerSupply[0];
            if (dArrayClosedCircuitVoltagePowerSupply[0] != -1) {
                if (dArrayClosedCircuitVoltagePowerSupply[0] < settings.uutbb_closecircuitpower_limit) {
                    sResult = "Не норма!";
                    color = "red";
                }
                else {
                    sResult = "Норма";
                    color = "green";
                }
                sLabelText = tr("1) \"%0\"").arg(battery[iBatteryIndex].uutbb_closecircuitpower[0]);
                ui->labelClosedCircuitVoltagePowerSupply0->setText(tr("%0 = <b>%1</b> В. %2").arg(sLabelText).arg(dArrayClosedCircuitVoltagePowerSupply[0], 0, 'f', 2).arg(sResult));
                ui->labelClosedCircuitVoltagePowerSupply0->setStyleSheet("QLabel { color : "+color+"; }");
                ui->tabWidget->addTab(ui->tabClosedCircuitVoltagePowerSupply, ui->rbClosedCircuitVoltagePowerSupply->text());
            }
        }
    }
    ui->btnBuildReport->setEnabled(true); /// разрешаем кнопку формирования отчета
    ui->groupBoxCheckParams->setDisabled(true); /// запретить выбрать параметр проверки ручного режима
    ui->groupBoxCheckParamsAutoMode->setDisabled(true); /// запретить выбрать начальный параметр проверки автоматического режима
    Log(tr("Проверка успешно загружена из файла <b>\"%0.dat\"</b>.").arg(QFileInfo(fileName).baseName()), "green");
    QMessageBox::information(this, "Проверка", "Проверка успешно загружена из файла!"); // выводим сообщение о завершении загрузки проверки из файла
}
