/* История изменений:
 * 03.02.2016 -
 * Добавил класс для работы с последовательным портом.
 * Добавил конечный автомат (КА).
 * Логика по открытию последовательного порта.
 * Добавлен Режим проверки соответствия типа выбранной оператором батареи и типа реально подключенной батареи.
 */

/* Замечания и план работы:
 * 1. Сделать вёрстку tabWidget в лэйаут.  Чтобы при ресайзе окна tabWidget растягивался и вправо, и вниз.
2. При выборе в комбобоксе типа батареи запрещать галку УУТББ для тех батарей, у которых УУТББ бы не может в принципе.
А именно - 9ER14P-24 и 9ER20P-28. А для 9ER20P-20 и 9ЕR14PS-24 -  разрешать галку.

Добавить прогресс-бар для режима проверки типа подключенной батареи.
Добавить логику соответствия выбранного на экране порта - реально подключенному. (в одном из случаев есть неопределённость)
Сейчас программка просто пишет в статус-бар.

Где-то на виду сделать большое табло с буквами, в котором писать текущее состояние: типа идёт такая-то проверка.
и туда же перетащить прогресс-бар.  Потому что в статус-баре нихрена не видно, там пусть останется инфа о состоянии связи,
ну и дублируется инфа из табло.

3. Переделать весь исходник под КА. Выкинуть лишнее (предыдущую реализацию последовательного порта, и логику кнопок)
Добавить на каждый режим проверки батареи свой отдельный файл с КА. Чтобы там отлаживаться отдельно. Там же и вся логика.
В качестве рыбы - режим открытия порта и режим проверки типа подключенной батареи при нажатии на кнопку "Проверить".
Начать с какого-нибудь короткого режима, типа проверка изоляции.
delay/sleep - не нужен в принципе.

Дальнейший план:
4. Вставить объекты класса батарей, в которых будут находиться все параметры проверяемых батарей - кол-во цепей, предельные значения
и т.п. Использовать эти данные при режимах проверки батареи.
5. Добавить ini-файл, в котором будут находиться все константы батарей (строковые, целые, вещественные) для КДС.
По началу работы разбирать ini-файл и устанавливать параметры батарей в соответствии с.
6. Добавить класс работы с файлом отчёта и само наполнение файла.

7. График распассивации.

8. !!! Сохранение текущего промежуточного состояния проверки, возобновление проверки с места предыдущей остановки сеанса проверки.
Текущая проверка - прошедшие проверки, остановка, сохранение незаконченной долгой проверки, и т.д.
Отчёт продолжить в старом файле, или бахнуть новый? ...
Использовать журнал событий для.
*/

/* Соглашения:
 * 1. три воскл.знака !!! в комментариях - обратить внимание, временный или недоделанный участок кода, важное замечание.
 * 2. Писать как можно больше ОСМЫСЛЕННЫХ комментариев. Потому что потом хрен вспомнишь, что где и по чём.
 * 3. Комментировать назначение всех переменных и функций (что, для чего, параметры, возвращаемое значение)
    (ну, кроме откровенно вспомогательных переменных, типа счётчика цикла).
 * Желательно в стиле Doxygen. Может быть потом автоматом соберётся хелп по программке.
 * 4. Имена виджетов и контролов давать с префиксом, отражающим суть виджета - btn_, button_, pushButton_, label_  и т.п.
 * 5.
*/

/* Описание начала работы:
 * При начале работы пользователь должен открыть последовательный порт. Если портов нет, или порт открылся с ошибкой,
 * то дальнейшая работа невозможна.
 * Когда порт открылся нормально, периодически посылается пинг, и при нормальном ответе пишется сообщение, что связь установлена.
 * При отсутствии ответа пишется, что связи нет.
 * После установления связи с коробком, выдать команду IDLE, для сброса коробка в исходное состояние.
 *
 * По протоколу обмена: сначала режим собирается, а потом он опрашивается.
 */

#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    serialPort(new SerialPort),
    start_work(true)
{
    ui->setupUi(this);

    //+++ Edward
    //ui->btnCOMPortDisconnect->hide(); // !!! вообще отключу, за ненадобностью. надо будет выкинуть из формы
    // Timeout - непериодический. таймаут ответа коробочки
    timeout = new QTimer;
    timeout->setSingleShot(true);
    connect(timeout, SIGNAL(timeout()), this, SLOT(procTimeout()));
    // Таймер между пингами.  непериодический
    timerPing = new QTimer;
    timerPing->setSingleShot(true);
    //connect(timerPing, SIGNAL(timeout()), this, SLOT(procTimerPing())); // на всякий случай. пока не понадобилось
    connect(timerPing, SIGNAL(timeout()), this, SLOT(sendPing())); // по окончанию паузы между пингами - послать следующий

    // Таймер задержки выдачи следующей команды после выдачи ИДЛЕ, команд, опрос/запрос.
    timerDelay = new QTimer;
    timerDelay->setSingleShot(true);
    // Таймер задержки выдачи следующей команды после выдачи ИДЛЕ, команд, опрос/запрос.
    timerDelay0 = new QTimer;
    timerDelay0->setSingleShot(true);
    // Таймер задержки выдачи следующей команды после выдачи ИДЛЕ, команд, опрос/запрос.
    timerDelay1 = new QTimer;
    timerDelay1->setSingleShot(true);

    delayTime = 0; // обнулим время для timerDelay. потом будет подставляться необходимое кол-во мсек.
    commandString = "IDLE#"; // init some string

    // посылать данные в порт. можно, конечно, напрямую serialPort->writeSerialPort();  но лучше так.
    connect(this, SIGNAL(sendSerialData(quint8,QByteArray)), serialPort, SLOT(writeSerialPort(quint8,QByteArray)));
    // при получении на порту данных - принимать их в главном окне
    connect(serialPort, SIGNAL(readySerialData(quint8,QByteArray)), this, SLOT(recvSerialData(quint8,QByteArray)));

    // Настройка и запуск КА
    setupMachine();
    //+++


    ui->groupBoxDiagnosticDevice->setDisabled(true);
    ui->groupBoxDiagnosticMode->setDisabled(true);
    ui->groupBoxCheckParams->setDisabled(true);
    ui->rbInsulationResistanceMeasuringBoardUUTBB->hide();
    ui->cbInsulationResistanceMeasuringBoardUUTBB->hide();
    ui->btnInsulationResistanceMeasuringBoardUUTBB->hide();
    ui->btnInsulationResistanceMeasuringBoardUUTBB_2->hide();
    ui->rbOpenCircuitVoltagePowerSupply->hide();
    ui->cbOpenCircuitVoltagePowerSupply->hide();
    ui->btnOpenCircuitVoltagePowerSupply->hide();
    ui->btnOpenCircuitVoltagePowerSupply_2->hide();
    ui->rbClosedCircuitVoltagePowerSupply->hide();
    ui->cbClosedCircuitVoltagePowerSupply->hide();
    ui->btnClosedCircuitVoltagePowerSupply->hide();
    ui->btnClosedCircuitVoltagePowerSupply_2->hide();

    for (int i = 1; i < 9; i++) {
        ui->tabWidget->removeTab(1);
    }

    //bStop = false;
    bPause = false;
    bCheckCompleteVoltageOnTheHousing = false;
    bCheckCompleteInsulationResistance = false;
    bCheckCompleteOpenCircuitVoltageGroup = false;
    bCheckCompleteClosedCircuitVoltageGroup = false;
    bCheckCompleteClosedCircuitVoltageBattery = false;
    bCheckCompleteInsulationResistanceMeasuringBoardUUTBB = false;
    bCheckCompleteOpenCircuitVoltagePowerSupply = false;
    bCheckCompleteClosedCircuitVoltagePowerSupply = false;
    iBatteryIndex = 0;
    iStep = 0;
    iAllSteps = 0;
    iStepVoltageOnTheHousing = 1;
    iStepInsulationResistance = 1;
    iStepOpenCircuitVoltageGroup = 1;

    //ResetCheck();
    getCOMPorts();
    //com = new QSerialPort(this);
    //connect(ui->btnCOMPortConnect, SIGNAL(clicked(bool)), this, SLOT(openCOMPort()));
    //connect(ui->btnCOMPortDisconnect, SIGNAL(clicked(bool)), this, SLOT(closeCOMPort()));
    connect(ui->cbIsUUTBB, SIGNAL(toggled(bool)), this, SLOT(isUUTBB()));
    connect(ui->comboBoxBatteryList, SIGNAL(currentIndexChanged(int)), this , SLOT(handleSelectionChangedBattery(int)));
    connect(ui->rbModeDiagnosticAuto, SIGNAL(toggled(bool)), ui->groupBoxCheckParams, SLOT(setDisabled(bool)));
    connect(ui->rbModeDiagnosticAuto, SIGNAL(toggled(bool)), ui->btnStartAutoModeDiagnostic, SLOT(setEnabled(bool)));
    connect(ui->rbModeDiagnosticManual, SIGNAL(toggled(bool)), ui->rbVoltageOnTheHousing, SLOT(setEnabled(bool)));
    //connect(ui->rbModeDiagnosticManual, SIGNAL(toggled(bool)), ui->btnPauseAutoModeDiagnostic, SLOT(setDisabled(bool)));
    connect(ui->rbVoltageOnTheHousing, SIGNAL(toggled(bool)), ui->btnVoltageOnTheHousing, SLOT(setEnabled(bool)));
    connect(ui->rbVoltageOnTheHousing, SIGNAL(toggled(bool)), ui->cbVoltageOnTheHousing, SLOT(setEnabled(bool)));
    connect(ui->rbInsulationResistance, SIGNAL(toggled(bool)), ui->btnInsulationResistance, SLOT(setEnabled(bool)));
    connect(ui->rbOpenCircuitVoltageGroup, SIGNAL(toggled(bool)), ui->btnOpenCircuitVoltageGroup, SLOT(setEnabled(bool)));
    connect(ui->rbClosedCircuitVoltageGroup, SIGNAL(toggled(bool)), ui->btnClosedCircuitVoltageGroup, SLOT(setEnabled(bool)));
    connect(ui->rbClosedCircuitVoltageBattery, SIGNAL(toggled(bool)), ui->btnClosedCircuitVoltageBattery, SLOT(setEnabled(bool)));
    connect(ui->rbDepassivation, SIGNAL(toggled(bool)), ui->btnDepassivation, SLOT(setEnabled(bool)));
    connect(ui->rbInsulationResistanceMeasuringBoardUUTBB, SIGNAL(toggled(bool)), ui->btnInsulationResistanceMeasuringBoardUUTBB, SLOT(setEnabled(bool)));
    connect(ui->rbOpenCircuitVoltagePowerSupply, SIGNAL(toggled(bool)), ui->btnOpenCircuitVoltagePowerSupply, SLOT(setEnabled(bool)));
    connect(ui->rbClosedCircuitVoltagePowerSupply, SIGNAL(toggled(bool)), ui->btnClosedCircuitVoltagePowerSupply, SLOT(setEnabled(bool)));
    //connect(ui->btnVoltageOnTheHousing, SIGNAL(clicked(int)), this, SLOT(checkVoltageOnTheHousing(iBatteryIndex, iStepVoltageOnTheHousing)));
    connect(ui->btnVoltageOnTheHousing, SIGNAL(clicked(bool)), this, SLOT(checkVoltageOnTheHousing()));
    connect(ui->btnInsulationResistance, SIGNAL(clicked(bool)), this, SLOT(checkInsulationResistance()));
    connect(ui->btnOpenCircuitVoltageGroup, SIGNAL(clicked(bool)), this, SLOT(checkOpenCircuitVoltageGroup()));
    connect(ui->btnClosedCircuitVoltageGroup, SIGNAL(clicked(bool)), this, SLOT(checkClosedCircuitVoltageGroup()));
    connect(ui->btnDepassivation, SIGNAL(clicked(bool)), this, SLOT(checkDepassivation()));
    connect(ui->btnClosedCircuitVoltageBattery, SIGNAL(clicked(bool)), this, SLOT(checkClosedCircuitVoltageBattery()));
    connect(ui->btnInsulationResistanceMeasuringBoardUUTBB, SIGNAL(clicked(bool)), this, SLOT(checkInsulationResistanceMeasuringBoardUUTBB()));
    connect(ui->btnOpenCircuitVoltagePowerSupply, SIGNAL(clicked(bool)), this, SLOT(checkOpenCircuitVoltagePowerSupply()));
    connect(ui->btnClosedCircuitVoltagePowerSupply, SIGNAL(clicked(bool)), this, SLOT(checkClosedCircuitVoltagePowerSupply()));
    connect(ui->btnVoltageOnTheHousing_2, SIGNAL(clicked(bool)), this, SLOT(checkVoltageOnTheHousing()));
    connect(ui->btnInsulationResistance_2, SIGNAL(clicked(bool)), this, SLOT(checkInsulationResistance()));
    connect(ui->btnOpenCircuitVoltageGroup_2, SIGNAL(clicked(bool)), this, SLOT(checkOpenCircuitVoltageGroup()));
    connect(ui->btnClosedCircuitVoltageGroup_2, SIGNAL(clicked(bool)), this, SLOT(checkClosedCircuitVoltageGroup()));
    connect(ui->btnClosedCircuitVoltageBattery_2, SIGNAL(clicked(bool)), this, SLOT(checkClosedCircuitVoltageBattery()));
    connect(ui->btnDepassivation_2, SIGNAL(clicked(bool)), this, SLOT(checkDepassivation()));
    connect(ui->btnInsulationResistanceMeasuringBoardUUTBB_2, SIGNAL(clicked(bool)), this, SLOT(checkInsulationResistanceMeasuringBoardUUTBB()));
    connect(ui->btnOpenCircuitVoltagePowerSupply_2, SIGNAL(clicked(bool)), this, SLOT(checkOpenCircuitVoltagePowerSupply()));
    connect(ui->btnClosedCircuitVoltagePowerSupply_2, SIGNAL(clicked(bool)), this, SLOT(checkClosedCircuitVoltagePowerSupply()));
    connect(ui->btnStartAutoModeDiagnostic, SIGNAL(clicked(bool)), this, SLOT(checkAutoModeDiagnostic()));
    //connect(ui->btnPauseAutoModeDiagnostic, SIGNAL(clicked(bool)), this, SLOT(setPause()));
    //connect(ui->btnCOMPortDisconnect, SIGNAL(clicked(bool)), ui->btnStartAutoModeDiagnostic, SLOT(setEnabled(bool)));
}

MainWindow::~MainWindow()
{
    delete ui;
}

//+++ Edward ====================================================================================================================
// Приём данных от последовательного порта
void MainWindow::recvSerialData(quint8 operation_code, const QByteArray data)
{
    //qDebug()<<"mainwindow.cpp recvSerialData " << data.length() << " bytes " << data.toHex() << " text " << qPrintable(data);
    // когда приняли данные вовремя - остановить таймаут
    timeout->stop();
    //ui->statusBar->showMessage(tr(ONLINE)); // напишем в строке статуса, что связь есть

    if(operation_code == 0x01) // если приняли пинг, то
    {
        // следующий пинг пошлётся по окончанию timerPing
        if(data == PING) // вот тут по-хорошему надо бы по результам анализа ответа делать что-то полезное. только хз.
        {
            //qDebug()<<"ping correct";
            ui->statusBar->showMessage(tr(ONLINE)); // напишем в строке статуса, что связь есть, только при нормальном пинге
            if(start_work) // если первый ответ после установления связи
            {
                // сбросить коробок, послать IDLE
                // подготовим переменные: текущая команда#, пауза по протоколу, ф-ия разбора рез-та
                prepareSendIdleToFirstCommand();
                emit workStart(); // по этому сигналу КА перейдёт в состояние посылки первого сброса коробочки
            }
        }
        else
        {
            qDebug()<<"ping incorrect";
        }
        return;
    }
    if(operation_code == 0x08) // если приняли ответ на команду
    {
        // ищем требуемую строку по шаблону
        //if( data.indexOf(commandString + "OK")>=0 ) // Команда#OK режима отработана
        if( data.contains("OK") ) // Команда#OK режима отработана/ совсем простенькая проверка на наличии в ответе OK
        {
            timerDelay->start(delayTime); // после нормального выполнения команды запустить задержку перед следующим обменом, по протоколу
            // начинать отсчитывать задержку перед анализом инфы.  во-первых там может подготавливаться следующая команда и глобальная переменная delayTime перепишется
            // а во-вторых, чего зря простаивать.

            // выполнить ф-ю разбора принятой инфы
            (this->*funcCommandAnswer)(data);
        }
        else // пришла какая-то другая посылка
        {
            qDebug()<<"Incorrect reply. Should be "<<(commandString + "OK")<<" but got: "<<data;
            //emit signalWrongReply(); // по сигналу КА перейдёт в режим готовности/простоя/ожидания
        }
    }
}

// послать пинг
void MainWindow::sendPing()
{
    //qDebug()<<"sendPing";
    sendSerialData(0x01, PING); // пошлём пинг
    timeout->start(delay_timeOut); // заведём тайм-аут на не ответ
    timerPing->start(delay_timerPing); // цикл между пингами
}

// Посылка подготовленной команды commandString в порт
void MainWindow::sendCommand()
{
    qDebug()<<"sendCommand"<<commandString;
    timerPing->stop(); // остановим таймеры. отключим пинг и предыдущий таймаут (если вдруг он был)
    timeout->stop(); //
    timerDelay->stop(); // остановить таймеры задержек перед выдачей следующих команд после текущей.
    timerDelay0->stop();
    timerDelay1->stop();
    sendSerialData(8, qPrintable(commandString)); // послать команду (8 - это по протоколу)
    timeout->start(delay_timeOut); // заведём тайм-аут на неответ
}

// Подготовка команды для её последующей посылки в порт
void MainWindow::prepareSendCommand(QString cS, int dT, void (MainWindow::*fCA)(QByteArray))
{
    // подготовим переменные
    commandString = cS;
    delayTime = dT;
    funcCommandAnswer = fCA; // назначим конкретную ф-ию разбора ответа
}

// Подготовка команды IDLE с возвратом в первое состояние
void MainWindow::prepareSendIdleToFirstCommand()
{
    prepareSendCommand("IDLE#", delay_after_IDLE_before_other, &onIdleOK);
}

// нет ответа на запрос
void MainWindow::procTimeout()
{
    qDebug()<<"procTimeout";
    ui->statusBar->showMessage(tr(OFFLINE)); // напишем нет связи
}
//+++

/*
 * УУТББ дополнительные параметры проверки
 */
void MainWindow::setPause() {
    //bPause = ((QPushButton*)sender())->isChecked() ? true : false;
    bPause = true;
    ((QPushButton*)sender())->setEnabled(false);
    ui->btnStartAutoModeDiagnostic->setEnabled(true);
}


/*
 * УУТББ дополнительные параметры проверки
 */
void MainWindow::isUUTBB()
{
    if (ui->cbIsUUTBB->isChecked()) {
        ui->rbInsulationResistanceMeasuringBoardUUTBB->show();
        ui->cbInsulationResistanceMeasuringBoardUUTBB->show();
        ui->btnInsulationResistanceMeasuringBoardUUTBB->show();
        ui->btnInsulationResistanceMeasuringBoardUUTBB_2->show();
        ui->rbOpenCircuitVoltagePowerSupply->show();
        ui->cbOpenCircuitVoltagePowerSupply->show();
        ui->btnOpenCircuitVoltagePowerSupply->show();
        ui->btnOpenCircuitVoltagePowerSupply_2->show();
        ui->rbClosedCircuitVoltagePowerSupply->show();
        ui->cbClosedCircuitVoltagePowerSupply->show();
        ui->btnClosedCircuitVoltagePowerSupply->show();
        ui->btnClosedCircuitVoltagePowerSupply_2->show();
    } else {
        ui->rbInsulationResistanceMeasuringBoardUUTBB->hide();
        ui->cbInsulationResistanceMeasuringBoardUUTBB->hide();
        ui->btnInsulationResistanceMeasuringBoardUUTBB->hide();
        ui->btnInsulationResistanceMeasuringBoardUUTBB_2->hide();
        ui->rbOpenCircuitVoltagePowerSupply->hide();
        ui->cbOpenCircuitVoltagePowerSupply->hide();
        ui->btnOpenCircuitVoltagePowerSupply->hide();
        ui->btnOpenCircuitVoltagePowerSupply_2->hide();
        ui->rbClosedCircuitVoltagePowerSupply->hide();
        ui->cbClosedCircuitVoltagePowerSupply->hide();
        ui->btnClosedCircuitVoltagePowerSupply->hide();
        ui->btnClosedCircuitVoltagePowerSupply_2->hide();
    }
}


/*
 * Прогресс бар шаг вперед
 */
void MainWindow::progressBarSet(int iVal)
{
    ui->progressBar->setValue(ui->progressBar->value()+iVal);
}

/*
 * COM Порт получения списка портов
 */
void MainWindow::getCOMPorts()
{
    ui->comboBoxCOMPort->clear();
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        QStringList list;
        list << info.portName();
        ui->comboBoxCOMPort->addItems(list);
    }
}

/*
 * Задержка в милисекундах
 */
void MainWindow::delay( int millisecondsToWait )
{
    QTime dieTime = QTime::currentTime().addMSecs( millisecondsToWait );
    while( QTime::currentTime() < dieTime )
    {
        QCoreApplication::processEvents( QEventLoop::AllEvents, 100 );
    }
}


/*
 * Выбор батареи
 */
void MainWindow::handleSelectionChangedBattery(int index)
{
    if (index == 0 or index == 1 or index == 4) {
        ui->cbIsUUTBB->setEnabled(true);
    } else {
        ui->cbIsUUTBB->setEnabled(false);
        ui->cbIsUUTBB->setChecked(false);

    }

    iBatteryIndex = QString::number(index).toInt();
}

/*
 * Запись событий в журнал
 */
void MainWindow::Log(QString message, QString color)
{
    QTime time = QTime::currentTime();
    QString text = time.toString("hh:mm:ss.zzz") + " ";
    text = (color == "green" or color == "red" or color == "blue") ? text + "<font color=\""+color+"\">"+message+"</font>" : text + message;
    ui->EventLog->appendHtml(tr("%1").arg(text));
}


/*
 * Автоматический режим диагностики
 */
void MainWindow::checkAutoModeDiagnostic()
{
    bPause = false;
    ui->btnStartAutoModeDiagnostic->setEnabled(false);
    ui->groupBoxCOMPort->setEnabled(false);
    ui->groupBoxDiagnosticDevice->setEnabled(false);
    ui->groupBoxDiagnosticMode->setEnabled(false);
    //ui->btnPauseAutoModeDiagnostic->setEnabled(true);
    ui->btnBuildReport->setEnabled(false);
    if(!bCheckCompleteVoltageOnTheHousing)
        checkVoltageOnTheHousing();
    if(!bCheckCompleteInsulationResistance)
        checkInsulationResistance();
    if(!bCheckCompleteOpenCircuitVoltageGroup)
        checkOpenCircuitVoltageGroup();
    if(!bCheckCompleteClosedCircuitVoltageGroup)
        checkClosedCircuitVoltageGroup();
    if(!bCheckCompleteClosedCircuitVoltageBattery)
        checkClosedCircuitVoltageBattery();
    if (ui->cbIsUUTBB->isChecked()) {
        if(!bCheckCompleteClosedCircuitVoltageBattery)
            checkInsulationResistanceMeasuringBoardUUTBB();
        if(!bCheckCompleteOpenCircuitVoltagePowerSupply)
            checkOpenCircuitVoltagePowerSupply();
        if(!bCheckCompleteClosedCircuitVoltagePowerSupply)
            checkClosedCircuitVoltagePowerSupply();
    }
    //ui->btnStopCheck->setEnabled(false);
    //ui->btnBuildReport->setEnabled(true);
    if (bPause) { Log("ПАУЗА.", "red"); return; }
    ui->groupBoxCOMPort->setEnabled(true);
    ui->groupBoxDiagnosticDevice->setEnabled(true);
    ui->groupBoxDiagnosticMode->setEnabled(true);
    //ui->btnPauseAutoModeDiagnostic->setEnabled(false);
    ui->btnBuildReport->setEnabled(true);
    Log("Проверка завершена - Автоматический режим", "blue");
    Log(tr("[ОТЛАДКА] progressBarValue= %1, progressBarMaximum= %2").arg(ui->progressBar->value()).arg(ui->progressBar->maximum()), "blue");
}

/*
 * Напряжение на корпусе батареи
 */
void MainWindow::checkVoltageOnTheHousing()
{
    if (((QPushButton*)sender())->objectName() == "btnVoltageOnTheHousing") {
        /*if (((QPushButton*)sender())->isDown()) {
            ((QPushButton*)sender())->setText(tr("Стоп"));

        } else {
            ((QPushButton*)sender())->setText(tr("Пуск"));
        }*/
        iStepVoltageOnTheHousing = 1;
        bPause = false;
        ui->btnVoltageOnTheHousing_2->setEnabled(false);
    }
    if (((QPushButton*)sender())->objectName() == "btnVoltageOnTheHousing_2")
        bPause = false;
    if (bPause) return;
    ui->groupBoxCOMPort->setEnabled(false);
    ui->groupBoxDiagnosticDevice->setEnabled(false);
    ui->groupBoxDiagnosticMode->setEnabled(false);
    ui->tabWidget->addTab(ui->tabVoltageOnTheHousing, ui->rbVoltageOnTheHousing->text());
    //ui->tabWidget->setCurrentIndex(ui->tabWidget->count()-1);
    Log(tr("Проверка начата - %1").arg(ui->rbVoltageOnTheHousing->text()), "blue");
    switch (iBatteryIndex) {
    case 0: //9ER20P-20
        ui->progressBar->setValue(iStepVoltageOnTheHousing-1);
        ui->progressBar->setMaximum(2);
        while (iStepVoltageOnTheHousing <= 2) {
            if (bPause) return;
            switch (iStepVoltageOnTheHousing) {
            case 1:
                delay(1000);
                param = qrand()%3; //число полученное с COM-порта
                str = "1) между точкой металлизации и контактом 1 соединителя Х1 «Х1+» = <b>"+QString::number(param)+"</b>";
                break;
            case 2:
                delay(1000);
                param = qrand()%3;; //число полученное с COM-порта
                str = "2) между точкой металлизации и контактом 1 соединителя Х3 «Х3-» = <b>"+QString::number(param)+"</b>";
                break;
            default:
                return;
                break;
            }

            QLabel * label = findChild<QLabel*>(tr("labelVoltageOnTheHousing%1").arg(iStepVoltageOnTheHousing));
            label->setText(tr("%1) %2").arg(iStepVoltageOnTheHousing).arg(QString::number(param)));
            Log(str, (param > 1) ? "red" : "green");
            if (param > 1) {
                ui->rbModeDiagnosticManual->setChecked(true);
                ui->rbModeDiagnosticAuto->setEnabled(false);
                if (QMessageBox::question(this, "Внимание - "+ui->rbVoltageOnTheHousing->text(), tr("%1 \nпродолжить?").arg(str), tr("Да"), tr("Нет"))) {
                    ui->btnVoltageOnTheHousing_2->setEnabled(true);
                    bPause = true;
                    return;
                }
            }
            progressBarSet(1);
            iStepVoltageOnTheHousing++;
        }
        if (ui->rbModeDiagnosticAuto->isChecked())
            bCheckCompleteVoltageOnTheHousing = true;
        break;
    case 1:
        if (bPause) return;
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(1);
        break;
    case 2:
        if (bPause) return;
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(1);
        break;
    case 3:
        if (bPause) return;
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(1);
        break;
    default:
        break;
    }
    Log(tr("Проверка завершена - %1").arg(ui->rbVoltageOnTheHousing->text()), "blue");
    iStepVoltageOnTheHousing = 1;
    ui->rbInsulationResistance->setEnabled(true);
    ui->groupBoxCOMPort->setEnabled(true);
    ui->groupBoxDiagnosticDevice->setEnabled(true);
    ui->groupBoxDiagnosticMode->setEnabled(true);
}

/*
 * Сопротивление изоляции
 */
void MainWindow::checkInsulationResistance()
{
    if (((QPushButton*)sender())->objectName() == "btnInsulationResistance") {
        iStepInsulationResistance = 1;
        bPause = false;
        ui->btnInsulationResistance_2->setEnabled(false);
    }
    if (((QPushButton*)sender())->objectName() == "btnInsulationResistance_2")
        bPause = false;
    if (bPause) return;
    ui->groupBoxCOMPort->setEnabled(false);
    ui->groupBoxDiagnosticDevice->setEnabled(false);
    ui->groupBoxDiagnosticMode->setEnabled(false);
    ui->tabWidget->addTab(ui->tabInsulationResistance, ui->rbInsulationResistance->text());
    //ui->tabWidget->setCurrentIndex(ui->tabWidget->count()-1);
    Log(tr("Проверка начата - %1").arg(ui->rbInsulationResistance->text()), "blue");
    switch (iBatteryIndex) {
    case 0: //9ER20P-20
        ui->progressBar->setValue(iStepInsulationResistance-1);
        ui->progressBar->setMaximum(4);
        while (iStepInsulationResistance <= 4) {
            if (bPause) return;
            switch (iStepInsulationResistance) {
            case 1:
                delay(1000);
                param = qrand()%25; //число полученное с COM-порта
                str = "1) между точкой металлизации и контактом 1 соединителя Х1 «Х1+» = <b>"+QString::number(param)+"</b>";
                break;
            case 2:
                delay(1000);
                param = qrand()%25; //число полученное с COM-порта
                str = "2) между точкой металлизации и контактом 1 соединителя Х3 «Х3-» = <b>"+QString::number(param)+"</b>";
                break;
            case 3:
                delay(1000);
                param = qrand()%25; //число полученное с COM-порта
                str = "3) между точкой металлизации и контактом 6 соединителя Х1 «Х1+» = <b>"+QString::number(param)+"</b>";
                break;
            case 4:
                delay(1000);
                param = qrand()%25; //число полученное с COM-порта
                str = "4) между точкой металлизации и контактом 7 соединителя Х3 «Х3-» = <b>"+QString::number(param)+"</b>";
                break;
            default:
                return;
                break;
            }

            QLabel * label = findChild<QLabel*>(tr("labelInsulationResistance%1").arg(iStepInsulationResistance));
            label->setText(tr("%1) %2").arg(iStepInsulationResistance).arg(QString::number(param)));
            Log(str, (param < 20) ? "red" : "green");
            if (param < 20) {
                ui->rbModeDiagnosticManual->setChecked(true);
                ui->rbModeDiagnosticAuto->setEnabled(false);
                //ui->rbInsulationResistance->setChecked(true);
                if (QMessageBox::question(this, "Внимание - "+ui->rbInsulationResistance->text(), tr("%1 \nпродолжить?").arg(str), tr("Да"), tr("Нет"))) {
                    ui->btnInsulationResistance_2->setEnabled(true);
                    bPause = true;
                    return;
                }
            }
            progressBarSet(1);
            iStepInsulationResistance++;
        }
        ui->btnInsulationResistance_2->setEnabled(false);
        if (ui->rbModeDiagnosticAuto->isChecked())
             bCheckCompleteInsulationResistance = true;
        break;
    case 1:
        if (bPause) return;
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(1);
        break;
    case 2:
        if (bPause) return;
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(1);
        break;
    case 3:
        if (bPause) return;
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(1);
        break;
    default:
        break;
    }
    Log(tr("Проверка завершена - %1").arg(ui->rbInsulationResistance->text()), "blue");
    iStepInsulationResistance = 1;
    ui->rbOpenCircuitVoltageGroup->setEnabled(true);
    ui->groupBoxCOMPort->setEnabled(true);
    ui->groupBoxDiagnosticDevice->setEnabled(true);
    ui->groupBoxDiagnosticMode->setEnabled(true);
}

/*
 * Напряжение разомкнутой цепи группы
 */
void MainWindow::checkOpenCircuitVoltageGroup()
{
    if (((QPushButton*)sender())->objectName() == "btnOpenCircuitVoltageGroup") {
        iStepOpenCircuitVoltageGroup = 1;
        bPause = false;
        ui->btnOpenCircuitVoltageGroup_2->setEnabled(false);
    }
    if (((QPushButton*)sender())->objectName() == "btnOpenCircuitVoltageGroup_2")
        bPause = false;
    if (bPause) return;
    ui->groupBoxCOMPort->setEnabled(false);
    ui->groupBoxDiagnosticDevice->setEnabled(false);
    ui->groupBoxDiagnosticMode->setEnabled(false);
    ui->tabWidget->addTab(ui->tabOpenCircuitVoltageGroup, ui->rbOpenCircuitVoltageGroup->text());
    //ui->tabWidget->setCurrentIndex(ui->tabWidget->count()-1);
    Log(tr("Проверка начата - %1").arg(ui->rbOpenCircuitVoltageGroup->text()), "blue");
    switch (iBatteryIndex) {
    case 0: //9ER20P-20
        ui->progressBar->setValue(iStepOpenCircuitVoltageGroup-1);
        ui->progressBar->setMaximum(20);
        while (iStepOpenCircuitVoltageGroup <= 20) {
            if (bPause) return;
            switch (iStepOpenCircuitVoltageGroup) {
            case 1:
                delay(1000);
                param = qrand()%40+10; //число полученное с COM-порта
                break;
            case 2:
                delay(1000);
                param = qrand()%40+10; //число полученное с COM-порта
                break;
            case 3:
                delay(1000);
                param = qrand()%40+10; //число полученное с COM-порта
                break;
            case 4:
                delay(1000);
                param = qrand()%40+10; //число полученное с COM-порта
                break;
            case 5:
                delay(1000);
                param = qrand()%40+10; //число полученное с COM-порта
                break;
            case 6:
                delay(1000);
                param = qrand()%40+10; //число полученное с COM-порта
                break;
            case 7:
                delay(1000);
                param = qrand()%40+10; //число полученное с COM-порта
                break;
            case 8:
                delay(1000);
                param = qrand()%40+10; //число полученное с COM-порта
                break;
            case 9:
                delay(1000);
                param = qrand()%40+10; //число полученное с COM-порта
                break;
            case 10:
                delay(1000);
                param = qrand()%40+10; //число полученное с COM-порта
                break;
            case 11:
                delay(1000);
                param = qrand()%40+10; //число полученное с COM-порта
                break;
            case 12:
                delay(1000);
                param = qrand()%40+10; //число полученное с COM-порта
                break;
            case 13:
                delay(1000);
                param = qrand()%40+10; //число полученное с COM-порта
                break;
            case 14:
                delay(1000);
                param = qrand()%40+10; //число полученное с COM-порта
                break;
            case 15:
                delay(1000);
                param = qrand()%40+10; //число полученное с COM-порта
                break;
            case 16:
                delay(1000);
                param = qrand()%40+10; //число полученное с COM-порта
                break;
            case 17:
                delay(1000);
                param = qrand()%40+10; //число полученное с COM-порта
                break;
            case 18:
                delay(1000);
                param = qrand()%40+10; //число полученное с COM-порта
                break;
            case 19:
                delay(1000);
                param = qrand()%40+10; //число полученное с COM-порта
                break;
            case 20:
                delay(1000);
                param = qrand()%40+10; //число полученное с COM-порта
                break;
            default:
                return;
                break;
            }

            QLabel * label = findChild<QLabel*>(tr("labelOpenCircuitVoltageGroup%1").arg(iStepOpenCircuitVoltageGroup));
            label->setText(tr("%1) %2").arg(iStepOpenCircuitVoltageGroup).arg(QString::number(param)));
            str = tr("%1) между контактом 1 соединителя Х3 «Х3-» и контактом %1 соединителя Х4 «4» = <b>%2</b>").arg(iStepOpenCircuitVoltageGroup).arg(QString::number(param));
            Log(str, (param < 32.3) ? "red" : "green");
            if (param < 32.3) {
                ui->rbModeDiagnosticManual->setChecked(true);
                ui->rbModeDiagnosticAuto->setEnabled(false);
                //ui->rbInsulationResistance->setChecked(true);
                if (QMessageBox::question(this, "Внимание - "+ui->rbOpenCircuitVoltageGroup->text(), tr("%1 \nпродолжить?").arg(str), tr("Да"), tr("Нет"))) {
                    ui->btnOpenCircuitVoltageGroup_2->setEnabled(true);
                    bPause = true;
                    return;
                }
            }
            progressBarSet(1);
            iStepOpenCircuitVoltageGroup++;
        }
        ui->btnOpenCircuitVoltageGroup_2->setEnabled(false);
        if (ui->rbModeDiagnosticAuto->isChecked())
            bCheckCompleteClosedCircuitVoltageGroup = true;
        break;
    case 1:
        if (bPause) return;
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(1);
        break;
    case 2:
        if (bPause) return;
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(1);
        break;
    case 3:
        if (bPause) return;
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(1);
        break;
    default:
        break;
    }

    Log(tr("Проверка завершена - %1").arg(ui->rbOpenCircuitVoltageGroup->text()), "blue");
    iStepOpenCircuitVoltageGroup = 1;
    ui->rbClosedCircuitVoltageGroup->setEnabled(true);
    ui->groupBoxCOMPort->setEnabled(true);
    ui->groupBoxDiagnosticDevice->setEnabled(true);
    ui->groupBoxDiagnosticMode->setEnabled(true);
}

/*
 * Напряжение замкнутой цепи группы
 */
void MainWindow::checkClosedCircuitVoltageGroup()
{
    if (((QPushButton*)sender())->objectName() == "btnClosedCircuitVoltageGroup") {
        iStepClosedCircuitVoltageGroup = 1;
        bPause = false;
        ui->btnClosedCircuitVoltageGroup_2->setEnabled(false);
    }
    if (((QPushButton*)sender())->objectName() == "btnClosedCircuitVoltageGroup_2")
        bPause = false;
    if (bPause) return;
    ui->groupBoxCOMPort->setEnabled(false);
    ui->groupBoxDiagnosticDevice->setEnabled(false);
    ui->groupBoxDiagnosticMode->setEnabled(false);
    ui->tabWidget->addTab(ui->tabOpenCircuitVoltageGroup, ui->rbOpenCircuitVoltageGroup->text());
    Log(tr("Проверка начата - %1").arg(ui->rbClosedCircuitVoltageGroup->text()), "blue");
    switch (iBatteryIndex) {
    case 0: //9ER20P-20
        ui->progressBar->setValue(iStepClosedCircuitVoltageGroup-1);
        ui->progressBar->setMaximum(20);
        while (iStepClosedCircuitVoltageGroup <= 20) {
            if (bPause) return;
            switch (iStepClosedCircuitVoltageGroup) {
            case 1:
                delay(1000);
                param = qrand()%40+10; //число полученное с COM-порта
                break;
            case 2:
                delay(1000);
                param = qrand()%40+10; //число полученное с COM-порта
                break;
            case 3:
                delay(1000);
                param = qrand()%40+10; //число полученное с COM-порта
                break;
            case 4:
                delay(1000);
                param = qrand()%40+10; //число полученное с COM-порта
                break;
            case 5:
                delay(1000);
                param = qrand()%40+10; //число полученное с COM-порта
                break;
            case 6:
                delay(1000);
                param = qrand()%40+10; //число полученное с COM-порта
                break;
            case 7:
                delay(1000);
                param = qrand()%40+10; //число полученное с COM-порта
                break;
            case 8:
                delay(1000);
                param = qrand()%40+10; //число полученное с COM-порта
                break;
            case 9:
                delay(1000);
                param = qrand()%40+10; //число полученное с COM-порта
                break;
            case 10:
                delay(1000);
                param = qrand()%40+10; //число полученное с COM-порта
                break;
            case 11:
                delay(1000);
                param = qrand()%40+10; //число полученное с COM-порта
                break;
            case 12:
                delay(1000);
                param = qrand()%40+10; //число полученное с COM-порта
                break;
            case 13:
                delay(1000);
                param = qrand()%40+10; //число полученное с COM-порта
                break;
            case 14:
                delay(1000);
                param = qrand()%40+10; //число полученное с COM-порта
                break;
            case 15:
                delay(1000);
                param = qrand()%40+10; //число полученное с COM-порта
                break;
            case 16:
                delay(1000);
                param = qrand()%40+10; //число полученное с COM-порта
                break;
            case 17:
                delay(1000);
                param = qrand()%40+10; //число полученное с COM-порта
                break;
            case 18:
                delay(1000);
                param = qrand()%40+10; //число полученное с COM-порта
                break;
            case 19:
                delay(1000);
                param = qrand()%40+10; //число полученное с COM-порта
                break;
            case 20:
                delay(1000);
                param = qrand()%40+10; //число полученное с COM-порта
                break;
            default:
                return;
                break;
            }

            QLabel * label = findChild<QLabel*>(tr("labelClosedCircuitVoltageGroup%1").arg(iStepClosedCircuitVoltageGroup));
            label->setText(tr("%1) %2").arg(iStepClosedCircuitVoltageGroup).arg(QString::number(param)));
            str = tr("%1) между контактом 1 соединителя Х3 «Х3-» и контактом %1 соединителя Х4 «4» = <b>%2</b>").arg(iStepClosedCircuitVoltageGroup).arg(QString::number(param));
            Log(str, (param < 32.3) ? "red" : "green");
            if (param < 32.3) {
                int ret = QMessageBox::question(this, "Внимание - "+ui->rbClosedCircuitVoltageGroup->text(), tr("%1 \nпродолжить?").arg(str), tr("Да"), tr("Да, необходима \"Распассивация\""), tr("Нет"));
                switch (ret) {
                case 0:
                    break;
                case 1:
                    imDepassivation.append(iStepClosedCircuitVoltageGroup-1);
                    Log(tr("%1) %1 - Х4 «4» добавлен для распассивации.").arg(iStepClosedCircuitVoltageGroup-1), "blue");
                    break;
                case 2:
                    ui->btnClosedCircuitVoltageGroup_2->setEnabled(true);
                    bPause = true;
                    return;
                    break;
                default:
                    break;
                }
                ui->rbModeDiagnosticManual->setChecked(true);
                ui->rbModeDiagnosticAuto->setEnabled(false);
                //ui->rbInsulationResistance->setChecked(true);
                /*if (QMessageBox::question(this, "Внимание - "+ui->rbClosedCircuitVoltageGroup->text(), tr("%1 \nпродолжить?").arg(str), tr("Да"), tr("Нет"))) {
                    ui->btnClosedCircuitVoltageGroup_2->setEnabled(true);
                    bPause = true;
                    return;
                }*/
            }
            progressBarSet(1);
            iStepClosedCircuitVoltageGroup++;
        }
        if (imDepassivation.count() != 0)
            ui->rbDepassivation->setEnabled(true);
        ui->btnClosedCircuitVoltageGroup_2->setEnabled(false);
        if (ui->rbModeDiagnosticAuto->isChecked())
            bCheckCompleteClosedCircuitVoltageGroup = true;
        break;
    case 1:
        if (bPause) return;
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(1);
        break;
    case 2:
        if (bPause) return;
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(1);
        break;
    case 3:
        if (bPause) return;
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(1);
        break;
    default:
        break;
    }
    Log(tr("Проверка завершена - %1").arg(ui->rbClosedCircuitVoltageGroup->text()), "blue");
    iStepClosedCircuitVoltageGroup = 1;
    ui->rbClosedCircuitVoltageBattery->setEnabled(true);
    ui->groupBoxCOMPort->setEnabled(true);
    ui->groupBoxDiagnosticDevice->setEnabled(true);
    ui->groupBoxDiagnosticMode->setEnabled(true);
}

/*
 * Распассивация
 */
void MainWindow::checkDepassivation()
{
    if (((QPushButton*)sender())->objectName() == "btnDepassivation") {
        iStepDepassivation = 1;
        bPause = false;
        ui->btnDepassivation_2->setEnabled(false);
    }
    if (((QPushButton*)sender())->objectName() == "btnDepassivation_2")
        bPause = false;
    if (bPause) return;
    ui->groupBoxCOMPort->setEnabled(false);
    ui->groupBoxDiagnosticDevice->setEnabled(false);
    ui->groupBoxDiagnosticMode->setEnabled(false);
    //ui->tabWidget->addTab(ui->tabDepassivation, ui->rbDepassivation->text());
    Log(tr("Проверка начата - %1").arg(ui->rbDepassivation->text()), "blue");
    switch (iBatteryIndex) {
    case 0: //9ER20P-20
        ui->progressBar->setValue(iStepDepassivation-1);
        ui->progressBar->setMaximum(imDepassivation.count());
        while (iStepDepassivation <= imDepassivation.count()) {
            if (bPause) return;
            delay(1000);
            Log(tr("%1) между контактом 1 соединителя Х3 «Х3-» и контактом %1 соединителя Х4 «4»").arg(imDepassivation.at(iStepDepassivation-1)), "green");
            progressBarSet(1);
            iStepDepassivation++;
        }
        break;
    default:
        break;
    }
    Log(tr("Проверка завершена - %1").arg(ui->rbDepassivation->text()), "blue");
    iStepDepassivation = 1;
    //ui->rbDepassivation->setEnabled(false);
    ui->btnDepassivation_2->setEnabled(false);
    ui->groupBoxCOMPort->setEnabled(true);
    ui->groupBoxDiagnosticDevice->setEnabled(true);
    ui->groupBoxDiagnosticMode->setEnabled(true);
}

/*
 * Напряжение замкнутой цепи батареи
 */
void MainWindow::checkClosedCircuitVoltageBattery()
{
    if (((QPushButton*)sender())->objectName() == "btnClosedCircuitVoltageBattery") {
        iStepClosedCircuitVoltageBattery = 1;
        bPause = false;
        ui->btnClosedCircuitVoltageBattery_2->setEnabled(false);
    }
    if (((QPushButton*)sender())->objectName() == "btnClosedCircuitVoltageBattery_2")
        bPause = false;
    if (bPause) return;
    ui->groupBoxCOMPort->setEnabled(false);
    ui->groupBoxDiagnosticDevice->setEnabled(false);
    ui->groupBoxDiagnosticMode->setEnabled(false);
    ui->tabWidget->addTab(ui->tabClosedCircuitVoltageBattery, ui->rbClosedCircuitVoltageBattery->text());
    //ui->tabWidget->setCurrentIndex(ui->tabWidget->count()-1);
    Log(tr("Проверка начата - %1").arg(ui->rbClosedCircuitVoltageBattery->text()), "blue");
    switch (iBatteryIndex) {
    case 0: //9ER20P-20
        if (bPause) return;
        delay(1000);
        progressBarSet(1);
        ui->labelClosedCircuitVoltageBattery->setText(tr("1) %2").arg(QString::number(param)));
        str = tr("1) между контактом 1 соединителя Х1 «1+» и контактом 1 соединителя Х3 «3-» = <b>%2</b>").arg(QString::number(param));
        Log(str, (param > 30.0) ? "red" : "green");
        progressBarSet(1);
        if (param > 30.0) {
            ui->rbModeDiagnosticManual->setChecked(true);
            ui->rbModeDiagnosticAuto->setEnabled(false);
            if (QMessageBox::question(this, "Внимание - "+ui->rbClosedCircuitVoltageBattery->text(), tr("%1 \nпродолжить?").arg(str), tr("Да"), tr("Нет"))) {
                ui->btnClosedCircuitVoltageBattery_2->setEnabled(true);
                bPause = true;
                return;
            }
        }
        ui->btnClosedCircuitVoltageBattery_2->setEnabled(false);
        if (ui->rbModeDiagnosticAuto->isChecked())
            bCheckCompleteClosedCircuitVoltageBattery = true;
        break;
    case 1:
        if (bPause) return;
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(1);
        break;
    case 2:
        if (bPause) return;
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(1);
        break;
    case 3:
        if (bPause) return;
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(1);
        break;
    default:
        break;
    }
    Log(tr("Проверка завершена - %1").arg(ui->rbClosedCircuitVoltageBattery->text()), "blue");
    iStepClosedCircuitVoltageBattery = 1;
    ui->rbInsulationResistanceMeasuringBoardUUTBB->setEnabled(true);
    ui->groupBoxCOMPort->setEnabled(true);
    ui->groupBoxDiagnosticDevice->setEnabled(true);
    ui->groupBoxDiagnosticMode->setEnabled(true);
}

/*
 * Сопротивление изоляции платы измерительной УУТББ
 */
void MainWindow::checkInsulationResistanceMeasuringBoardUUTBB()
{
    //if (ui->rbModeDiagnosticAuto->isChecked() and bStop) return;
    if (bPause) return;
    ui->groupBoxCOMPort->setEnabled(false);
    ui->groupBoxDiagnosticDevice->setEnabled(false);
    ui->groupBoxDiagnosticMode->setEnabled(false);
    ui->tabWidget->addTab(ui->tabInsulationResistanceMeasuringBoardUUTBB, ui->rbInsulationResistanceMeasuringBoardUUTBB->text());
    //ui->tabWidget->setCurrentIndex(ui->tabWidget->count()-1);
    Log(tr("Проверка начата - %1").arg(ui->rbInsulationResistanceMeasuringBoardUUTBB->text()), "blue");
    switch (iBatteryIndex) {
    case 0: //9ER20P-20
        while (iStepInsulationResistanceMeasuringBoardUUTBB <= 1) {
            if (bPause) return;
            switch (iStepInsulationResistanceMeasuringBoardUUTBB) {
            case 1:
                delay(1000);
                //Log(tr("1) между точкой металлизации и контактом 1 соединителя Х1 «Х1+» = <b>%1</b>").arg(QString::number(paramInsulationResistance1)), color);
                progressBarSet(1);
                break;
            default:
                break;
            }
            iStepInsulationResistanceMeasuringBoardUUTBB++;
        }
        if (ui->rbModeDiagnosticAuto->isChecked())
            bCheckCompleteInsulationResistanceMeasuringBoardUUTBB = true;
        break;
    case 1:
        if (bPause) return;
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(1);
        break;
    case 2:
        if (bPause) return;
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(1);
        break;
    case 3:
        if (bPause) return;
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(1);
        break;
    default:
        break;
    }

    Log(tr("Проверка завершена - %1").arg(ui->rbInsulationResistanceMeasuringBoardUUTBB->text()), "blue");
    iStepInsulationResistanceMeasuringBoardUUTBB = 1;
    ui->rbOpenCircuitVoltagePowerSupply->setEnabled(true);
    ui->groupBoxCOMPort->setEnabled(true);
    ui->groupBoxDiagnosticDevice->setEnabled(true);
    ui->groupBoxDiagnosticMode->setEnabled(true);
}

/*
 * Напряжение разомкнутой цепи блока питания
 */
void MainWindow::checkOpenCircuitVoltagePowerSupply()
{
    //if (ui->rbModeDiagnosticAuto->isChecked() and bStop) return;
    if (bPause) return;
    ui->groupBoxCOMPort->setEnabled(false);
    ui->groupBoxDiagnosticDevice->setEnabled(false);
    ui->groupBoxDiagnosticMode->setEnabled(false);
    ui->tabWidget->addTab(ui->tabOpenCircuitVoltagePowerSupply, ui->rbOpenCircuitVoltagePowerSupply->text());
    //ui->tabWidget->setCurrentIndex(ui->tabWidget->count()-1);
    Log(tr("Проверка начата - %1").arg(ui->rbOpenCircuitVoltagePowerSupply->text()), "blue");
    switch (iBatteryIndex) {
    case 0: //9ER20P-20
        while (iStepOpenCircuitVoltagePowerSupply <= 1) {
            if (bPause) return;
            switch (iStepOpenCircuitVoltagePowerSupply) {
            case 1:
                delay(1000);
                //Log(tr("1) между точкой металлизации и контактом 1 соединителя Х1 «Х1+» = <b>%1</b>").arg(QString::number(paramInsulationResistance1)), color);
                progressBarSet(1);
                break;
            default:
                break;
            }
            iStepOpenCircuitVoltagePowerSupply++;
        }
        if (ui->rbModeDiagnosticAuto->isChecked())
            bCheckCompleteOpenCircuitVoltagePowerSupply = true;
        break;
    case 1:
        if (bPause) return;
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(1);
        break;
    case 2:
        if (bPause) return;
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(1);
        break;
    case 3:
        if (bPause) return;
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(1);
        break;
    default:
        break;
    }
    Log(tr("Проверка завершена - %1").arg(ui->rbOpenCircuitVoltagePowerSupply->text()), "blue");
    iStepOpenCircuitVoltagePowerSupply = 1;
    ui->rbClosedCircuitVoltagePowerSupply->setEnabled(true);
    ui->groupBoxCOMPort->setEnabled(true);
    ui->groupBoxDiagnosticDevice->setEnabled(true);
    ui->groupBoxDiagnosticMode->setEnabled(true);
}

/*
 * Напряжение замкнутой цепи блока питания
 */
void MainWindow::checkClosedCircuitVoltagePowerSupply()
{
    //if (ui->rbModeDiagnosticAuto->isChecked() and bStop) return;
    if (bPause) return;
    ui->groupBoxCOMPort->setEnabled(false);
    ui->groupBoxDiagnosticDevice->setEnabled(false);
    ui->groupBoxDiagnosticMode->setEnabled(false);
    ui->tabWidget->addTab(ui->tabClosedCircuitVoltagePowerSupply, ui->rbClosedCircuitVoltagePowerSupply->text());
    //ui->tabWidget->setCurrentIndex(ui->tabWidget->count()-1);
    Log(tr("Проверка начата - %1").arg(ui->rbClosedCircuitVoltagePowerSupply->text()), "blue");
    switch (iBatteryIndex) {
    case 0: //9ER20P-20
        while (iStepClosedCircuitVoltagePowerSupply <= 1) {
            if (bPause) return;
            switch (iStepClosedCircuitVoltagePowerSupply) {
            case 1:
                delay(1000);
                //Log(tr("1) между точкой металлизации и контактом 1 соединителя Х1 «Х1+» = <b>%1</b>").arg(QString::number(paramInsulationResistance1)), color);
                progressBarSet(1);
                break;
            default:
                break;
            }
            iStepClosedCircuitVoltagePowerSupply++;
        }
        if (ui->rbModeDiagnosticAuto->isChecked())
            bCheckCompleteClosedCircuitVoltagePowerSupply = true;
        break;
    case 1:
        if (bPause) return;
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(1);
        break;
    case 2:
        if (bPause) return;
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(1);
        break;
    case 3:
        if (bPause) return;
        Log("Действия проверки.", "green");
        delay(1000);
        progressBarSet(1);
        break;
    default:
        break;
    } 
    Log(tr("Проверка завершена - %1").arg(ui->rbClosedCircuitVoltagePowerSupply->text()), "blue");
    iStepClosedCircuitVoltagePowerSupply = 1;
    ui->groupBoxCOMPort->setEnabled(true);
    ui->groupBoxDiagnosticDevice->setEnabled(true);
    ui->groupBoxDiagnosticMode->setEnabled(true);
}
