#include <QDebug>
#include <QMessageBox>
#include <QDateTime>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "battery.h"
#include "settings.h"

extern Settings settings;

extern QVector<Battery> battery;

// Нажата кнопка распассивации
void MainWindow::on_btnDepassivation_clicked()
{
    //checkDepassivation(); return;
    quint16 codeADC=0; // принятый код АЦП
    float fU=0; // принятое напряжение в вольтах
    // код порогового напряжения = пороговое напряжение В / коэфф. (вес разряда) + смещение (в коде)
    //quint16 codeLimit=settings.closecircuitgroup_limit/settings.coefADC1 + settings.offsetADC1; // код, пороговое напряжение.
    int ret=0; // код возврата ошибки
    QString str_num; // номер цепи
    QDateTime starttime; // время начала измерения
    QDateTime dt; // текущее время очередного измерения
    double x; // текущая координата Х
    int cycleTimeSec=0; // длительность цикла проверки в секундах
    bool firstMeasurement=true; // первое измерение
    int i=0; // номер цепи

    baSendArray.clear();
    baSendCommand.clear();
    baRecvArray.clear();

    /// при наличии галки имитатора, выводим сообщение о необходимости включить источник питания
    if(ui->cbIsImitator->isChecked() and iPowerState != 1) {
        QMessageBox::information(this, tr("Внимание! - %0").arg(ui->rbDepassivation->text()), tr("Перед проверкой необходимо включить источник питания!"));
        iPowerState = 1; /// состояние включенного источника питания
    }

    /// таблица - верх
    sHtml = tr("<table border=\"1\" width=\"100%\" cellpadding=\"3\" cellspacing=\"0\" bordercolor=\"black\">"\
               "    <tbody>"\
               "        <tr>"\
               "            <td colspan=\"3\">&nbsp;<strong>%0&nbsp;</strong></td>"\
               "        </tr>"\
               "        <tr>"\
               "            <td width=\"11%\">"\
               "                <p>&nbsp;<b>Время</b>&nbsp;</p>"\
               "            </td>"\
               "            <td width=\"54%\">"\
               "                <p>&nbsp;<b>Цепь</b>&nbsp;</p>"\
               "            </td>"\
               "            <td width=\"35%\">"\
               "                <p>&nbsp;<b>Результат</b>&nbsp;</p>"\
               "            </td>"\
               "        </tr>")
            .arg(ui->rbDepassivation->text());

    // написать про группы, в зависимости от признаков и флагов
    for(i = 0; i < 28; i++)
    {
        dArrayDepassivation[i] = -1;
        label = findChild<QLabel*>(tr("labelDepassivation%0").arg(i));
        label->setStyleSheet("QLabel { color : black; }");
        label->clear();
        if (i < battery[iBatteryIndex].group_num) {
            QStandardItem *sitm = modelDepassivation->item(i+1, 0);
            Qt::CheckState checkState = sitm->checkState();

            label = findChild<QLabel*>(tr("labelDepassivation%0").arg(i));
            if(!(battery[iBatteryIndex].b_flag_circuit[i] & CIRCUIT_OCG_TESTED))
            {
                label->setText(tr("%0) НРЦг не проверялось.").arg(i+1));
            }
            else if(battery[iBatteryIndex].b_flag_circuit[i] & CIRCUIT_FAULT)
            {
                label->setText(tr("%0) НРЦг < нормы, проверка под нагрузкой запрещена.").arg(i+1));
                label->setStyleSheet("QLabel { color : red; }");
            }
            else if(!(battery[iBatteryIndex].b_flag_circuit[i] & CIRCUIT_DEPASS) || (checkState != Qt::Checked))
            {
                label->setText(tr("%0) %1 не требуется.").arg(i+1).arg(battery[iBatteryIndex].circuitgroup[i]));
                label->setStyleSheet("QLabel { color : black; }");
            }
            else
            {
                label->setText(tr("%0) %1 требуется!").arg(i+1).arg(battery[iBatteryIndex].circuitgroup[i]));
                label->setStyleSheet("QLabel { color : blue; }");
            }
        }
    }
    // !!! лишние label вообще стереть.
    // Подготовка графика
    /*ui->widgetDepassivation->addGraph(); // blue line
    ui->widgetDepassivation->graph(0)->setPen(QPen(Qt::blue));
    ui->widgetDepassivation->graph(0)->clearData();
    ui->widgetDepassivation->addGraph(); // blue dot
    ui->widgetDepassivation->graph(1)->clearData();
    ui->widgetDepassivation->graph(1)->setLineStyle(QCPGraph::lsNone);
    //ui->widgetDepassivation->graph(1)->setPen(QPen(Qt::green));
    ui->widgetDepassivation->graph(1)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, Qt::blue, Qt::white, 7));*/
    /*ui->widgetDepassivation->addGraph(); // red line
    ui->widgetDepassivation->graph(2)->setPen(QPen(Qt::red));
    ui->widgetDepassivation->graph(2)->setBrush(QBrush(QColor(255, 0, 0, 20)));
    ui->widgetDepassivation->graph(2)->clearData();
    ui->widgetDepassivation->graph(2)->addData(0, settings.closecircuitgroup_limit);
    ui->widgetDepassivation->graph(2)->addData(cycleTimeSec+1, settings.closecircuitgroup_limit);*/

    /*ui->widgetDepassivation->xAxis->setLabel(tr("Время, c"));
    // ниже в цикле ui->widgetDepassivation->xAxis->setRange(0, cycleTimeSec+1);
    ui->widgetDepassivation->yAxis->setLabel(tr("Напряжение, В"));
    ui->widgetDepassivation->yAxis->setRange(24, 33);*/

    if(bCheckInProgress) // если зашли в эту ф-ию по нажатию кнопки ("Стоп"), будучи уже в состоянии проверки, значит стоп режима
    {
        // остановить текущую проверку, выход
        bCheckInProgress = false;
        timerSend->stop(); // остановить посылку очередной команды в порт
        timeoutResponse->stop(); // остановить предыдущий таймаут (если был, конечно)
        qDebug()<<"loop.isRunning()"<<loop.isRunning();
        if(loop.isRunning())
        {
            loop.exit(KDS_STOP); // прекратить цикл ожидания посылки/ожидания ответа от коробочки
        }
        return;
    }

    if(loop.isRunning()){qDebug()<<"loop.isRunning()!"; return;} // костыль: если цикл уже работает - выйти обратно
    timerPing->stop(); // остановить пинг
    bCheckInProgress = true; // вошли в состояние проверки

    setGUI(false); ///  отключаем интерфейс

    ui->tabWidget->addTab(ui->tabDepassivation, ui->rbDepassivation->text());
    ui->tabWidget->setCurrentIndex(ui->tabWidget->count()-1);
    Log(tr("Проверка начата - %1").arg(ui->rbDepassivation->text()), "blue");
    ui->statusBar->showMessage(tr("Проверка ")+ui->rbDepassivation->text()+" ...");

    iCurrentStep=0; // в ручном начнём сначала
    iMaxSteps=modelClosedCircuitVoltageGroup->rowCount()-1; // -1 с учётом первой строки в комбобоксе

    // сбросить коробочку
    baSendArray = (baSendCommand="IDLE")+"#"; // подготовить буфер для передачи
    sendSerialData(); // послать baSendArray в порт
    // ждём ответа. по сигналу о готовности принятых данных или по таймауту, вывалимся из цикла
    ret=loop.exec();
    if(ret) goto stop; // если не ноль (ошибка таймаута) - вывалиться из режима. если 0, то приняли данные из порта

    // !!! если нечего проверять, список пуст - проверить алгоритм
    // Пробежимся по списку цепей
    for(int i=1; i < modelDepassivation->rowCount(); i++)
    {
        if(battery[iBatteryIndex].b_flag_circuit[i-1] == 5) {
            /// заполняем массив проверок для отчета
            dateTime = QDateTime::currentDateTime();
            sHtml += tr("<tr>"\
                        "    <td><p>&nbsp;%0&nbsp;</p></td>"\
                        "    <td><p>&nbsp;%1&nbsp;</p></td>"\
                        "    <td>&nbsp;НРЦг &lt; нормы, р-ция запр-на!&nbsp;</td>"\
                        "</tr>")
                        .arg(dateTime.toString("hh:mm:ss"))
                        .arg(battery[iBatteryIndex].circuitgroup[i-1]);
        }

        QStandardItem *sitm = modelDepassivation->item(i, 0);
        Qt::CheckState checkState = sitm->checkState();
        if (checkState != Qt::Checked) continue;
        label = findChild<QLabel*>(tr("labelDepassivation%0").arg(i-1));

        // три ступени распассивации
        for(int k=0; k<3; k++)
        {
            cycleTimeSec = settings.time_depassivation[k];
            //ui->widgetDepassivation->graph(0)->setName(tr("Ток: %0 А").arg(settings.depassivation_current[k]));
            widgetDepassivationTextLabel->setText(tr(" Ток: %0 А ").arg(settings.depassivation_current[k]));
            ui->widgetDepassivation->xAxis->setRange(0, cycleTimeSec+1); // длительность цикла
            label->setText(tr("%1) Идёт распассивация током %2 А...").arg(i).arg(QString::number(settings.depassivation_current[k])));

            // собрать режим
            str_num.sprintf(" %02i %1i", i, k+1); // напечатать номер цепи и номер тока по протоколу
            baSendArray=(baSendCommand="UccG")+str_num.toLocal8Bit()+"#";
            if(bDeveloperState) Log(QString("Sending ") + qPrintable(baSendArray), "blue");
            QTimer::singleShot(settings.delay_after_IDLE_before_other, this, SLOT(sendSerialData()));
            ret=loop.exec();
            if(ret) goto stop;

            starttime = QDateTime::currentDateTime(); // время начала измерения
            dt = QDateTime::currentDateTime(); // текущее время
            ui->widgetDepassivation->graph(0)->clearData(); // очистить график

            dateTime = QDateTime::currentDateTime(); /// время начала распассивации

            // цикл измерения
            while(-dt.msecsTo(starttime) < cycleTimeSec*1000) // пока время цикла проверки не вышло, продолжим измерять
            {
                // опросить
                baSendArray=baSendCommand+"?#";
                QTimer::singleShot(settings.delay_after_request_before_next_ADC1, this, SLOT(sendSerialData()));
                ret=loop.exec();
                if(ret) goto stop;
                codeADC = getRecvData(baRecvArray); // напряжение в коде
                fU = ((codeADC-settings.offsetADC1[settings.board_counter])*settings.coefADC1[settings.board_counter]); // напряжение в вольтах

                /// только для 1А
                if (k == 2)
                    fU = fU + settings.closecircuitgroup_loss; /// добавляем к результату потери на кабеле

                // нарисуем график
                if(firstMeasurement)
                {
                    firstMeasurement = false;
                    starttime = QDateTime::currentDateTime(); // время начала измерения начнём считать после получения первого ответа (чтобы график рисовался с нуля)
                }
                dt = QDateTime::currentDateTime(); // текущее время
                x= -dt.msecsTo(starttime); // кол-во миллисекунд, прошедших с начала измерения

                ui->widgetDepassivation->graph(0)->rescaleValueAxis(true); // для автоматического перерисовывания шкалы графика, если значения за пределами экрана
                ui->widgetDepassivation->graph(0)->addData((double)x/1000, (double)fU);
                ui->widgetDepassivation->replot();
            }

            /// добавим в массив графиков полученный график
            ui->widgetDepassivation->savePng(QDir::tempPath()+"DepassivationGraph.png", 493, 526, 1.0, -1);
            img.load(QDir::tempPath()+"DepassivationGraph.png");
            imgArrayReportGraph.append(img);
            sArrayReportGraphDescription.append(tr("График. %0. Цепь: \"%1\". Время: %2.").arg(ui->rbDepassivation->text()).arg(battery[iBatteryIndex].circuitgroup[i-1]).arg(dateTime.toString("hh:mm:ss")));

            if(bDeveloperState)
                Log("Цепь "+battery[iBatteryIndex].circuitgroup[i-1]+" Receive "+qPrintable(baRecvArray)+" codeADC1=0x"+QString("%1").arg((ushort)codeADC, 0, 16), "blue");

            // по окончанию цикла снять нагрузку, разобрать режим
            baSendArray = (baSendCommand="IDLE")+"#";
            QTimer::singleShot(settings.delay_after_request_before_next_ADC1, this, SLOT(sendSerialData()));
            ret=loop.exec();
            if(ret) goto stop;
        }

        sLabelText = tr("%0) \"%1\"").arg(i).arg(battery[iBatteryIndex].circuitgroup[i-1]);

        sResult = "Выполнена";
        color = "green";
        label->setText(tr("%0 %1").arg(sLabelText).arg(sResult));
        label->setStyleSheet("QLabel { color : "+color+"; }");
        Log(tr("%0 %1").arg(sLabelText).arg(sResult), color);

        /// заполняем массив проверок для отчета
        sHtml += tr("<tr>"\
                    "    <td><p>&nbsp;%0&nbsp;</td>"\
                    "    <td><p>&nbsp;%1&nbsp;</td>"\
                    "    <td><p>&nbsp;%2&nbsp;</td>"\
                    "</tr>")
                    .arg(dateTime.toString("hh:mm:ss"))
                    .arg(battery[iBatteryIndex].circuitgroup[i-1])
                    .arg(sResult);

        /// снимаем галку с провереной
        item = new QStandardItem(QString("%0").arg(battery[iBatteryIndex].circuitgroup[i-1]));
        item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        item->setData(Qt::Unchecked, Qt::CheckStateRole);
        modelDepassivation->setItem(i, 0, item);
    }// конец цикла проверок цепей

stop:
    // сбросить коробочку
    baSendArray = (baSendCommand="IDLE")+"#";
    timerSend->start(settings.delay_after_request_before_next_ADC1);
    ret=loop.exec();

    bCheckInProgress = false; // вышли из состояния проверки

    // если отладочный режим, напечатать отладочную инфу
    if(bDeveloperState)
    {
        if(ret == KDS_TIMEOUT) Log(tr("Timeout!"), "red");
        else if(ret == KDS_INCORRECT_REPLY) Log(tr("Incorrect reply!"), "red");
        else if(ret == KDS_STOP) Log(tr("Stop checking!"), "red");
    }
    if(ret == KDS_STOP) {
        label->setText(sLabelText + " распассивация прервана!");
        label->setStyleSheet("QLabel { color : red; }");
        Log(sLabelText + " распассивация прервана!", "red");
        sHtml += tr("<tr><td>&nbsp;%0&nbsp;</td><td>&nbsp;%1&nbsp;</td><td><p>&nbsp;распассивация прервана!&nbsp;</td></tr>")
                .arg(dateTime.toString("hh:mm:ss"))
                .arg(battery[iBatteryIndex].circuitgroup[i]);
    }

    timerPing->start(delay_timerPing); // запустить пинг по выходу из режима
    baSendArray.clear(); // очистить буфера команд.
    baSendCommand.clear();
    baRecvArray.clear();

    setGUI(true); /// включаем интерфейс

    /// таблица - низ
    sHtml +="   </tbody>"\
            "</table>"\
            "<br/>";
    sArrayReport.append(sHtml); /// добавляем таблицу в массив проверок

    Log(tr("Проверка завершена - %1").arg(ui->rbDepassivation->text()), "blue");
}

// слот вызывается при изменении чекбоксов элементов списка комбобокса
void MainWindow::itemChangedDepassivation(QStandardItem* itm)
{
    itm->text(); /// чтобы небыло варнинга при компиляции на неиспользование itm
    int count = 0;
    for(int i=1; i < modelDepassivation->rowCount(); i++)
    {
        QStandardItem *sitm = modelDepassivation->item(i, 0);
        Qt::CheckState checkState = sitm->checkState();
        if (checkState == Qt::Checked)
            count++;
    }
    ui->cbDepassivation->setItemText(0, tr("Выбрано: %0 из %1").arg(count).arg(modelDepassivation->rowCount()-1));
    ui->cbDepassivation->setCurrentIndex(0);
}
