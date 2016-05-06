#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "settings.h"

extern Settings settings;

// Приём данных от последовательного порта
void MainWindow::recvSerialData(quint8 operation_code, const QByteArray data)
{
    //qDebug()<<"recvSerialData"<<data<<"command:"<<baSendCommand;
    // когда приняли данные вовремя - остановить таймаут
    timeoutResponse->stop();
    //ui->statusBar->showMessage(tr(ONLINE)); // напишем в строке статуса, что связь есть
    if(operation_code == 0x01) // если приняли пинг, то
    {
        // следующий пинг пошлётся по окончанию timerPing
        if(data == baSendCommand) // вот тут по-хорошему надо бы по результам анализа ответа делать что-то полезное. только хз.
        {
            //qDebug()<<"ping correct";
            ui->statusBar->showMessage(tr(ONLINE)); // напишем в строке статуса, что связь есть, только при нормальном пинге
            if(bFirstPing) // если первый ответ после установления связи
            {
                // сбросить коробок, послать IDLE
                baSendArray = (baSendCommand="IDLE")+"#"; // подготовить буфер для передачи
                sendSerialData(); // послать baSendArray в порт
                // !!! первую посылку айдл надо бы куда-нить в другое место
                //ret=loop.exec(); // ждём ответа. по сигналу о готовности принятых данных, вывалимся из цикла
                //qDebug()<<"ret=loop.exec()"<<ret;
                //qDebug()<<baRecvArray; // принятый массив
                //baRecvArray.clear();
            }
            return;
        }
        else
        {
            qDebug()<<"ping incorrect";
            //return;
        }
//        return;
    }
    if(operation_code == 0x08) // если приняли ответ на команду
    {
        if( data.contains(baSendCommand) && data.contains("OK") ) // Команда#OK режима отработана/ совсем простенькая проверка на наличии в ответе OK
        {
            qDebug()<<"recvSerialData"<<data<<"command:"<<baSendCommand;
            if(baSendCommand.contains("IDLE")) // если команда равна IDLE
            {
                // взять номер комплекта платы
                bool ok;
                int j=QString(data[data.indexOf("OK")+QString("OK").length()]).toInt(&ok);
                if(ok)
                    settings.board_counter = j;
            }
            baRecvArray=data;
            emit signalSerialDataReady(); // сигнал - данные готовы. цикл ожидания закончится.
            if(bFirstPing) // если это был ответ на первый айдл, то продолжить пинг
            {
                qDebug()<<"settings.board_counter"<<settings.board_counter;
                bFirstPing = false;
                sendPing();
                ui->btnCheckConnectedBattery->setEnabled(true); // т.к. коробочка на связи и сбросилась в исходное, то разрешим кнопочку "Проверить батарею"
            }
        }
        else // пришла какая-то другая посылка
        {
            qDebug()<<"Incorrect reply. Should be "<<(baSendCommand + " and OK")<<" but got: "<<data;
            if(loop.isRunning()) loop.exit(KDS_INCORRECT_REPLY); // вывалиться из цикла ожидания приёма с кодом ошибки неправильной команды
        }
    }
}

// Посылка подготовленных данных baSendArray в порт
void MainWindow::sendSerialData()
{
    if(!bPortOpen) return;
    //qDebug()<<"sendSerialData"<<baSendArray;
    timerPing->stop(); // остановим таймеры. отключим пинг и предыдущий таймаут (если вдруг он был)
    timeoutResponse->stop();
    signalSendSerialData(8, baSendArray); // 8 - код операции, по протоколу.  здесь - всегда 8.
    timeoutResponse->start(delay_timeOut); // заведём тайм-аут на неответ
}

// Получить из принятого массива данные опроса
quint16 MainWindow::getRecvData(QByteArray baRecvArray)
{
    quint16 u=0;
    QDataStream ds(baRecvArray.left(2)); // Для преобразования первых двух байт в unsigned short
    ds>>u;
    return u;
}

// нет ответа на запрос
void MainWindow::procTimeoutResponse()
{
    //qDebug()<<"procTimeoutResponse";
    ui->statusBar->showMessage(tr(OFFLINE)); // напишем нет связи
    //emit signalTimeoutResponse();
    /// отключаем интерфейс, возобновление проверки только через проверку подключенной батареи
    ui->groupBoxDiagnosticMode->setDisabled(true);
    ui->groupBoxCheckParams->setDisabled(true);
    ui->groupBoxCheckParamsAutoMode->setDisabled(true);
    ui->btnBuildReport->setDisabled(true);
    ui->progressBar->reset(); /// сбросим прогресс бар
    bState = false; /// выходим из режима проверки

    if(loop.isRunning())
    {
        loop.exit(KDS_TIMEOUT); // вывалиться из цикла ожидания приёма с кодом ошибки таймаута
        baRecvArray.clear(); // очистить массив перед следующим приёмом
    }
}

// послать пинг
void MainWindow::sendPing()
{
    if(!bPortOpen) return;
    baSendCommand.clear();
    baSendCommand="PING";
    if(settings.verbose > 1) qDebug()<<"sendPing"<<baSendCommand;
    signalSendSerialData(1, baSendCommand);//PING);
    timeoutResponse->start(delay_timeOut); // заведём тайм-аут на неответ
    timerPing->start(delay_timerPing); // цикл между пингами
}

// Нажата кнопка открыть/закрыть порт
void MainWindow::on_btnCOMPortOpenClose_clicked()
{
    if(ui->comboBoxCOMPort->currentText().isEmpty()) // последовательных портов в системе нет
    {
        ui->statusBar->showMessage(tr("Нет последовательных портов"));
        return;
    }
    if(serialPort && ui->btnCOMPortOpenClose->text()==tr("Открыть"))
    {
        if(serialPort->openPort(ui->comboBoxCOMPort->currentText())) // открыть порт. если он открылся нормально
        {
            // порт открыт
            ui->statusBar->showMessage(tr("Порт %1 открыт").arg(serialPort->serial->portName()));
            ui->btnCOMPortOpenClose->setText(tr("Закрыть")); // в этом состоянии напишем такие буквы на кнопке
            ui->comboBoxCOMPort->setEnabled(false); // и запретим выбор ком-порта
            bFirstPing = true; // первый удачный пинг после открытия порта
            timerPing->start(delay_timerPing); // начнём пинговать
            baSendArray.clear(); baSendCommand.clear(); // очистить буфера
            baRecvArray.clear();
            bPortOpen = true;
            ui->groupBoxDiagnosticDevice->setEnabled(true); // разрешить комбобокс выбора типа батареи и проверки её подключения
            ui->btnCOMPortRefresh->setDisabled(true); /// отключаем кнопку обновления
        }
        else // если порт не открылся
        {
            QMessageBox::critical(this, tr("Ошибка"), serialPort->serial->errorString()); // показать текст ошибки
            bPortOpen = false;
        }
    }
    else //if(serialPort && ui->btnCOMPortOpenClose->text()=="Закрыть")  // если есть объект последовательного порта,
    {
        if(serialPort) serialPort->closePort(); // то закрыть его
        ui->btnCOMPortOpenClose->setText(tr("Открыть")); // в этом состоянии напишем такие буквы на кнопке
        ui->comboBoxCOMPort->setEnabled(true); // и разрешим комбобокс выбора порта
        ui->statusBar->showMessage(tr("Порт закрыт"));
        timerPing->stop();// остановить пинг
        ui->groupBoxDiagnosticDevice->setDisabled(true); // закрыть группу выбора батареи
        ui->groupBoxDiagnosticMode->setDisabled(true); // запретить бокс выбора режима диагностики
        ui->groupBoxCheckParams->setDisabled(true); // запретить бокс выбора параметра проверки ручного режима
        ui->groupBoxCheckParamsAutoMode->setDisabled(true); // запретить бокс выбора начальных параметров проверки автоматического режима
        ui->btnCOMPortRefresh->setEnabled(true); /// включаем кнопку обновления
        ui->btnVoltageOnTheHousing->setDisabled(true); /// отключаем кнопки проверок
        ui->btnInsulationResistance->setDisabled(true); /// отключаем кнопки проверок
        ui->btnOpenCircuitVoltageGroup->setDisabled(true); /// отключаем кнопки проверок
        ui->btnOpenCircuitVoltageBattery->setDisabled(true); /// отключаем кнопки проверок
        ui->btnClosedCircuitVoltageGroup->setDisabled(true); /// отключаем кнопки проверок
        ui->btnDepassivation->setDisabled(true); /// отключаем кнопки проверок
        ui->btnClosedCircuitVoltageBattery->setDisabled(true); /// отключаем кнопки проверок
        ui->btnInsulationResistanceUUTBB->setDisabled(true); /// отключаем кнопки проверок
        ui->btnOpenCircuitVoltagePowerSupply->setDisabled(true); /// отключаем кнопки проверок
        ui->btnClosedCircuitVoltagePowerSupply->setDisabled(true); /// отключаем кнопки проверок
        bPortOpen = false;
        loop.exit(-1); // закончить цикл ожидания ответа
    }
}

/*!
 * COM Порт получения списка портов
 */
void MainWindow::getCOMPorts()
{
    ui->comboBoxCOMPort->clear();
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        QStringList list;
        list << info.portName()+" - "+info.manufacturer();
        ui->comboBoxCOMPort->addItems(list);
    }
}

/*!
 * COM Порт - кнопка "обновления" списка портов.
 */
void MainWindow::on_btnCOMPortRefresh_clicked()
{
    getCOMPorts();
}
