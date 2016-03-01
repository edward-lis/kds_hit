#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "datafile.h"

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
            << data.icbVoltageOnTheHousing
            << data.icbInsulationResistance
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
            << data.sArrayReportVoltageOnTheHousing
            << data.sArrayReportInsulationResistance
            << data.sArrayReportOpenCircuitVoltageGroup
            << data.sArrayReportOpenCircuitVoltageBattery
            << data.sArrayReportDepassivation
            << data.sArrayReportClosedCircuitVoltageGroup
            << data.sArrayReportClosedCircuitVoltageBattery
            << data.sArrayReportInsulationResistanceUUTBB
            << data.sArrayReportOpenCircuitVoltagePowerSupply
            << data.sArrayReportClosedCircuitVoltagePowerSupply;
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
            >> data.icbVoltageOnTheHousing
            >> data.icbInsulationResistance
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
            >> data.sArrayReportVoltageOnTheHousing
            >> data.sArrayReportInsulationResistance
            >> data.sArrayReportOpenCircuitVoltageGroup
            >> data.sArrayReportOpenCircuitVoltageBattery
            >> data.sArrayReportDepassivation
            >> data.sArrayReportClosedCircuitVoltageGroup
            >> data.sArrayReportClosedCircuitVoltageBattery
            >> data.sArrayReportInsulationResistanceUUTBB
            >> data.sArrayReportOpenCircuitVoltagePowerSupply
            >> data.sArrayReportClosedCircuitVoltagePowerSupply;
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

    /// Напряжение на корпусе
    data.icbVoltageOnTheHousing = ui->cbVoltageOnTheHousing->currentIndex();
    data.dArrayVoltageOnTheHousing = dArrayVoltageOnTheHousing;

    /// Сопротивление изоляции
    data.icbInsulationResistance = ui->cbInsulationResistance->currentIndex();
    data.dArrayInsulationResistance = dArrayInsulationResistance;

    /// Напряжение разомкнутой цепи группы
    for (int r = 0; r < ui->cbOpenCircuitVoltageGroup->count(); r++)
    {
        QModelIndex index = ui->cbOpenCircuitVoltageGroup->model()->index(r, 0);
        data.itemsOpenCircuitVoltageGroup.append(index.data(Qt::CheckStateRole));
    }
    data.dArrayOpenCircuitVoltageGroup = dArrayOpenCircuitVoltageGroup;

    /// Напряжение разомкнутой цепи батареи
    data.dArrayOpenCircuitVoltageBattery = dArrayOpenCircuitVoltageBattery;

    /// Напряжение замкнутой цепи группы
    for (int r = 0; r < ui->cbClosedCircuitVoltageGroup->count(); r++)
    {
        QModelIndex index = ui->cbClosedCircuitVoltageGroup->model()->index(r, 0);
        data.itemsClosedCircuitVoltageGroup.append(index.data(Qt::CheckStateRole));
    }
    data.dArrayClosedCircuitVoltageGroup = dArrayClosedCircuitVoltageGroup;

    /// Распассивация /*переделать!*/
    for (int i = 0; i < ui->cbDepassivation->count(); i++)
    {
        QModelIndex index = ui->cbDepassivation->model()->index(i, 0);
        data.itemsDepassivation.append(index.data(Qt::CheckStateRole));
    }
    data.dArrayDepassivation = dArrayDepassivation;

    /// Напряжение замкнутой цепи батареи
    data.dArrayClosedCircuitVoltageBattery = dArrayClosedCircuitVoltageBattery;

    /// только для батарей 9ER20P_20 или 9ER14PS_24
    if (iBatteryIndex == 0 or iBatteryIndex == 1) {
        /// Сопротивление изоляции УУТББ
        for (int r = 0; r < ui->cbInsulationResistanceUUTBB->count(); r++)
        {
            QModelIndex index = ui->cbInsulationResistanceUUTBB->model()->index(r, 0);
            data.itemsInsulationResistanceUUTBB.append(index.data(Qt::CheckStateRole));
        }
        data.dArrayInsulationResistanceUUTBB = dArrayInsulationResistanceUUTBB;

        /// Напряжение разомкнутой цепи БП
        data.dArrayOpenCircuitVoltagePowerSupply = dArrayOpenCircuitVoltagePowerSupply;

        /// Напряжение замкнутой цепи БП
        data.icbClosedCircuitVoltagePowerSupply = ui->cbClosedCircuitVoltagePowerSupply->currentIndex();
        data.dArrayClosedCircuitVoltagePowerSupply = dArrayClosedCircuitVoltagePowerSupply;
    }

    /// выполненые проверки для отчета
    data.sArrayReportVoltageOnTheHousing = sArrayReportVoltageOnTheHousing;
    data.sArrayReportInsulationResistance = sArrayReportInsulationResistance;
    data.sArrayReportOpenCircuitVoltageGroup = sArrayReportOpenCircuitVoltageGroup;
    data.sArrayReportOpenCircuitVoltageBattery = sArrayReportOpenCircuitVoltageBattery;
    data.sArrayReportDepassivation = sArrayReportDepassivation;
    data.sArrayReportClosedCircuitVoltageGroup = sArrayReportClosedCircuitVoltageGroup;
    data.sArrayReportClosedCircuitVoltageBattery = sArrayReportClosedCircuitVoltageBattery;
    data.sArrayReportInsulationResistanceUUTBB = sArrayReportInsulationResistanceUUTBB;
    data.sArrayReportOpenCircuitVoltagePowerSupply = sArrayReportOpenCircuitVoltagePowerSupply;
    data.sArrayReportClosedCircuitVoltagePowerSupply = sArrayReportClosedCircuitVoltagePowerSupply;

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
        sArrayReportVoltageOnTheHousing = data.sArrayReportVoltageOnTheHousing;
        sArrayReportInsulationResistance = data.sArrayReportInsulationResistance;
        sArrayReportOpenCircuitVoltageGroup = data.sArrayReportOpenCircuitVoltageGroup;
        sArrayReportOpenCircuitVoltageBattery = data.sArrayReportOpenCircuitVoltageBattery;
        sArrayReportDepassivation = data.sArrayReportDepassivation;
        sArrayReportClosedCircuitVoltageGroup = data.sArrayReportClosedCircuitVoltageGroup;
        sArrayReportClosedCircuitVoltageBattery = data.sArrayReportClosedCircuitVoltageBattery;
        sArrayReportInsulationResistanceUUTBB = data.sArrayReportInsulationResistanceUUTBB;
        sArrayReportOpenCircuitVoltagePowerSupply = data.sArrayReportOpenCircuitVoltagePowerSupply;
        sArrayReportClosedCircuitVoltagePowerSupply = data.sArrayReportClosedCircuitVoltagePowerSupply;

        /// Напряжение на корпусе
        ui->cbVoltageOnTheHousing->setCurrentIndex(data.icbVoltageOnTheHousing);
        for (int i = 0; i < 2; i++) {
            dArrayVoltageOnTheHousing[i] = data.dArrayVoltageOnTheHousing[i];
            if (dArrayVoltageOnTheHousing[i] != -1) {
                str = tr("Напряжение цепи \"%0\" = <b>%1</b> В.").arg(battery[iBatteryIndex].str_voltage_corpus[i]).arg(dArrayVoltageOnTheHousing[i], 0, 'f', 2);
                QLabel * label = findChild<QLabel*>(tr("labelVoltageOnTheHousing%0").arg(i));
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
        if (!sArrayReportVoltageOnTheHousing.isEmpty())
            ui->tabWidget->addTab(ui->tabVoltageOnTheHousing, ui->rbVoltageOnTheHousing->text());

        /// Сопротивление изоляции
        ui->cbInsulationResistance->setCurrentIndex(data.icbInsulationResistance);
        for (int i = 0; i < battery[iBatteryIndex].i_isolation_resistance_num; i++) {
            dArrayInsulationResistance[i] = data.dArrayInsulationResistance[i];
            if (dArrayInsulationResistance[i] != -1) {
                str = tr("Сопротивление цепи \"%0\" = <b>%1</b> МОм.").arg(battery[iBatteryIndex].str_isolation_resistance[i]).arg(dArrayInsulationResistance[i], 0, 'g', 0);
                QLabel * label = findChild<QLabel*>(tr("labelInsulationResistance%0").arg(i));
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
        if (!sArrayReportInsulationResistance.isEmpty())
            ui->tabWidget->addTab(ui->tabInsulationResistance, ui->rbInsulationResistance->text());


        /// Напряжение разомкнутой цепи группы
        for (int i = 0; i < battery[iBatteryIndex].group_num; i++)
        {
            dArrayOpenCircuitVoltageGroup[i] = data.dArrayOpenCircuitVoltageGroup[i];
            QStandardItem* item;
            item = new QStandardItem(QString("%0").arg(battery[iBatteryIndex].circuitgroup[i]));
            item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
            item->setData(data.itemsOpenCircuitVoltageGroup[i+1], Qt::CheckStateRole);
            modelOpenCircuitVoltageGroup->setItem(i+1, 0, item);

            if (dArrayOpenCircuitVoltageGroup[i] != -1) {
                str = tr("Напряжение цепи \"%0\" = <b>%1</b> В.").arg(battery[iBatteryIndex].circuitgroup[i]).arg(dArrayOpenCircuitVoltageGroup[i]);
                QLabel * label = findChild<QLabel*>(tr("labelOpenCircuitVoltageGroup%0").arg(i));
                if (dArrayOpenCircuitVoltageGroup[i] > settings.closecircuitgroup_limit) {
                    str += " Не норма.";
                    color = "red";
                } else
                    color = "green";
                label->setText(str);
                label->setStyleSheet("QLabel { color : "+color+"; }");
            }
        }
        if (!sArrayReportOpenCircuitVoltageGroup.isEmpty())
            ui->tabWidget->addTab(ui->tabOpenCircuitVoltageGroup, ui->rbOpenCircuitVoltageGroup->text());


        /// Напряжение разомкнутой цепи батареи
        for (int i = 0; i < 1; i++) {
            if (dArrayOpenCircuitVoltageBattery[i] != -1) {
                dArrayOpenCircuitVoltageBattery[i] = data.dArrayOpenCircuitVoltageBattery[i];
                str = tr("Напряжение цепи \"%0\" = <b>%1</b> В.").arg(battery[iBatteryIndex].circuitbattery).arg(dArrayOpenCircuitVoltageBattery[i]);
                QLabel * label = findChild<QLabel*>(tr("labelOpenCircuitVoltageBattery%0").arg(i));
                if (dArrayOpenCircuitVoltageBattery[i] > settings.opencircuitbattery_limit) {
                    str += " Не норма.";
                    color = "red";
                } else
                    color = "green";
                label->setText(str);
                label->setStyleSheet("QLabel { color : "+color+"; }");
            }
        }
        if (!sArrayReportOpenCircuitVoltageBattery.isEmpty())
            ui->tabWidget->addTab(ui->tabOpenCircuitVoltageBattery, ui->rbOpenCircuitVoltageBattery->text());


        /// Напряжение замкнутой цепи группы
        for (int i = 0; i < battery[iBatteryIndex].group_num; i++)
        {
            dArrayClosedCircuitVoltageGroup[i] = data.dArrayClosedCircuitVoltageGroup[i];
            QStandardItem* item;
            item = new QStandardItem(QString("%0").arg(battery[iBatteryIndex].circuitgroup[i]));
            item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
            item->setData(data.itemsClosedCircuitVoltageGroup[i+1], Qt::CheckStateRole);
            modelClosedCircuitVoltageGroup->setItem(i+1, 0, item);

            if (dArrayClosedCircuitVoltageGroup[i] != -1) {
                str = tr("Напряжение цепи \"%0\" = <b>%1</b> В.").arg(battery[iBatteryIndex].circuitgroup[i]).arg(dArrayClosedCircuitVoltageGroup[i]);
                QLabel * label = findChild<QLabel*>(tr("labelClosedCircuitVoltageGroup%0").arg(i));
                if (dArrayClosedCircuitVoltageGroup[i] > settings.closecircuitgroup_limit) {
                    str += " Не норма.";
                    color = "red";
                } else
                    color = "green";
                label->setText(str);
                label->setStyleSheet("QLabel { color : "+color+"; }");
            }
        }
        if (!sArrayReportClosedCircuitVoltageGroup.isEmpty())
            ui->tabWidget->addTab(ui->tabClosedCircuitVoltageGroup, ui->rbClosedCircuitVoltageGroup->text());


        /// Распассивация /*переделать!*/
        for (int i = 0; i < battery[iBatteryIndex].group_num; i++)
        {
            dArrayDepassivation[i] = data.dArrayDepassivation[i];
            if (dArrayDepassivation[i] != -1) {
                QStandardItem* item;
                item = new QStandardItem(QString("%0").arg(battery[iBatteryIndex].circuitgroup[i]));
                if (data.itemsDepassivation[i+1] == 2)
                    item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
                else
                    item->setFlags(Qt::NoItemFlags);
                item->setData(data.itemsDepassivation[i+1], Qt::CheckStateRole);
                modelDepassivation->setItem(i+1, 0, item);

                str = tr("Напряжение цепи \"%0\" = <b>%1</b> В.").arg(battery[iBatteryIndex].circuitgroup[i]).arg(dArrayDepassivation[i]);
                QLabel * label = findChild<QLabel*>(tr("labelDepassivation%0").arg(i));
                if (dArrayDepassivation[i] > settings.closecircuitgroup_limit) {
                    str += " Не норма.";
                    color = "red";
                } else
                    color = "green";
                label->setText(str);
                label->setStyleSheet("QLabel { color : "+color+"; }");
            }
        }
        if (!sArrayReportDepassivation.isEmpty())
            ui->tabWidget->addTab(ui->tabDepassivation, ui->rbDepassivation->text());


        /// Напряжение замкнутой цепи батареи
        for (int i = 0; i < 1; i++) {
            dArrayClosedCircuitVoltageBattery[i] = data.dArrayClosedCircuitVoltageBattery[i];
            if (dArrayClosedCircuitVoltageBattery[i] != -1) {
                str = tr("Напряжение цепи \"%0\" = <b>%1</b> В.").arg(battery[iBatteryIndex].circuitbattery).arg(dArrayClosedCircuitVoltageBattery[i]);
                QLabel * label = findChild<QLabel*>(tr("labelClosedCircuitVoltageBattery%0").arg(i));
                if (dArrayClosedCircuitVoltageBattery[i] > settings.closecircuitbattery_limit) {
                    str += " Не норма.";
                    color = "red";
                } else
                    color = "green";
                label->setText(str);
                label->setStyleSheet("QLabel { color : "+color+"; }");
            }
        }
        if (!sArrayReportClosedCircuitVoltageBattery.isEmpty())
            ui->tabWidget->addTab(ui->tabClosedCircuitVoltageBattery, ui->rbClosedCircuitVoltageBattery->text());


        /// только для батарей 9ER20P_20 или 9ER14PS_24
        if (iBatteryIndex == 0 or iBatteryIndex == 1) {
            /// Сопротивление изоляции УУТББ
            for (int i = 0; i < battery[iBatteryIndex].i_uutbb_resist_num; i++)
            {
                dArrayInsulationResistanceUUTBB[i] = data.dArrayInsulationResistanceUUTBB[i];
                QStandardItem* item;
                item = new QStandardItem(QString("%0").arg(battery[iBatteryIndex].uutbb_resist[i]));
                item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
                item->setData(data.itemsInsulationResistanceUUTBB[i+1], Qt::CheckStateRole);
                modelInsulationResistanceUUTBB->setItem(i+1, 0, item);

                if (dArrayInsulationResistanceUUTBB[i] != -1) {
                    str = tr("\"%0\" = <b>%1</b> МОм.").arg(battery[iBatteryIndex].uutbb_resist[i]).arg(dArrayInsulationResistanceUUTBB[i]);
                    QLabel * label = findChild<QLabel*>(tr("labelInsulationResistanceUUTBB%0").arg(i));
                    if (dArrayInsulationResistanceUUTBB[i] > settings.uutbb_isolation_resist_limit) {
                        str += " Не норма.";
                        color = "red";
                    } else
                        color = "green";
                    label->setText(str);
                    label->setStyleSheet("QLabel { color : "+color+"; }");
                }
            }
            if (!sArrayReportInsulationResistanceUUTBB.isEmpty())
                ui->tabWidget->addTab(ui->tabInsulationResistanceUUTBB, ui->rbInsulationResistanceUUTBB->text());


            /// Напряжение разомкнутой цепи БП
            for (int i = 0; i < 1; i++) {
                dArrayOpenCircuitVoltagePowerSupply[i] = data.dArrayOpenCircuitVoltagePowerSupply[i];
                if (dArrayOpenCircuitVoltagePowerSupply[i] != -1) {
                    str = tr("Напряжение цепи \"%0\" = <b>%1</b> В.").arg(battery[iBatteryIndex].uutbb_closecircuitpower[i]).arg(dArrayOpenCircuitVoltagePowerSupply[i]);
                    QLabel * label = findChild<QLabel*>(tr("labelOpenCircuitVoltagePowerSupply%0").arg(i));
                    if (dArrayOpenCircuitVoltagePowerSupply[i] < settings.uutbb_opencircuitpower_limit_min or dArrayOpenCircuitVoltagePowerSupply[i] > settings.uutbb_opencircuitpower_limit_max) {
                        str += " Не норма.";
                        color = "red";
                    } else
                        color = "green";
                    label->setText(str);
                    label->setStyleSheet("QLabel { color : "+color+"; }");
                }
            }
            if (!sArrayReportOpenCircuitVoltagePowerSupply.isEmpty())
                ui->tabWidget->addTab(ui->tabOpenCircuitVoltagePowerSupply, ui->rbOpenCircuitVoltagePowerSupply->text());


            /// Напряжение замкнутой цепи БП
            ui->cbClosedCircuitVoltagePowerSupply->setCurrentIndex(data.icbClosedCircuitVoltagePowerSupply);
            for (int i = 0; i < 2; i++) {
                dArrayClosedCircuitVoltagePowerSupply[i] = data.dArrayClosedCircuitVoltagePowerSupply[i];
                if (dArrayClosedCircuitVoltagePowerSupply[i] != -1) {
                    str = tr("Напряжение цепи \"%0\" = <b>%1</b> В.").arg(battery[iBatteryIndex].uutbb_closecircuitpower[i]).arg(dArrayClosedCircuitVoltagePowerSupply[i]);
                    QLabel * label = findChild<QLabel*>(tr("labelClosedCircuitVoltagePowerSupply%0").arg(i));
                    if (dArrayClosedCircuitVoltagePowerSupply[i] > settings.uutbb_closecircuitpower_limit) {
                        str += " Не норма.";
                        color = "red";
                    } else
                        color = "green";
                    label->setText(str);
                    label->setStyleSheet("QLabel { color : "+color+"; }");
                }
            }
            if (!sArrayReportClosedCircuitVoltagePowerSupply.isEmpty())
                ui->tabWidget->addTab(ui->tabClosedCircuitVoltagePowerSupply, ui->rbClosedCircuitVoltagePowerSupply->text());
        }
    }
    ui->btnBuildReport->setEnabled(true); /// разрешаем кнопку формирования отчета
    ui->groupBoxCheckParams->setDisabled(true); /// запретить выбрать параметр проверки ручного режима
    ui->groupBoxCheckParamsAutoMode->setDisabled(true); /// запретить выбрать начальный параметр проверки автоматического режима
    Log(tr("Проверка успешно загружена из файла <b>\"%0.dat\"</b>.").arg(QFileInfo(fileName).baseName()), "green");
    QMessageBox::information(this, "Проверка", "Проверка успешно загружена из файла!"); // выводим сообщение о завершении загрузки проверки из файла
}
