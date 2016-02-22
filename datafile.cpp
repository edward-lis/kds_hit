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
            << data.imDepassivation
            << data.icbDepassivation
            << data.itemsInsulationResistanceUUTBB
            << data.icbClosedCircuitVoltagePowerSupply;
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
            >> data.imDepassivation
            >> data.icbDepassivation
            >> data.itemsInsulationResistanceUUTBB
            >> data.icbClosedCircuitVoltagePowerSupply;
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

    /// Сопротивление изоляции
    data.icbInsulationResistance = ui->cbInsulationResistance->currentIndex();

    /// Напряжение разомкнутой цепи группы
    for (int r = 0; r < ui->cbOpenCircuitVoltageGroup->count(); r++)
    {
        QModelIndex index = ui->cbOpenCircuitVoltageGroup->model()->index(r, 0);
        data.itemsOpenCircuitVoltageGroup.append(index.data(Qt::CheckStateRole));
    }

    /// Напряжение замкнутой цепи группы
    for (int r = 0; r < ui->cbClosedCircuitVoltageGroup->count(); r++)
    {
        QModelIndex index = ui->cbClosedCircuitVoltageGroup->model()->index(r, 0);
        data.itemsClosedCircuitVoltageGroup.append(index.data(Qt::CheckStateRole));
    }

    /// Распассивация
    data.imDepassivation = imDepassivation;
    data.icbDepassivation = ui->cbDepassivation->currentIndex();

    /// Сопротивление изоляции УУТББ
    for (int r = 0; r < ui->cbInsulationResistanceUUTBB->count(); r++)
    {
        QModelIndex index = ui->cbInsulationResistanceUUTBB->model()->index(r, 0);
        data.itemsInsulationResistanceUUTBB.append(index.data(Qt::CheckStateRole));
    }

    /// Напряжение замкнутой цепи БП
    data.icbClosedCircuitVoltagePowerSupply = ui->cbClosedCircuitVoltagePowerSupply->currentIndex();
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

        /// Сопротивление изоляции
        ui->cbInsulationResistance->setCurrentIndex(data.icbInsulationResistance);

        /// Напряжение разомкнутой цепи группы
        for (int r = 0; r < battery[iBatteryIndex].group_num; r++)
        {
            QStandardItem* item;
            item = new QStandardItem(QString("%0").arg(battery[iBatteryIndex].circuitgroup[r]));
            item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
            item->setData(data.itemsOpenCircuitVoltageGroup[r+1], Qt::CheckStateRole);
            modelOpenCircuitVoltageGroup->setItem(r+1, 0, item);
        }

        /// Напряжение замкнутой цепи группы
        for (int r = 0; r < battery[iBatteryIndex].group_num; r++)
        {
            QStandardItem* item;
            item = new QStandardItem(QString("%0").arg(battery[iBatteryIndex].circuitgroup[r]));
            item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
            item->setData(data.itemsClosedCircuitVoltageGroup[r+1], Qt::CheckStateRole);
            modelClosedCircuitVoltageGroup->setItem(r+1, 0, item);
        }

        /// Распассивация
        ui->cbDepassivation->clear();
        int i;
        foreach (i, data.imDepassivation)
            ui->cbDepassivation->addItem(battery[data.iBatteryIndex].circuitgroup[i]);
        ui->cbDepassivation->setCurrentIndex(data.icbDepassivation);

        /// Сопротивление изоляции УУТББ
        for (int r = 0; r < battery[iBatteryIndex].i_uutbb_resist_num; r++)
        {
            QStandardItem* item;
            item = new QStandardItem(QString("%0").arg(battery[iBatteryIndex].uutbb_resist[r]));
            item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
            item->setData(data.itemsInsulationResistanceUUTBB[r+1], Qt::CheckStateRole);
            modelInsulationResistanceUUTBB->setItem(r+1, 0, item);
        }

        /// Напряжение замкнутой цепи БП
        ui->cbClosedCircuitVoltagePowerSupply->setCurrentIndex(data.icbClosedCircuitVoltagePowerSupply);
    }

    Log(tr("Проверка успешно загружена из файла <b>\"%0.dat\"</b>.").arg(QFileInfo(fileName).baseName()), "green");
}
