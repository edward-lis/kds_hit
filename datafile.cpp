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
            << data.dArrayClosedCircuitVoltagePowerSupply;
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
            >> data.dArrayClosedCircuitVoltagePowerSupply;
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

    /// Распассивация
    for (int i = 0; i < ui->cbDepassivation->count(); i++)
    {
        QModelIndex index = ui->cbDepassivation->model()->index(i, 0);
        data.itemsDepassivation.append(index.data(Qt::CheckStateRole));
    }
    data.dArrayDepassivation = dArrayDepassivation;

    /// Напряжение замкнутой цепи батареи
    data.dArrayClosedCircuitVoltageBattery = dArrayClosedCircuitVoltageBattery;

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
    list << data;

    /// открывает диалог для размещения файла сохраненния проверки и задания его имени
    QDateTime dateTime = QDateTime::currentDateTime();
    QString textDateTime = dateTime.toString("yyyy-MM-dd-hh-mm-ss-zzz");
    QString fileName = QFileDialog::getSaveFileName(this, tr("Сохранить проверку"), tr("%0_%1.dat").arg(ui->comboBoxBatteryList->currentText()).arg(textDateTime), tr("Проверка (*.dat)"));
    QFile file(fileName);
    if(!file.open(QIODevice::WriteOnly) and fileName.length() < 5) /// без имени файла не сохраняем
        return;

    QDataStream stream(&file);
    stream.setVersion(QDataStream::Qt_DefaultCompiledVersion); /// устанвливаем версию нашего компилятора
    stream << list; /// передаем массив в поток
    file.close(); /// закрываем файл

    Log(tr("Проверка успешно сохранена в файл <b>\"%0.dat\"</b>.").arg(QFileInfo(fileName).baseName()), "green");
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
    if(!file.open(QIODevice::ReadOnly) and fileName.length() < 5) /// без имени файла не сохраняем
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

        /// Напряжение на корпусе
        ui->cbVoltageOnTheHousing->setCurrentIndex(data.icbVoltageOnTheHousing);
        for (int i = 0; i < 2; i++) {
            dArrayVoltageOnTheHousing[i] = data.dArrayVoltageOnTheHousing[i];
            str = tr("Напряжение цепи \"%0\" = <b>%1</b> В.").arg(battery[iBatteryIndex].str_voltage_corpus[i]).arg(dArrayVoltageOnTheHousing[i]);
            QLabel * label = findChild<QLabel*>(tr("labelVoltageOnTheHousing%0").arg(i));
            if (dArrayVoltageOnTheHousing[i] > settings.voltage_corpus_limit) {
                str += " Не норма.";
                color = "red";
            } else
                color = "green";
            label->setText(str);
            label->setStyleSheet("QLabel { color : "+color+"; }");
        }
        ui->tabWidget->addTab(ui->tabVoltageOnTheHousing, ui->rbVoltageOnTheHousing->text());

        /// Сопротивление изоляции
        ui->cbInsulationResistance->setCurrentIndex(data.icbInsulationResistance);
        for (int i = 0; i < battery[iBatteryIndex].i_isolation_resistance_num; i++) {
            dArrayInsulationResistance[i] = data.dArrayInsulationResistance[i];
            str = tr("Сопротивление цепи \"%0\" = <b>%1</b> МОм.").arg(battery[iBatteryIndex].str_isolation_resistance[i]).arg(dArrayInsulationResistance[i]);
            QLabel * label = findChild<QLabel*>(tr("labelInsulationResistance%0").arg(i));
            if (dArrayInsulationResistance[i] > settings.isolation_resistance_limit) {
                str += " Не норма.";
                color = "red";
            } else
                color = "green";
            label->setText(str);
            label->setStyleSheet("QLabel { color : "+color+"; }");
        }
        ui->tabWidget->addTab(ui->tabInsulationResistance, ui->rbInsulationResistance->text());

        /// Напряжение разомкнутой цепи группы
        for (int i = 0; i < battery[iBatteryIndex].group_num; i++)
        {
            QStandardItem* item;
            item = new QStandardItem(QString("%0").arg(battery[iBatteryIndex].circuitgroup[i]));
            item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
            item->setData(data.itemsOpenCircuitVoltageGroup[i+1], Qt::CheckStateRole);
            modelOpenCircuitVoltageGroup->setItem(i+1, 0, item);

            dArrayOpenCircuitVoltageGroup[i] = data.dArrayOpenCircuitVoltageGroup[i];
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
        ui->tabWidget->addTab(ui->tabOpenCircuitVoltageGroup, ui->rbOpenCircuitVoltageGroup->text());

        /// Напряжение разомкнутой цепи батареи
        for (int i = 0; i < 1; i++) {
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
        ui->tabWidget->addTab(ui->tabOpenCircuitVoltageBattery, ui->rbOpenCircuitVoltageBattery->text());

        /// Напряжение замкнутой цепи группы
        for (int i = 0; i < battery[iBatteryIndex].group_num; i++)
        {
            QStandardItem* item;
            item = new QStandardItem(QString("%0").arg(battery[iBatteryIndex].circuitgroup[i]));
            item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
            item->setData(data.itemsClosedCircuitVoltageGroup[i+1], Qt::CheckStateRole);
            modelClosedCircuitVoltageGroup->setItem(i+1, 0, item);

            dArrayClosedCircuitVoltageGroup[i] = data.dArrayClosedCircuitVoltageGroup[i];
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

        /// Распассивация
        for (int i = 0; i < battery[iBatteryIndex].group_num; i++)
        {
            QStandardItem* item;
            item = new QStandardItem(QString("%0").arg(battery[iBatteryIndex].circuitgroup[i]));
            if (data.itemsDepassivation[i+1] == 2)
                item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
            else
                item->setFlags(Qt::NoItemFlags);
            item->setData(data.itemsDepassivation[i+1], Qt::CheckStateRole);
            modelDepassivation->setItem(i+1, 0, item);

            dArrayDepassivation[i] = data.dArrayDepassivation[i];
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
        ui->tabWidget->addTab(ui->tabDepassivation, ui->rbDepassivation->text());

        /// Напряжение замкнутой цепи батареи
        for (int i = 0; i < 1; i++) {
            dArrayClosedCircuitVoltageBattery[i] = data.dArrayClosedCircuitVoltageBattery[i];
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
        ui->tabWidget->addTab(ui->tabClosedCircuitVoltageBattery, ui->rbClosedCircuitVoltageBattery->text());

        /// Сопротивление изоляции УУТББ
        for (int i = 0; i < battery[iBatteryIndex].i_uutbb_resist_num; i++)
        {
            QStandardItem* item;
            item = new QStandardItem(QString("%0").arg(battery[iBatteryIndex].uutbb_resist[i]));
            item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
            item->setData(data.itemsInsulationResistanceUUTBB[i+1], Qt::CheckStateRole);
            modelInsulationResistanceUUTBB->setItem(i+1, 0, item);

            dArrayInsulationResistanceUUTBB[i] = data.dArrayInsulationResistanceUUTBB[i];
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
        ui->tabWidget->addTab(ui->tabInsulationResistanceUUTBB, ui->rbInsulationResistanceUUTBB->text());

        /// Напряжение разомкнутой цепи БП
        for (int i = 0; i < 1; i++) {
            dArrayOpenCircuitVoltagePowerSupply[i] = data.dArrayOpenCircuitVoltagePowerSupply[i];
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
        ui->tabWidget->addTab(ui->tabOpenCircuitVoltagePowerSupply, ui->rbOpenCircuitVoltagePowerSupply->text());

        /// Напряжение замкнутой цепи БП
        ui->cbClosedCircuitVoltagePowerSupply->setCurrentIndex(data.icbClosedCircuitVoltagePowerSupply);
        for (int i = 0; i < 2; i++) {
            dArrayClosedCircuitVoltagePowerSupply[i] = data.dArrayClosedCircuitVoltagePowerSupply[i];
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
        ui->tabWidget->addTab(ui->tabClosedCircuitVoltagePowerSupply, ui->rbClosedCircuitVoltagePowerSupply->text());
    }
    ui->btnBuildReport->setEnabled(true);
    Log(tr("Проверка успешно загружена из файла <b>\"%0.dat\"</b>.").arg(QFileInfo(fileName).baseName()), "green");
}
