#include "serialport.h"
#include <QtCore>
#include <QErrorMessage>
#include "settings.h"


SerialPort::SerialPort(QObject *parent) : QObject(parent)
{
    codec = QTextCodec::codecForName("Windows-1251"); // кодировка для строк информационного обмена
    serial = new QSerialPort(this);
    // перехватчик критических ошибок
    connect(serial, SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(handleError(QSerialPort::SerialPortError)));
    // при появлении данных в порту - прочитаем их
    connect(serial, SIGNAL(readyRead()), this, SLOT(readSerialPort()));
    // инициализация таймера для тайм-аута
    timerTimeOut = new QTimer(this);
    timerTimeOut->setInterval(50); // на скорости 115200 посылка из мах 255 байт должна продолжаться около 22 мс
    connect(timerTimeOut, SIGNAL(timeout()), this, SLOT(procTimerTimeOut()));
}

bool SerialPort::openPort(QString portName)
{
    bool result=false;
    if (serial)
    {
        serial->setPortName(portName);
        serial->setBaudRate(QSerialPort::Baud115200);
        serial->setDataBits(QSerialPort::Data8);
        serial->setParity(QSerialPort::NoParity);
        serial->setStopBits(QSerialPort::OneStop);
        serial->setFlowControl(QSerialPort::NoFlowControl);
        result = serial->open(QIODevice::ReadWrite);
        if (result)
        {
            qDebug()<<"Open serial port "<<portName;
        } else
        {
            qDebug()<<"Error open serial port "<<portName;
        }
    }
    return result;
}

void SerialPort::closePort()
{
    if(serial && (serial->isOpen()))
    {
        serial->close();
        qDebug()<<"Serial Port close";
    }
}

// перехватчик критических ошибок
void SerialPort::handleError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::ResourceError)
    {
        emit signalCriticalError(); //
        //closePort();
        /*QErrorMessage errorMessage;
        errorMessage.showMessage("Критическая ошибка порта\n"+serial->errorString());
        errorMessage.exec();*/
        //QMessageBox::critical(NULL, tr("Критическая ошибка порта!"), serial->errorString()); // а уже потом бахнем сообщение.
        // при вытаскивании из юсб-порта разъёма, почему-то приходит два соообщения. хотя порт закрывается после первого (а потом виджеты открываются)
        // надо бы исследовать поведение КА. http://doc.qt.io/qt-5/statemachine-api.html - вот тут погуглить про месседжбокс, что-то было похожее.
    }
}
// Запись массива data в порт
void SerialPort::writeSerialPort(quint8 operation_code, const QByteArray &data)
{
    QByteArray request_pfx; request_pfx.resize(2); request_pfx[0]=0xFF; request_pfx[1]=0xFF; // префикс запроса
    QByteArray ba;
    qint8 crc=0;
    if (serial && serial->isOpen())
    {
        // Сбор буфера по протоколу обмена
        ba.clear();
        ba+=operation_code;
        ba+='0'; // зарезервируем место под длину пакета
        ba+=codec->fromUnicode(data); // так как у нас текст программки в юникоде, а в протоколе должна быть кодировка 1251, то используем кодек
        ba+=0x07; // nmc=7 всегда;
        ba[1]=ba.size()-1; // длина пакета
        for(int i=0; i<ba.size(); i++) crc += ba[i]; // посчитали простенький CRC
        ba+=crc;
        ba.insert(0,request_pfx); // всунули вначало префикс запросного пакета
        serial->write(ba); // собственно передача в порт
        /* !!! сделать человеческую печать if(::Settings.verbose > 1)*/
        //qDebug() << "serialport.cpp writeSerialPort(): " << ba.length() << " bytes " << ba.toHex() << " text " << qPrintable(ba);
    }
}
// тайм-аут по чтению из порта
void SerialPort::procTimerTimeOut()
{
    timerTimeOut->stop(); // остановить периодический таймер
    //qDebug() << " serialport.cpp procTimerTimeOut(): read port " << rxBuffer.length() << " bytes " << rxBuffer.toHex() << " text " << qPrintable(rxBuffer);
    disassembleReadMsg(rxBuffer); // разобрать принятый буфер (и там внутри передать его дальше)
    rxBuffer.clear();
}

// Чтение данных из последовательного порта, по сигналу от драйвера о готовности данных для чтения.
void SerialPort::readSerialPort()
{
    if (serial && serial->isOpen())
    {
        if (rxBuffer.isEmpty())
            timerTimeOut->start(); // старт тайм-аута начала приёма

        QByteArray data = serial->readAll(); // читаем все доступные данные
        //qDebug() << " serialport.cpp readSerialPort(): read port " << data.length() << " bytes " << data.toHex() << " text " << qPrintable(data);
        rxBuffer.append(data); // и складываем в промежуточный приёмный буфер
    }
}

// Разобрать принятый пакет
void SerialPort::disassembleReadMsg(QByteArray data)
{
        // Разбор принятого буфера по протоколу обмена
int i=0;
unsigned char operation_code=0, length = 0, crc = 0;
QByteArray body;

        while((data[0].operator ==((quint8)0xAA)) && (data[1].operator == ((quint8)0xAA))) // если префикс нормальный
        {
            operation_code = data[2];
            if((operation_code == (unsigned char)0x08) || (operation_code == (unsigned char)0x01))// если код операции соответствующий
            {
                length = data[3];
                if(length <= data.size()-4) // если длина принятой посылки меньше или совпадает (-4 = -два байта префикса, - байт кода операции, - байт длины)
                {
                    for(i=2; i < length+3; i++)
                        crc += data[i]; // посчитаем crc
                    if(crc == (unsigned char)data[length+3]) // если crc совпадает
                    {
                        // то пакет принят нормально.   передаём данные из него дальше в какой-нить объект
                        body = data.mid(4,length-2);
                        //qDebug() << " serialpor.cpp readSerialPort(): Rx: " << data << "\n";
                        // испустим сигнал с полученными данными (чтобы принятые здесь данные приняли другие объекты)
                        emit readySerialData(operation_code, body);
                        // если в принятом буфере более чем один информационный пакет, то продолжим
                        //qDebug() << "data.size = " << data.size() << " length= " << length;
                        data.remove(0, length+4);
                        //qDebug()<<"after remove  data.size()= " << data.size();
                        if(data.size()>0)
                        {
                            body.clear();
                            crc=0;
                            //qDebug()<<"new data= " << data.toHex();
                            continue;
                        }
                        else break;
                    }
                    else
                    {
                        qDebug() << "\nError CRC= " << crc << " data[length+3]= " << data[length+3] << " length+3= " << length+3;
                        break;
                    }
                }
                else
                {
                    qDebug() << "\n serialport.cpp: error length= " << length << " data.size()-4= " << data.size()-4;
                    break;
                }
            }
            else
            {
                qDebug() << "\nError operation_code = data[2] = " << operation_code;
                break;
            }
            //qDebug()<<"1new data= " << data.toHex();
        }
}

#if 0 // чтение побайтно, надо делать в отдельном потоке, а не в слоте по сигналу readyRead
// потому что когда попадаем в слотовую ф-ию по сигналу readyRead, то waitForReadyRead() не отрабатывает таймаут.
// и read() больше ничего не читает.  см. документацию на сигнал.

int rrto=5000; // ms timeout
QByteArray ba; //received data
quint8 prfx1=0, prfx2=0, operation_code=0, length=0, nmc=0x00,//номер устройства - 07 для БИП, 09 для Имитатора батареи
        crc = 0;

// читаем по байту, ждём префикса
while(1)
{
    qDebug()<<"serial->bytesAvailable() "<<serial->bytesAvailable();
    // если есть данные в порту, и не случился таймаут
    if ((serial->bytesAvailable() > 0) ||  serial->waitForReadyRead(rrto))
    {
        qDebug()<<"reading...";
        // читаем один байт
        ba += serial->read(1);
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
    }
}
qDebug()<<"2 serial->bytesAvailable() "<<serial->bytesAvailable();
// приняли префикс, теперь принимаем код операции
if ((serial->bytesAvailable() > 0) ||  serial->waitForReadyRead(rrto))
{
    qDebug()<<"2 reading...";
    // читаем один байт.   начнём заполнять буфер сначала (чтобы потом посчитать crc), поэтому не +=,а просто =
    ba = serial->read(1);
    operation_code = ba[ba.size()-1];
    //crc += operation_code;
    qDebug() << "Read operation code byte is : " << operation_code;
}
// если таймаут, то начинаем сначала
else
{
    qDebug() << "Timeout read operation code in time : " << QTime::currentTime();
    return;
}
qDebug()<<"3 serial->bytesAvailable() "<<serial->bytesAvailable();
// приняли код операции, теперь принимаем длину посылки - кол-во оставшихся байт, включая crc
if ((serial->bytesAvailable() > 0) ||  serial->waitForReadyRead(rrto))
{
    qDebug()<<"3 reading...";
    // читаем один байт.
    ba += serial->read(1);
    length = ba[ba.size()-1];
    //crc += length;
    qDebug() << "Read lenght byte is : " << length;
}
// если таймаут, то начинаем сначала
else
{
    qDebug() << "Timeout read lenght in time : " << QTime::currentTime();
    return;
}
qDebug()<<"4 serial->bytesAvailable() "<<serial->bytesAvailable();
// приняли длину оставшейся посылки, теперь принимаем саму посылку
if ((serial->bytesAvailable() > 0) ||  serial->waitForReadyRead(rrto))
{
    qDebug()<<"4 reading...";
    // читаем length байт
    int rest=length;
    do
    {
        qDebug()<<"rest " <<rest;
    ba += serial->read(rest);
    qDebug() << "Read body bytes is : " << " lenght " << length << " read size " << ba.size() << " buffer: " << ba.toHex();
    qDebug() << "buf: " << ba;
    /* // проверить соответствие запрашиваемой длины и принятой длины. если не равно, то сначала.
    if((ba.size()-2) != length)
    {
        qDebug() << "Wrong lenght! break.";
        return;
    }*/
    rest=length - ba.size()-2;
    qDebug()<<rest;
    }while(rest);
    //qDebug() << "Read body bytes is : " << ba.toHex();
}
// если таймаут, то начинаем сначала
else
{
    qDebug() << "Timeout read left body in time : " << QTime::currentTime();
    return;
}
// пробежаться по буферу, посчитать crc
for(i=0; i<ba.size()-1; i++)
{
    crc += ba[i];
}
if(crc != (quint8)ba[ba.size()-1])
{
    qDebug() << "Wrong CRC! Received crc ba.[" << ba.size()-1 << "]=" << QString::number((quint8)ba[ba.size()-1],16) << " but count crc is 0x" << QString::number(crc,16);
    return;
}
else qDebug() << "Good CRC = " << crc;
nmc = ba[ba.size()-2]; // номер устройства
// если пакет принят нормально
qDebug() << " serialport.cpp writeSerialPort(): " << ba.length() << " bytes " << ba.toHex() << " text " << qPrintable(ba);
/*        // то передаём данные из него дальше в какой-нить объект
body = data.mid(4,length-2);
//qDebug() << " serialpor.cpp readSerialPort(): Rx: " << data << "\n";
// испустим сигнал с полученными данными (чтобы принятые здесь данные приняли другие объекты)
emit readySerialData(operation_code, body);
*/
#endif
