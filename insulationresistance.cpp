#include <QDebug>
#include <QMessageBox>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "battery.h"

extern QVector<Battery> battery;

// Нажата кнопка проверки сопротивления изоляции
void MainWindow::on_btnInsulationResistance_clicked()
{
    QString str_num; // номер цепи
    quint16 u=0; // полученный код АЦП
    float resist=0; // получившееся сопротивление
    int j=0;
    int ret=0; // код возврата ошибки
    QString strResist; // строка с получившимся значением

    if(loop.isRunning()){qDebug()<<"loop.isRunning()!"; return;} // костыль: если цикл уже работает - выйти обратно
    ui->btnInsulationResistance->setEnabled(false); // на время проверки запретить кнопку
    timerPing->stop(); // остановить пинг

    baSendArray.clear();
    baSendCommand.clear();
    baRecvArray.clear();

    ui->statusBar->showMessage(tr("Проверка сопротивления изоляции ..."));
//    ui->progressBar->setValue(ui->progressBar->value()+1);
    Log(tr("Проверка сопротивления изоляции"), "blue");

    // сбросить коробочку
    baSendArray = (baSendCommand="IDLE")+"#"; // подготовить буфер для передачи
    sendSerialData(); // послать baSendArray в порт
    // ждём ответа. по сигналу о готовности принятых данных или по таймауту, вывалимся из цикла
    ret=loop.exec();
    if(ret) goto stop; // если не ноль (ошибка таймаута) - вывалиться из режима. если 0, то приняли данные из порта

    str_num.sprintf(" %02i", battery[iBatteryIndex].isolation_resistance_nn[ui->cbInsulationResistance->currentIndex()]); // напечатать номер точки измерения изоляции
    baSendArray=(baSendCommand="Rins")+str_num.toLocal8Bit()+"#";
    if(bDeveloperState) Log(QString("Sending ") + qPrintable(baSendArray), "blue");
    QTimer::singleShot(delay_after_IDLE_before_other, this, SLOT(sendSerialData()));
    ret=loop.exec();
    if(ret) goto stop;

    baSendArray=baSendCommand+"?#";
    QTimer::singleShot(delay_command_after_start_before_request, this, SLOT(sendSerialData()));
    ret=loop.exec();
    if(ret) goto stop; // если ошибка - вывалиться из режима
    u = getRecvData(baRecvArray); // получить данные опроса

    //qDebug()<<"u"<<qPrintable(QString::number(u, 16));

    // будем щщитать сразу в коде, без перехода в вольты
    // пробежимся по точкам ф-ии, и высчитаем сопротивление согласно напряжению
    for(j=0; j<settings.functionResist.size()-2; j++)
    {
        //qDebug()<<"u"<<qPrintable(QString::number(u, 16))<<qPrintable(QString::number(settings.functionResist[j].codeADC, 16))<<qPrintable(QString::number(settings.functionResist[j+1].codeADC, 16));
        if((u > settings.functionResist[j].codeADC) && (u <= settings.functionResist[j+1].codeADC))
        {
            resist = ( -(settings.functionResist[j].codeADC * settings.functionResist[j+1].resist - settings.functionResist[j+1].codeADC * settings.functionResist[j].resist) - (settings.functionResist[j].resist - settings.functionResist[j+1].resist) * u) / (settings.functionResist[j+1].codeADC - settings.functionResist[j].codeADC);
            break;
        }
    }
    // если напряжение меньше меньшего, то будем щщитать как первом отрезке ф-ии
    if(u <= settings.functionResist[0].codeADC)
    {
        j=0;
        resist = ( -(settings.functionResist[j].codeADC * settings.functionResist[j+1].resist - settings.functionResist[j+1].codeADC * settings.functionResist[j].resist) - (settings.functionResist[j].resist - settings.functionResist[j+1].resist) * u) / (settings.functionResist[j+1].codeADC - settings.functionResist[j].codeADC);
    }
    // если напряжение больше большего, то будем щщитать как на последнем отрезке ф-ии
    if(u>settings.functionResist[settings.functionResist.size()-1].codeADC)
    {
        j=settings.functionResist.size()-2;
        resist = ( -(settings.functionResist[j].codeADC * settings.functionResist[j+1].resist - settings.functionResist[j+1].codeADC * settings.functionResist[j].resist) - (settings.functionResist[j].resist - settings.functionResist[j+1].resist) * u) / (settings.functionResist[j+1].codeADC - settings.functionResist[j].codeADC);
    }
    // если меньше нуля, то обнулим
    if(resist<0) resist=0; // не бывает отрицательного сопротивления
    if(resist > 1000000)
    {
        // переведём в мегаомы
        resist = resist/1000000;
        strResist = QString::number(resist, 'f', 2) + "МОм, ";
    }
    else if(resist > 1000)
    {
        // переведём в килоомы
        resist = resist/1000;
        strResist = QString::number(resist, 'f', 2) + "кОм, ";
    }
    else
    {
        // омы
        resist = resist/1000;
        strResist = QString::number(resist, 'f', 2) + "Ом, ";
    }


    qDebug()<<" u=0x"<<qPrintable(QString::number(u, 16))<<" resist="<<resist;

    // сбросить коробочку
    baSendArray = (baSendCommand="IDLE")+"#";
    QTimer::singleShot(delay_command_after_request_before_next, this, SLOT(sendSerialData()));
    ret=loop.exec();
    if(ret) goto stop; // если ошибка - вывалиться из режима

    // если отладочный режим, напечатать отладочную инфу
    if(bDeveloperState)
    {
        Log("Сопротивление изоляции: " + ui->cbInsulationResistance->currentText() + "=" + strResist + "код АЦП= 0x" + QString("%1").arg((ushort)u, 0, 16), "green");
    }
stop:
    // если отладочный режим, напечатать отладочную инфу
    if(bDeveloperState)
    {
        if(ret==1) Log(tr("Timeout!"), "red");
        else if(ret==2) Log(tr("Incorrect reply!"), "red");
    }
    ui->btnInsulationResistance->setEnabled(true); // разрешить кнопку
    timerPing->start(delay_timerPing); // запустить пинг по выходу из режима
    baSendArray.clear(); // очистить буфера команд.
    baSendCommand.clear();
    baRecvArray.clear();
// !!! сбросить прогрессбар
}


#if 0
u=code; // будем щщитать сразу в коде, без перехода в вольты
 // пробежимся по точкам ф-ии, и высчитаем сопротивление согласно напряжению
 for(j=0; j<points-1; j++)
 {
     if((u>uizol[j]) && (u<=uizol[j+1]))
     {
         resist=(-(uizol[j]*r[j+1] - uizol[j+1]*r[j]) - (r[j]-r[j+1])*u)/(uizol[j+1]-uizol[j]);
     }
 }
 // если напряжение меньше меньшего, то будем щщитать как первом отрезке ф-ии
 if(u<=uizol[0])
 {
     j=0;
     resist=(-(uizol[j]*r[j+1] - uizol[j+1]*r[j]) - (r[j]-r[j+1])*u)/(uizol[j+1]-uizol[j]);
 }
 // если напряжение больше большего, то будем щщитать как на последнем отрезке ф-ии
 if(u>uizol[11])
 {
     j=10;
     resist=(-(uizol[j]*r[j+1] - uizol[j+1]*r[j]) - (r[j]-r[j+1])*u)/(uizol[j+1]-uizol[j]);
 }
 // если меньше нуля, то обнулим
 if(resist<0) resist=0; // не бывает отрицательного сопротивления
 // переведём в мегаомы
 resist = resist/1000000;

 qDebug()<<"code= "<<code<<" u= "<<u<<" resist= "<<resist;
#endif
