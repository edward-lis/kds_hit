#include <QDebug>
#include "kds.h"

Kds::Kds(QObject *parent) : QObject(parent)
{
    // тут начальные условия. а ещё сделать ф-ию инициализации конкретной батареи при её выборе.
    codec = QTextCodec::codecForName("Windows-1251"); // кодировка для строк информационного обмена
    nmc = 0;        //!!! времянка
}

void Kds::send_request(quint8 operation_code, QByteArray text)
{
    QByteArray request_pfx; request_pfx.resize(2); request_pfx[0]=0xFF; request_pfx[1]=0xFF; // префикс запроса
    QByteArray ba;
    //int bw=0;
    qint8 crc=0;
    //text = codec->fromUnicode(text);
        ba.clear();
        ba+=operation_code;
        ba+='0'; // зарезервируем место под длину пакета
        ba+=codec->fromUnicode(text);;
        ba+=nmc;
        ba[1]=ba.size()-1; // длина пакета
        for(int i=0; i<ba.size(); i++) crc += ba[i];
        ba+=crc;
        ba.insert(0,request_pfx); // всунули вначало префикс запросного пакета
        /////bw = port->write(ba); // послали запрос
        // qDebug() << " kds.cpp send_request(): Request is : " << ba.size() << " bytes " << ba.toHex();
        // qDebug() << " kds.cpp send_request(): text is : " << text;
        //qDebug() << " kds.cpp send_request(): hex is : " << ba.toHex() << "\n";
        emit this->sendSerialData(ba);

}

void Kds::getSerialDataReceived(QByteArray data)
{
    int i=0;
    unsigned char operation_code=0, length = 0, crc = 0;
    QByteArray body;
    QByteArray answer_pfx;

    //qDebug() << "kds.cpp getSerialDataReceived():  Rx: " << data;

//    answer_pfx.resize(2); answer_pfx[0]=0xAA; answer_pfx[1]=0xAA; // префикс ответа
//    data.insert(0,answer_pfx); // всунули вначало префикс ответного пакета (лень отлаживаться, потом убрать лишний префикс!)

    while((data[0].operator ==((quint8)0xAA)) && (data[1].operator == ((quint8)0xAA))) // если префикс нормальный
    {
        operation_code = data[2];
        if((operation_code == (unsigned char)0x08) || (operation_code == (unsigned char)0x01))// если код операции соответствующий
        {
            length = data[3];
            if(length <= data.size()-4) // если длина принятой посылки меньше или совпадает (-4 = -два байта префикса, - байт кода операции, - байт длины)
            {
//                for(int i=2; i<data.size()-1; i++)
                for(i=2; i < length+3; i++)
                    crc += data[i]; // посчитаем crc
//                if(crc == data[data.size()-1]) // если crc совпадает
                if(crc == (unsigned char)data[length+3]) // если crc совпадает
                {
                    // то пакет принят нормально.   передаём данные из него дальше в какой-нить объект
//                    body = data.mid(4,data.size()-6);
                    body = data.mid(4,length-2);
                    //qDebug() << " kds.cpp getSerialDataReceived(): Kds Rx: " << data << "\n";
                    // испустим сигнал с полученными данными (чтобы принятые здесь данные приняли другие объекты)
                    emit this->sendSerialReceivedData(operation_code, body);
                    // если в принятом более чем один информационный пакет, то продолжим
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
                qDebug() << "\n kds.cpp: error length= " << length << " data.size()-4= " << data.size()-4;
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
/*
    // приём префикса
    // читаем по байту, ждём префикса
    while(1)
    {
        // если есть данные в порту, и не случился таймаут
        if ((port->bytesAvailable() > 0) ||  port->waitForReadyRead(rrto))
        {
            // читаем один байт
            ba += port->read(1);
            prfx1 = ba[ba.size()-1];
            //qDebug() << "Read prefix byte is : " << ba.size() << " bytes " << ba.toHex();
            qDebug() << "Read prefix byte is : " << prfx1;
            // если текущий байт равен префиксу
            if(prfx1 == 0xFF)
            {
                // и предыдущий байт тоже равен префиксу, то вываливаемся из цикла приёма префикса
                if(prfx2 == 0xff) break;
            }
            prfx2 = prfx1;
        }
        // если таймаут, то начинаем сначала
        else
        {
            qDebug() << "Timeout read prefix in time : " << QTime::currentTime();
            goto begin;
            continue;
        }
    }
    // приняли префикс, теперь принимаем код операции
    if ((port->bytesAvailable() > 0) ||  port->waitForReadyRead(rrto))
    {
        // читаем один байт.   начнём заполнять буфер сначала (чтобы потом посчитать crc), поэтому не +=,а просто =
        ba = port->read(1);
        operation_code = ba[ba.size()-1];
        //crc += operation_code;
        qDebug() << "Read operation code byte is : " << operation_code;
    }
    // если таймаут, то начинаем сначала
    else
    {
        qDebug() << "Timeout read operation code in time : " << QTime::currentTime();
        goto begin;
        continue;
    }
    // приняли код операции, теперь принимаем длину посылки - кол-во оставшихся байт, включая crc
    if ((port->bytesAvailable() > 0) ||  port->waitForReadyRead(rrto))
    {
        // читаем один байт.
        ba += port->read(1);
        length = ba[ba.size()-1];
        //crc += length;
        qDebug() << "Read lenght byte is : " << length;
    }
    // если таймаут, то начинаем сначала
    else
    {
        qDebug() << "Timeout read lenght in time : " << QTime::currentTime();
        goto begin;
        continue;
    }
    //
    qDebug() << "ba size " << ba.size() << " :" << ba.toHex();
    // приняли длину оставшейся посылки, теперь принимаем саму посылку
    if ((port->bytesAvailable() > 0) ||  port->waitForReadyRead(rrto))
    {
        // читаем length байт
        ba += port->read(length);
        qDebug() << "Read body bytes is : " << " lenght " << length << " read size " << ba.size() << " buffer: " << ba.toHex();
        // проверить соответствие запрашиваемой длины и принятой длины. если не равно, то сначала.
        if((ba.size()-2) != length)
        {
            qDebug() << "Wrong lenght! break.";
            goto begin;
            continue;
        }
        //qDebug() << "Read body bytes is : " << ba.toHex();
    }
    // если таймаут, то начинаем сначала
    else
    {
        qDebug() << "Timeout read left body in time : " << QTime::currentTime();
        goto begin;
        continue;
    }
    // пробежаться по буферу, посчитать crc
    for(i=0; i<ba.size()-1; i++)
    {
        crc += ba[i];
    }
    if(crc != (quint8)ba[ba.size()-1])
    {
        qDebug() << "Wrong CRC! Received crc ba.[" << ba.size()-1 << "]=" << QString::number((quint8)ba[ba.size()-1],16) << " but count crc is 0x" << QString::number(crc,16);
        goto begin;
        continue;
    }
    else qDebug() << "Good CRC = " << crc;
    nmc = ba[ba.size()-2]; // номер устройства
    // если пакет принят нормально, начинаем его разбирать
*/
/*
    // разобрать посылку!!!
    qDebug() << "Kds Rx: " << data;
    // испустим сигнал с полученными данными (для передачи куда-нить дальше)
    emit this->sendSerialReceivedData(data);
    */
}
