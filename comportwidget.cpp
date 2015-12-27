#include <QtCore/QStringList>
#include <QtCore/QTimer>
#include <QtCore/QDebug>
#include <QMessageBox>

#include "comportwidget.h"
#include "ui_comportwidget.h"

ComPortWidget::ComPortWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ComPortWidget),
    timer(0)
{
    ui->setupUi(this);

    fillPortsInfo();

    updateSettings();
    initButtonConnections();
    initComPortWidgetCloseState();
    serial = new QSerialPort(this);

    connect(serial, SIGNAL(readyRead()), this, SLOT(procSerialDataReceive()));
    connect(serial, SIGNAL(error(QSerialPort::SerialPortError)), this,
            SLOT(handleError(QSerialPort::SerialPortError)));
}

ComPortWidget::~ComPortWidget()
{
    delete ui;
}

void ComPortWidget::procSerialDataReceive()
{
    if (serial && serial->isOpen())
    {
        initTimer();

        if (rxBuffer.isEmpty())
            timer->start();
        QByteArray data = serial->readAll();
        //qDebug() << " comportwidget.cpp procSerialDataReceive(): read port " << data;

        rxBuffer.append(data);
    }
}
// write data to serial port
void ComPortWidget::procSerialDataTransfer(const QByteArray &data)
{
    if (serial && serial->isOpen())
    {
        serial->write(data);
        //qDebug() << " comportwidget.cpp procSerialDataTransfer(): write port " << data.toHex();
    }
}

// open/close serial port button
void ComPortWidget::procControlButtonClick()
{
    updateSettings();

    if (serial)
    {
        bool result = serial->isOpen();
        if (result)
        {
            serial->close();
//            ui->statusBar->showMessage(tr("Disconnected"));
            result = false;
        }
        else
        {
            serial->setPortName(currentSettings.name);
            serial->setBaudRate(currentSettings.baudRate);
            serial->setDataBits(currentSettings.dataBits);
            serial->setParity(currentSettings.parity);
            serial->setStopBits(currentSettings.stopBits);
            serial->setFlowControl(currentSettings.flowControl);
            result = serial->open(QIODevice::ReadWrite);
            if (result)
            {
//                    ui->statusBar->showMessage(tr("Connected to %1 : %2, %3, %4, %5, %6")
//                                               .arg(currentSettings.name).arg(currentSettings.stringBaudRate).arg(currentSettings.stringDataBits)
//                                               .arg(currentSettings.stringParity).arg(currentSettings.stringStopBits).arg(currentSettings.stringFlowControl));
            } else
            {
                QMessageBox::critical(this, tr("Ошибка"), serial->errorString());

//                ui->statusBar->showMessage(tr("Open error"));
            }
        }

        (result) ? this->initComPortWidgetOpenState() : this->initComPortWidgetCloseState();
    }
}

// по тайм-ауту передадим принятый буфер дальше куда-нибудь
void ComPortWidget::procTimerOut(){
    timer->stop();
    /////////////this->traceWidget->printTrace(this->rxBuffer, true);
    //qDebug() << "comportwidget.cpp procTimerOut(): sendSerialReceivedData " << this->rxBuffer.toHex();
    emit sendSerialReceivedData(this->rxBuffer);
    this->rxBuffer.clear();
}

/* Private methods section */
// изменить состояние комбо-бокса и название кнопочки, если порт закрыт
void ComPortWidget::initComPortWidgetCloseState()
{
    ui->portBox->setEnabled(true);
    ui->controlButton->setText(QString(tr("Открыть")));

}
// изменить состояние комбо-бокса и название кнопочки, если порт открыт
void ComPortWidget::initComPortWidgetOpenState()
{
    ui->portBox->setEnabled(false);
    ui->controlButton->setText(QString(tr("Закрыть")));

}
//таймер приёма. по тайм-ауту выдаст сигнал и выполнит слот
void ComPortWidget::initTimer()
{
    if (timer) // пусть таймер проинициализируется только раз.  если уже, то выход.  а чо б его тогда сразу не???
        return;

    timer = new QTimer(this);
    //this->timer->setInterval(50);
    timer->setInterval(250);
    connect(timer, SIGNAL(timeout()), this, SLOT(procTimerOut()));
}

void ComPortWidget::deinitTimer()
{
    // тут что-то надо написать
}

// соединить сигналы от кнопочек со слотами
void ComPortWidget::initButtonConnections()
{
    connect(ui->controlButton, SIGNAL(clicked()), this, SLOT(procControlButtonClick()));
    connect(ui->closeWidgetButton, SIGNAL(clicked()), this, SLOT(close()));

}

//заполнить список комбо-бокса доступными портами
void ComPortWidget::fillPortsInfo()
{
    ui->portBox->clear();
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        QStringList list;
        list << info.portName();
        ui->portBox->addItems(list);
    }
}
// задать параметры порта
void ComPortWidget::updateSettings()
{
    currentSettings.name = ui->portBox->currentText();
    currentSettings.baudRate = QSerialPort::Baud115200;
    currentSettings.stringBaudRate = QString::number(currentSettings.baudRate);
    currentSettings.dataBits = QSerialPort::Data8;
    currentSettings.parity = QSerialPort::NoParity;
    currentSettings.stopBits = QSerialPort::OneStop;
    currentSettings.flowControl = QSerialPort::NoFlowControl;

}
//выдать окошко с ошибкой
void ComPortWidget::handleError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::ResourceError) {
        QMessageBox::critical(this, tr("Критическая ошибка"), serial->errorString());
        //closeSerialPort();
        serial->close();
    }
}

//=============  это алгоритм, если принимать из порта побайтно.  тут оказался не нужен, поэтому в подвал
#if 0

// моя вставочка
        QByteArray ba; //received data
        quint8 prfx1=0, prfx2=0, operation_code=0, length=0, nmc=0x00,//номер устройства - 07 для БИП, 09 для Имитатора батареи
                crc = 0;
        int i=0;
        int rrto = 100; //timeout for ready read
begin:
        ba.clear(); // очистим приёмный буффер
            prfx1 = 0;
            prfx2 = 0;
            operation_code = 0;
            length = 0;
            crc = 0;
        // приём префикса
        // читаем по байту, ждём префикса
        while(1)
        {
            // если есть данные в порту, и не случился таймаут
            // вызов waitForReadyRead() внутри слота, связанного с сигналом readyRead(), является причиной прекращения выдачи сигналов новых данных
//            if ((this->serial->bytesAvailable() > 0) ||  this->serial->waitForReadyRead(rrto))
            if (this->serial->bytesAvailable() > 0)
            {
                // читаем один байт
                ba += this->serial->read(1);
                prfx1 = ba[ba.size()-1];
                //qDebug() << "Read prefix byte is : " << ba.size() << " bytes " << ba.toHex();
                qDebug() << "Read prefix byte is : " << prfx1;
                // если текущий байт равен префиксу
                if(prfx1 == 0xAA)
                {
                    // и предыдущий байт тоже равен префиксу, то вываливаемся из цикла приёма префикса
                    if(prfx2 == 0xAA) break;
                }
                prfx2 = prfx1;
            }
            // если таймаут, то начинаем сначала
            else
            {
                qDebug() << "Timeout read prefix in time : " << QTime::currentTime();
                return;
                goto begin;
                continue;
            }
        }
        // приняли префикс, теперь принимаем код операции
//        if ((this->serial->bytesAvailable() > 0) ||  this->serial->waitForReadyRead(rrto))
        if (this->serial->bytesAvailable() > 0)
        {
            // читаем один байт.   начнём заполнять буфер сначала (чтобы потом посчитать crc), поэтому не +=,а просто =
            ba = this->serial->read(1);
            operation_code = ba[ba.size()-1];
            //crc += operation_code;
            qDebug() << "Read operation code byte is : " << operation_code;
        }
        // если таймаут, то начинаем сначала
        else
        {
            qDebug() << "Timeout read operation code in time : " << QTime::currentTime();
            return;
            //goto begin;
            //continue;
        }
        // приняли код операции, теперь принимаем длину посылки - кол-во оставшихся байт, включая crc
 //       if ((this->serial->bytesAvailable() > 0) ||  this->serial->waitForReadyRead(rrto))
        if (this->serial->bytesAvailable() > 0)
        {
            // читаем один байт.
            ba += this->serial->read(1);
            length = ba[ba.size()-1];
            //crc += length;
            qDebug() << "Read lenght byte is : " << length;
        }
        // если таймаут, то начинаем сначала
        else
        {
            qDebug() << "Timeout read lenght in time : " << QTime::currentTime();
            return;
            //goto begin;
            //continue;
        }
        //
        qDebug() << "ba size " << ba.size() << " :" << ba.toHex();
        // приняли длину оставшейся посылки, теперь принимаем саму посылку
        while (1)
        {
//            if ((this->serial->bytesAvailable() > 0) ||  this->serial->waitForReadyRead(rrto))
            if (this->serial->bytesAvailable() > 0)
            {
                // читаем length байт
                ba += this->serial->read(length);
                qDebug() << "Read body bytes is : " << " lenght " << length << " read size " << ba.size() << " buffer: " << ba.toHex();
                // проверить соответствие запрашиваемой длины и принятой длины. если не равно, то сначала.
                if((ba.size()-2) < length)
                {
                    length -= ba.size()-2;
                    continue;
                }
                if((ba.size()-2) == ba[1]) // если вся длина, то выходим
                {
                    qDebug() << "Read body bytes is : " << ba.toHex();
                    break;
                }

            }
            else
            {
                qDebug() << "Timeout read left body in time : " << QTime::currentTime();
                return;

            }
        }


/*        if ((this->serial->bytesAvailable() > 0) ||  this->serial->waitForReadyRead(rrto))
        {
            // читаем length байт
            ba += this->serial->read(length);
            qDebug() << "Read body bytes is : " << " lenght " << length << " read size " << ba.size() << " buffer: " << ba.toHex();
            // проверить соответствие запрашиваемой длины и принятой длины. если не равно, то сначала.
            if((ba.size()-2) != length)
            {
                qDebug() << "Wrong lenght! break.";
                return;
                //goto begin;
                //continue;
            }
            //qDebug() << "Read body bytes is : " << ba.toHex();
        }
        // если таймаут, то начинаем сначала
        else
        {
            qDebug() << "Timeout read left body in time : " << QTime::currentTime();
            return;
            //goto begin;
            //continue;
        }*/
        // пробежаться по буферу, посчитать crc
        for(i=0; i<ba.size()-1; i++)
        {
            crc += ba[i];
        }
        if(crc != (quint8)ba[ba.size()-1])
        {
            qDebug() << "Wrong CRC! Received crc ba.[" << ba.size()-1 << "]=" << QString::number((quint8)ba[ba.size()-1],16) << " but count crc is 0x" << QString::number(crc,16);
            return;
            //goto begin;
            //continue;
        }
        else qDebug() << "Good CRC = " << crc;
        nmc = ba[ba.size()-2]; // номер устройства
        // если пакет принят нормально, начинаем его разбирать
// конец моей вставочки

        // испустим сигнал с полученными данными
//        emit this->sendSerialReceivedData(data);
        emit this->sendSerialReceivedData(ba);
        ba.clear(); // зачем ???
qDebug() << "end of function: " << ba.toHex();
qDebug() << "left bytes" << this->serial->bytesAvailable();
// а printTrace выполнится по окончанию таймера
        // this->traceWidget->printTrace(data, true);

//        this->rxBuffer.append(this->serial->readAll());
//        this->rxBuffer.append(data);
 //       this->rxBuffer.append(ba);
#endif
