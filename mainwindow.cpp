/* История изменений:
 * 03.02.2016 -
 * Добавил класс для работы с последовательным портом.
 * Добавил конечный автомат (КА).
 * Логика по открытию последовательного порта.
 * Добавлен Режим проверки соответствия типа выбранной оператором батареи и типа реально подключенной батареи.
 * 11.02.2016 - выкинул КА. Реализация через Qt механизм КА слишком объёмная получается.
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

Дальнейший план:
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

#include <QShortcut>

#include "mainwindow.h"
#include "ui_mainwindow.h"

QVector<Battery> battery; ///< массив типов батарей, с настройками для каждого типа

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    settings(0),
    serialPort(new SerialPort), bPortOpen(false),
    timeoutResponse(NULL), timerPing(NULL),
    loop(0),
    baRecvArray(0),
    baSendArray(0), baSendCommand(0),
    bDeveloperState(true) // режим разработчика, временно тру, потом поменять на фолс!!!
{
    ui->setupUi(this);
    model = new QStandardItemModel(5, 1); // 4 rows, 1 col
    for (int r = 0; r < 5; ++r)
    {
        QStandardItem* item;
        if(r == 0)
            item = new QStandardItem(QString("Все"));
        else
            item = new QStandardItem(QString("%0").arg(r));

        item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        item->setData(Qt::Unchecked, Qt::CheckStateRole);

        model->setItem(r, 0, item);
    }

    ui->cbInsulationResistance->setModel(model);
    connect(model, SIGNAL(itemChanged(QStandardItem*)), this, SLOT(itemChanged(QStandardItem*)));

    //+++ Edward
    // загрузить конфигурационные установки и параметры батарей из ini-файла.
    // файл находится в том же каталоге, что и исполняемый.
    settings.loadSettings();

    // по комбинации клавиш Ctrl-R перезагрузить ini-файл настроек settings.loadSettings();
    QShortcut *reloadSettings = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_R), this);
    connect(reloadSettings, SIGNAL(activated()), &settings, SLOT(loadSettings()));
    //reloadSettings->setContext(Qt::ShortcutContext::ApplicationShortcut);

    // Добавить в комбобокс наименования батарей, считанных из ини-файла
    for(int i=0; i<settings.num_batteries_types; i++)
    {
        ui->comboBoxBatteryList->addItem(battery[i].str_type_name);
    }
    // !!! написать во всех других виджетах соответствующие текущей батарее строки
    ui->cbVoltageOnTheHousing->addItem(battery[0].str_voltage_corpus[0]);
    ui->cbVoltageOnTheHousing->addItem(battery[0].str_voltage_corpus[1]);

    // посылать масив в порт. можно, конечно, напрямую serialPort->writeSerialPort();  но лучше так.
    connect(this, SIGNAL(signalSendSerialData(quint8,QByteArray)), serialPort, SLOT(writeSerialPort(quint8,QByteArray)));
    // при получении на порту данных - принимать их в главном окне
    connect(serialPort, SIGNAL(readySerialData(quint8,QByteArray)), this, SLOT(recvSerialData(quint8,QByteArray)));
    // по сигналу готовности данных по приёму из последовательного порта - выйти из пустого цикла
    connect(this, SIGNAL(signalSerialDataReady()), &loop, SLOT(quit()));

    // Timeout - непериодический. таймаут ответа коробочки
    timeoutResponse = new QTimer;
    timeoutResponse->setSingleShot(true);
    connect(timeoutResponse, SIGNAL(timeout()), this, SLOT(procTimeoutResponse()));
    // Таймер между пингами.  непериодический
    timerPing = new QTimer;
    timerPing->setSingleShot(true);
    connect(timerPing, SIGNAL(timeout()), this, SLOT(sendPing())); // по окончанию паузы между пингами - послать следующий

    ui->btnCheckConnectedBattery->setEnabled(false); // по началу работы проверять нечего
    //+++


    ui->groupBoxDiagnosticDevice->setDisabled(true);
    ui->groupBoxDiagnosticMode->setDisabled(true);
    ui->groupBoxCheckParams->setDisabled(false);// !!!
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
    //connect(ui->cbIsUUTBB, SIGNAL(toggled(bool)), this, SLOT(isUUTBB()));
    connect(ui->comboBoxBatteryList, SIGNAL(currentIndexChanged(int)), this , SLOT(handleSelectionChangedBattery(int)));
    //connect(ui->rbModeDiagnosticAuto, SIGNAL(toggled(bool)), ui->groupBoxCheckParams, SLOT(setDisabled(bool)));
    //connect(ui->rbModeDiagnosticAuto, SIGNAL(toggled(bool)), ui->btnStartAutoModeDiagnostic, SLOT(setEnabled(bool)));
    //connect(ui->rbModeDiagnosticManual, SIGNAL(toggled(bool)), ui->rbVoltageOnTheHousing, SLOT(setEnabled(bool)));
    //connect(ui->rbModeDiagnosticManual, SIGNAL(toggled(bool)), ui->btnPauseAutoModeDiagnostic, SLOT(setDisabled(bool)));
    //connect(ui->rbVoltageOnTheHousing, SIGNAL(toggled(bool)), ui->btnVoltageOnTheHousing, SLOT(setEnabled(bool)));
    //connect(ui->rbVoltageOnTheHousing, SIGNAL(toggled(bool)), ui->cbVoltageOnTheHousing, SLOT(setEnabled(bool)));
    //connect(ui->rbInsulationResistance, SIGNAL(toggled(bool)), ui->btnInsulationResistance, SLOT(setEnabled(bool)));
    connect(ui->rbOpenCircuitVoltageGroup, SIGNAL(toggled(bool)), ui->btnOpenCircuitVoltageGroup, SLOT(setEnabled(bool)));
    connect(ui->rbClosedCircuitVoltageGroup, SIGNAL(toggled(bool)), ui->btnClosedCircuitVoltageGroup, SLOT(setEnabled(bool)));
    connect(ui->rbClosedCircuitVoltageBattery, SIGNAL(toggled(bool)), ui->btnClosedCircuitVoltageBattery, SLOT(setEnabled(bool)));
    connect(ui->rbDepassivation, SIGNAL(toggled(bool)), ui->btnDepassivation, SLOT(setEnabled(bool)));
    connect(ui->rbInsulationResistanceMeasuringBoardUUTBB, SIGNAL(toggled(bool)), ui->btnInsulationResistanceMeasuringBoardUUTBB, SLOT(setEnabled(bool)));
    connect(ui->rbOpenCircuitVoltagePowerSupply, SIGNAL(toggled(bool)), ui->btnOpenCircuitVoltagePowerSupply, SLOT(setEnabled(bool)));
    connect(ui->rbClosedCircuitVoltagePowerSupply, SIGNAL(toggled(bool)), ui->btnClosedCircuitVoltagePowerSupply, SLOT(setEnabled(bool)));
    //connect(ui->btnVoltageOnTheHousing, SIGNAL(clicked(int)), this, SLOT(checkVoltageOnTheHousing(iBatteryIndex, iStepVoltageOnTheHousing)));
//Ed remove    connect(ui->btnVoltageOnTheHousing, SIGNAL(clicked(bool)), this, SLOT(checkVoltageOnTheHousing()));
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

    /*QStandardItemModel model(3, 1); // 3 rows, 1 col
    for (int r = 0; r < 3; ++r)
    {
        QStandardItem* item = new QStandardItem(QString("Item %0").arg(r));
        item->setText("test");
        item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        item->setData(Qt::Unchecked, Qt::CheckStateRole);
        model.setItem(r, 0, item);
    }
    ui->cbInsulationResistance->setModel(&model);*/


    /*connect(model, SIGNAL(dataChanged ( const QModelIndex&, const QModelIndex&)), this, SLOT(slot_changed(const QModelIndex&, const QModelIndex&)));*/
    /*Model = new QStandardItemModel;
    this->Item1 = new QStandardItem;
    this->Item1->setText("test");
    this->Item1->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
    this->Item1->setData(Qt::Unchecked, Qt::CheckStateRole);
    this->Model->insertRow(0, this->Item1);
    ui->cbInsulationResistance->setModel(&Model);*/
}
void MainWindow::itemChanged(QStandardItem* itm)
{
    QString text = itm->data(Qt::DisplayRole).toString();
    if(text == QString("Все"))
    {
        Qt::CheckState checkState = itm->checkState();
        if(checkState == Qt::Checked)
        {
           qDebug() << "Qt::Checked";
           for(int i=1; i < model->rowCount(); i++)
           {
               QStandardItem *sitm = model->item(i, 0);
               sitm->setData(Qt::Checked, Qt::CheckStateRole);
           }

        }
        else if(checkState == Qt::Unchecked)
        {
            qDebug() << "Qt::Unchecked";
            for(int i=1; i < model->rowCount(); i++)
            {
                QStandardItem *sitm = model->item(i, 0);
                sitm->setData(Qt::Unchecked, Qt::CheckStateRole);
            }
        }
    }

    ui->cbInsulationResistance->setModel(this->model);
}

MainWindow::~MainWindow()
{
    delete ui;
}

//+++ Edward ====================================================================================================================
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
            baRecvArray=data;
            emit signalSerialDataReady(); // сигнал - данные готовы. цикл ожидания закончится.
            if(bFirstPing) // если это был ответ на первый айдл, то продолжить пинг
            {
                bFirstPing = false;
                sendPing();
                ui->btnCheckConnectedBattery->setEnabled(true); // т.к. коробочка на связи и сбросилась в исходное, то разрешим кнопочку "Проверить батарею"
            }
        }
        else // пришла какая-то другая посылка
        {
            qDebug()<<"Incorrect reply. Should be "<<(baSendCommand + " and OK")<<" but got: "<<data;
            loop.exit(KDS_INCORRECT_REPLY); // вывалиться из цикла ожидания приёма с кодом ошибки неправильной команды
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
    signalSendSerialData(8, baSendArray);
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
    qDebug()<<"procTimeoutResponse";
    ui->statusBar->showMessage(tr(OFFLINE)); // напишем нет связи
    //emit signalTimeoutResponse();
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
    //qDebug()<<"sendPing"<<baSendCommand;
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
        ui->btnCheckConnectedBattery->setEnabled(false); // закрыть кнопку проверки батареи
        bPortOpen = false;
        loop.exit(-1); // закончить цикл ожидания ответа
    }
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
    if (index == 0 or index == 1) {
        ui->cbIsUUTBB->setEnabled(true);
    } else {
        ui->cbIsUUTBB->setEnabled(false);
        ui->cbIsUUTBB->setChecked(false);
    }

    iBatteryIndex = index; // зачем целое преобразовывать в строку, а потом обратно в целое? - QString::number(index).toInt();

    // !!! написать во всех других виджетах соответствующие текущей батарее строки
    ui->cbVoltageOnTheHousing->setItemText(0, battery[index].str_voltage_corpus[0]);
    ui->cbVoltageOnTheHousing->setItemText(1, battery[index].str_voltage_corpus[1]);
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
        if (bPause) {
            ((QPushButton*)sender())->setText(tr("Стоп"));
        } /*else {
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
    ui->tabWidget->setCurrentIndex(ui->tabWidget->count()-1);
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
    ui->tabWidget->setCurrentIndex(ui->tabWidget->count()-1);
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
    ui->tabWidget->setCurrentIndex(ui->tabWidget->count()-1);
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
    ui->tabWidget->addTab(ui->tabClosedCircuitVoltageGroup, ui->rbClosedCircuitVoltageGroup->text());
    ui->tabWidget->setCurrentIndex(ui->tabWidget->count()-1);
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
    ui->tabWidget->setCurrentIndex(ui->tabWidget->count()-1);
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
    ui->tabWidget->setCurrentIndex(ui->tabWidget->count()-1);
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
    ui->tabWidget->setCurrentIndex(ui->tabWidget->count()-1);
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
    ui->tabWidget->setCurrentIndex(ui->tabWidget->count()-1);
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


void MainWindow::on_rbModeDiagnosticAuto_toggled(bool checked)
{
    ui->groupBoxCheckParams->setDisabled(checked);
    ui->btnStartAutoModeDiagnostic->setEnabled(checked);
}

void MainWindow::on_rbModeDiagnosticManual_toggled(bool checked)
{
    ui->rbVoltageOnTheHousing->setEnabled(checked);
}

void MainWindow::on_rbVoltageOnTheHousing_toggled(bool checked)
{
    ui->btnVoltageOnTheHousing->setEnabled(checked);
    ui->cbVoltageOnTheHousing->setEnabled(checked);
    ui->btnVoltageOnTheHousing_2->setEnabled((checked and iStepVoltageOnTheHousing > 1) ? true : false);
}

void MainWindow::on_rbInsulationResistance_toggled(bool checked)
{
    ui->btnInsulationResistance->setEnabled(checked);
    ui->cbInsulationResistance->setEnabled(checked);
    ui->btnInsulationResistance_2->setEnabled((checked and iStepInsulationResistance > 1) ? true : false);
}

void MainWindow::on_rbOpenCircuitVoltageGroup_toggled(bool checked)
{
    ui->btnOpenCircuitVoltageGroup->setEnabled(checked);
    ui->cbOpenCircuitVoltageGroup->setEnabled(checked);
    ui->btnOpenCircuitVoltageGroup_2->setEnabled((checked and iStepOpenCircuitVoltageGroup > 1) ? true : false);
}

void MainWindow::on_rbClosedCircuitVoltageGroup_toggled(bool checked)
{
    ui->btnClosedCircuitVoltageGroup->setEnabled(checked);
    ui->cbClosedCircuitVoltageGroup->setEnabled(checked);
    ui->btnClosedCircuitVoltageGroup_2->setEnabled((checked and iStepClosedCircuitVoltageGroup > 1) ? true : false);
}

void MainWindow::on_rbDepassivation_toggled(bool checked)
{
    ui->btnDepassivation->setEnabled(checked);
    ui->cbDepassivation->setEnabled(checked);
    ui->btnDepassivation_2->setEnabled((checked and iStepDepassivation > 1) ? true : false);
}

void MainWindow::on_rbClosedCircuitVoltageBattery_toggled(bool checked)
{
    ui->btnClosedCircuitVoltageBattery->setEnabled(checked);
    ui->cbClosedCircuitVoltageBattery->setEnabled(checked);
    ui->btnClosedCircuitVoltageBattery_2->setEnabled((checked and iStepClosedCircuitVoltageBattery > 1) ? true : false);
}

void MainWindow::on_rbInsulationResistanceMeasuringBoardUUTBB_toggled(bool checked)
{
    ui->btnInsulationResistanceMeasuringBoardUUTBB->setEnabled(checked);
    ui->cbInsulationResistanceMeasuringBoardUUTBB->setEnabled(checked);
    ui->btnInsulationResistanceMeasuringBoardUUTBB_2->setEnabled((checked and iStepInsulationResistanceMeasuringBoardUUTBB > 1) ? true : false);
}

void MainWindow::on_cbIsUUTBB_toggled(bool checked)
{
    if (checked) {
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


// add two new graphs and set their look:
/*customPlot->addGraph();
customPlot->graph(0)->setPen(QPen(Qt::blue)); // line color blue for first graph
customPlot->graph(0)->setBrush(QBrush(QColor(0, 0, 255, 20))); // first graph will be filled with translucent blue
customPlot->addGraph();
customPlot->graph(1)->setPen(QPen(Qt::red)); // line color red for second graph
// generate some points of data (y0 for first, y1 for second graph):
QVector<double> x(250), y0(250), y1(250);
for (int i=0; i<250; ++i)
{
  x[i] = i;
  y0[i] = qExp(-i/150.0)*qCos(i/10.0); // exponentially decaying cosine
  y1[i] = qExp(-i/150.0);              // exponential envelope
}
// configure right and top axis to show ticks but no labels:
// (see QCPAxisRect::setupFullAxesBox for a quicker method to do this)
customPlot->xAxis2->setVisible(true);
customPlot->xAxis2->setTickLabels(false);
customPlot->yAxis2->setVisible(true);
customPlot->yAxis2->setTickLabels(false);
// make left and bottom axes always transfer their ranges to right and top axes:
connect(customPlot->xAxis, SIGNAL(rangeChanged(QCPRange)), customPlot->xAxis2, SLOT(setRange(QCPRange)));
connect(customPlot->yAxis, SIGNAL(rangeChanged(QCPRange)), customPlot->yAxis2, SLOT(setRange(QCPRange)));
// pass data points to graphs:
customPlot->graph(0)->setData(x, y0);
customPlot->graph(1)->setData(x, y1);
// let the ranges scale themselves so graph 0 fits perfectly in the visible area:
customPlot->graph(0)->rescaleAxes();
// same thing for graph 1, but only enlarge ranges (in case graph 1 is smaller than graph 0):
customPlot->graph(1)->rescaleAxes(true);
// Note: we could have also just called customPlot->rescaleAxes(); instead
// Allow user to drag axis ranges with mouse, zoom with mouse wheel and select graphs by clicking:
customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);*/

void MainWindow::on_pushButton_clicked()
{
    int h = 100; //Шаг, с которым будем пробегать по оси Ox
    QVector<double> x(20), y(20); //Массивы координат точек

    ui->widgetClosedCircuitVoltageGroup->clearGraphs();
    for (int i=0; i<20; i++)
    {
        x[i] = h*i;
        y[i] = qrand()%40+10;;

    }
    ui->widgetClosedCircuitVoltageGroup->addGraph();
    ui->widgetClosedCircuitVoltageGroup->graph(0)->setPen(QPen(Qt::green));
    ui->widgetClosedCircuitVoltageGroup->graph(0)->setData(x, y);
    /*for (int i=0; i<20; i++)
    {
        x[i] = h*i;
        y[i] = qrand()%40+10;;

    }
    ui->widgetClosedCircuitVoltageGroup->addGraph();
    ui->widgetClosedCircuitVoltageGroup->graph(1)->setPen(QPen(Qt::blue));
    ui->widgetClosedCircuitVoltageGroup->graph(1)->setData(x, y);
    for (int i=0; i<20; i++)
    {
        x[i] = h*i;
        y[i] = qrand()%40+10;;

    }
    ui->widgetClosedCircuitVoltageGroup->addGraph();
    ui->widgetClosedCircuitVoltageGroup->graph(2)->setPen(QPen(Qt::red));
    ui->widgetClosedCircuitVoltageGroup->graph(2)->setData(x, y);*/

    //ui->widgetClosedCircuitVoltageGroup->clearGraphs();//Если нужно, но очищаем все графики
    //Добавляем один график в widget
    ui->widgetClosedCircuitVoltageGroup->addGraph();
    //ui->widgetClosedCircuitVoltageGroup->graph(0)->setPen(QPen(Qt::blue));
    //ui->widgetClosedCircuitVoltageGroup->graph(0)->setData(x, y);
    ui->widgetClosedCircuitVoltageGroup->xAxis->setLabel(tr("Время, c"));
    ui->widgetClosedCircuitVoltageGroup->yAxis->setLabel(tr("Напряжение, В"));
    ui->widgetClosedCircuitVoltageGroup->yAxis->grid()->setSubGridVisible(true);
    ui->widgetClosedCircuitVoltageGroup->xAxis->grid()->setSubGridVisible(true);
    ui->widgetClosedCircuitVoltageGroup->yAxis->setSubTickCount(10);
    ui->widgetClosedCircuitVoltageGroup->xAxis->setRange(0, 360);
    ui->widgetClosedCircuitVoltageGroup->yAxis->setRange(24, 33); //
    ui->widgetClosedCircuitVoltageGroup->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    ui->widgetClosedCircuitVoltageGroup->axisRect()->setupFullAxesBox();
    //ui->widgetClosedCircuitVoltageGroup->replot();

    /*ui->widgetOpenCircuitVoltageGroup->xAxis->setLabel(tr("Время, c"));
    ui->widgetOpenCircuitVoltageGroup->yAxis->setLabel(tr("Напряжение, В"));

    ui->widgetOpenCircuitVoltageGroup->yAxis->grid()->setSubGridVisible(true);
    ui->widgetOpenCircuitVoltageGroup->xAxis->grid()->setSubGridVisible(true);
    ui->widgetOpenCircuitVoltageGroup->yAxis->setSubTickCount(10);

    // set axes ranges, so we see all data:
    ui->widgetOpenCircuitVoltageGroup->xAxis->setRange(0, 120000/1000);
    ui->widgetOpenCircuitVoltageGroup->yAxis->setRange(24, 33); //

    // make range draggable and zoomable:
    ui->widgetOpenCircuitVoltageGroup->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);

    // make top right axes clones of bottom left axes:
    ui->widgetOpenCircuitVoltageGroup->axisRect()->setupFullAxesBox();
    // connect signals so top and right axes move in sync with bottom and left axes:
    connect(ui->widgetOpenCircuitVoltageGroup->xAxis, SIGNAL(rangeChanged(QCPRange)), ui->widgetOpenCircuitVoltageGroup->xAxis2, SLOT(setRange(QCPRange)));
    connect(ui->widgetOpenCircuitVoltageGroup->yAxis, SIGNAL(rangeChanged(QCPRange)), ui->widgetOpenCircuitVoltageGroup->yAxis2, SLOT(setRange(QCPRange)));*/
}

void MainWindow::on_cbInsulationResistance_currentIndexChanged(const QString &arg1)
{

}

