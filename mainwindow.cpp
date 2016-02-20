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
    bDeveloperState(true), // признак режим разработчика, временно тру, потом поменять на фолс!!!
    bModeManual(true) // признак ручной режим. менять при выборе радиобаттонов !!!

{
    ui->setupUi(this);

    //+++ Edward
    // загрузить конфигурационные установки и параметры батарей из ini-файла.
    // файл находится в том же каталоге, что и исполняемый.
    settings.loadSettings();

    // по комбинации клавиш Ctrl-R перезагрузить ini-файл настроек settings.loadSettings();
    QShortcut *reloadSettings = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_R), this);
    connect(reloadSettings, SIGNAL(activated()), &settings, SLOT(loadSettings()));
    //reloadSettings->setContext(Qt::ShortcutContext::ApplicationShortcut);

    // по комбинации клавиш Ctrl-D переключить режим разработчика/отладчика
    QShortcut *developMode = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_D), this);
    connect(developMode, SIGNAL(activated()), this, SLOT(triggerDeveloperState()));

    // Добавить в комбобокс наименования батарей, считанных из ини-файла
    for(int i=0; i<settings.num_batteries_types; i++)
    {
        ui->comboBoxBatteryList->addItem(battery[i].str_type_name);
    }

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

    ui->groupBoxDiagnosticDevice->setDisabled(false); // пусть сразу разрешена вся группа. а вот кнопка "проверить тип батареи" разрешится после установления свяязи с коробком
    ui->groupBoxDiagnosticMode->setDisabled(false); //
    ui->groupBoxCheckParams->setDisabled(false);// !!!
    ui->rbInsulationResistanceMeasuringBoardUUTBB->hide();
    ui->cbInsulationResistanceMeasuringBoardUUTBB->hide();
    ui->btnInsulationResistanceMeasuringBoardUUTBB->hide();
    //ui->btnInsulationResistanceMeasuringBoardUUTBB_2->hide();
    ui->rbOpenCircuitVoltagePowerSupply->hide();
    ui->cbOpenCircuitVoltagePowerSupply->hide();
    ui->btnOpenCircuitVoltagePowerSupply->hide();
    //ui->btnOpenCircuitVoltagePowerSupply_2->hide();
    ui->rbClosedCircuitVoltagePowerSupply->hide();
    ui->cbClosedCircuitVoltagePowerSupply->hide();
    ui->btnClosedCircuitVoltagePowerSupply->hide();
    //ui->btnClosedCircuitVoltagePowerSupply_2->hide();

    for (int i = 1; i < 11; i++) {
        ui->tabWidget->removeTab(1);
    }

    bState = false;
    bCheckCompleteVoltageOnTheHousing = false;
    bCheckCompleteInsulationResistance = false;
    bCheckCompleteOpenCircuitVoltageGroup = false;
    bCheckCompleteClosedCircuitVoltageGroup = false;
    bCheckCompleteClosedCircuitVoltageBattery = false;
    bCheckCompleteInsulationResistanceMeasuringBoardUUTBB = false;
    bCheckCompleteOpenCircuitVoltagePowerSupply = false;
    bCheckCompleteClosedCircuitVoltagePowerSupply = false;
    iBatteryIndex = 0;
    //iStep = 0;
    //iAllSteps = 0;
    //iStepVoltageOnTheHousing = 0;
    //iStepInsulationResistance = 0;
    iStepOpenCircuitVoltageGroup = 0;

    getCOMPorts();
    comboxSetData();
    //Ed connect(ui->btnClosedCircuitVoltageGroup, SIGNAL(clicked(bool)), this, SLOT(checkClosedCircuitVoltageGroup()));
    //connect(ui->rbModeDiagnosticAuto, SIGNAL(toggled(bool)), ui->groupBoxCheckParams, SLOT(setDisabled(bool)));
    //connect(ui->rbModeDiagnosticAuto, SIGNAL(toggled(bool)), ui->btnStartAutoModeDiagnostic, SLOT(setEnabled(bool)));
    //connect(ui->rbModeDiagnosticManual, SIGNAL(toggled(bool)), ui->rbVoltageOnTheHousing, SLOT(setEnabled(bool)));
    //connect(ui->rbModeDiagnosticManual, SIGNAL(toggled(bool)), ui->btnPauseAutoModeDiagnostic, SLOT(setDisabled(bool)));
    //connect(ui->rbVoltageOnTheHousing, SIGNAL(toggled(bool)), ui->btnVoltageOnTheHousing, SLOT(setEnabled(bool)));
    //connect(ui->rbVoltageOnTheHousing, SIGNAL(toggled(bool)), ui->cbVoltageOnTheHousing, SLOT(setEnabled(bool)));
    //connect(ui->rbInsulationResistance, SIGNAL(toggled(bool)), ui->btnInsulationResistance, SLOT(setEnabled(bool)));
    /*connect(ui->rbOpenCircuitVoltageGroup, SIGNAL(toggled(bool)), ui->btnOpenCircuitVoltageGroup, SLOT(setEnabled(bool)));
    connect(ui->rbClosedCircuitVoltageGroup, SIGNAL(toggled(bool)), ui->btnClosedCircuitVoltageGroup, SLOT(setEnabled(bool)));
    connect(ui->rbClosedCircuitVoltageBattery, SIGNAL(toggled(bool)), ui->btnClosedCircuitVoltageBattery, SLOT(setEnabled(bool)));
    connect(ui->rbDepassivation, SIGNAL(toggled(bool)), ui->btnDepassivation, SLOT(setEnabled(bool)));
    connect(ui->rbInsulationResistanceMeasuringBoardUUTBB, SIGNAL(toggled(bool)), ui->btnInsulationResistanceMeasuringBoardUUTBB, SLOT(setEnabled(bool)));
    connect(ui->rbOpenCircuitVoltagePowerSupply, SIGNAL(toggled(bool)), ui->btnOpenCircuitVoltagePowerSupply, SLOT(setEnabled(bool)));
    connect(ui->rbClosedCircuitVoltagePowerSupply, SIGNAL(toggled(bool)), ui->btnClosedCircuitVoltagePowerSupply, SLOT(setEnabled(bool)));*/
    //connect(ui->btnVoltageOnTheHousing, SIGNAL(clicked(int)), this, SLOT(checkVoltageOnTheHousing(iBatteryIndex, iStepVoltageOnTheHousing)));
//Ed remove    connect(ui->btnVoltageOnTheHousing, SIGNAL(clicked(bool)), this, SLOT(checkVoltageOnTheHousing()));
    /*connect(ui->btnInsulationResistance, SIGNAL(clicked(bool)), this, SLOT(checkInsulationResistance()));
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
    connect(ui->btnStartAutoModeDiagnostic, SIGNAL(clicked(bool)), this, SLOT(checkAutoModeDiagnostic()));*/
    //connect(ui->btnPauseAutoModeDiagnostic, SIGNAL(clicked(bool)), this, SLOT(setPause()));
    //connect(ui->btnCOMPortDisconnect, SIGNAL(clicked(bool)), ui->btnStartAutoModeDiagnostic, SLOT(setEnabled(bool)));

    /*QStandardItemModel model(3, 1); // 3 rows, 1 col
    for (int r = 0; r < 3; r++)
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

void MainWindow::itemChangedOpenCircuitVoltageGroup(QStandardItem* itm)
{
    qDebug() << "modelOpenCircuitVoltageGroup->rowCount()=" << modelOpenCircuitVoltageGroup->rowCount();
    int count = 0;
    for(int i=1; i < modelOpenCircuitVoltageGroup->rowCount(); i++)
    {
        QStandardItem *sitm = modelOpenCircuitVoltageGroup->item(i, 0);
        Qt::CheckState checkState = sitm->checkState();
        if (checkState == Qt::Checked)
            count++;
    }
    qDebug() << "countOpenCircuitVoltageGroup=" << count;
    ui->cbOpenCircuitVoltageGroup->setItemText(0, tr("Выбрано: %0 из %1").arg(count).arg(modelOpenCircuitVoltageGroup->rowCount()-1));
    ui->cbOpenCircuitVoltageGroup->setCurrentIndex(0);
}

void MainWindow::itemChangedClosedCircuitVoltageGroup(QStandardItem* itm)
{
    qDebug() << "modelClosedCircuitVoltageGroup->rowCount()=" << modelClosedCircuitVoltageGroup->rowCount();
    int count = 0;
    for(int i=1; i < modelClosedCircuitVoltageGroup->rowCount(); i++)
    {
        QStandardItem *sitm = modelClosedCircuitVoltageGroup->item(i, 0);
        Qt::CheckState checkState = sitm->checkState();
        if (checkState == Qt::Checked)
            count++;
    }
    qDebug() << "countClosedCircuitVoltageGroup=" << count;
    ui->cbClosedCircuitVoltageGroup->setItemText(0, tr("Выбрано: %0 из %1").arg(count).arg(modelClosedCircuitVoltageGroup->rowCount()-1));
    ui->cbClosedCircuitVoltageGroup->setCurrentIndex(0);
}

void MainWindow::itemChangedInsulationResistanceMeasuringBoardUUTBB(QStandardItem* itm)
{
    qDebug() << "modelInsulationResistanceMeasuringBoardUUTBB->rowCount()=" << modelInsulationResistanceMeasuringBoardUUTBB->rowCount();
    int count = 0;
    for(int i=1; i < modelInsulationResistanceMeasuringBoardUUTBB->rowCount(); i++)
    {
        QStandardItem *sitm = modelInsulationResistanceMeasuringBoardUUTBB->item(i, 0);
        Qt::CheckState checkState = sitm->checkState();
        if (checkState == Qt::Checked)
            count++;
    }
    qDebug() << "countInsulationResistanceMeasuringBoardUUTBB=" << count;
    ui->cbInsulationResistanceMeasuringBoardUUTBB->setItemText(0, tr("Выбрано: %0 из %1").arg(count).arg(modelInsulationResistanceMeasuringBoardUUTBB->rowCount()-1));
    ui->cbInsulationResistanceMeasuringBoardUUTBB->setCurrentIndex(0);
}

MainWindow::~MainWindow()
{
    delete ui;
}

//+++ Edward ====================================================================================================================
// перегруз крестика закрытия и Alt-F4
void MainWindow::closeEvent(QCloseEvent *event)
 {
    event->accept();
    qApp->quit(); // qApp - это глобальный (доступный из любого места приложения) указатель на объект текущего приложения
    exit(0);
 }

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
            //ui->groupBoxDiagnosticDevice->setEnabled(true); // разрешить комбобокс выбора типа батареи и проверки её подключения
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

double MainWindow::randMToN(double M, double N)
{
    return M + (rand() / ( RAND_MAX / (N-M) ) ) ;
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



/*!
 * Заполнение комбоксов
 * Вызывается при инициализации и выборе батареи.
 * Входные параметры: int iBatteryIndex
 */
void MainWindow::comboxSetData() {
    qDebug() << "comboxSetData()= " << iBatteryIndex;
    ui->cbParamsAutoMode->clear();

    /// 1. Напряжения на корпусе
    ui->cbParamsAutoMode->addItem(tr("1. %0").arg(ui->rbVoltageOnTheHousing->text()));
    ui->cbVoltageOnTheHousing->clear();
    ui->cbVoltageOnTheHousing->addItem(battery[iBatteryIndex].str_voltage_corpus[0]);
    ui->cbVoltageOnTheHousing->addItem(battery[iBatteryIndex].str_voltage_corpus[1]);

    /// 2. Сопротивление изоляции
    ui->cbParamsAutoMode->addItem(tr("2. %0").arg(ui->rbInsulationResistance->text()));
    ui->cbInsulationResistance->clear();
    ui->cbInsulationResistance->addItem(battery[iBatteryIndex].str_isolation_resistance[0]);
    ui->cbInsulationResistance->addItem(battery[iBatteryIndex].str_isolation_resistance[1]);
    if (iBatteryIndex == 0 or iBatteryIndex == 3) { /// еще две пары если батарея 9ER20P_20 или 9ER20P_28
        ui->cbInsulationResistance->addItem(battery[iBatteryIndex].str_isolation_resistance[2]);
        ui->cbInsulationResistance->addItem(battery[iBatteryIndex].str_isolation_resistance[3]);
    }

    /// 3. Напряжение разомкнутой цепи группы
    ui->cbParamsAutoMode->addItem(tr("3. %0").arg(ui->rbOpenCircuitVoltageGroup->text()));
    ui->cbOpenCircuitVoltageGroup->clear();
    modelOpenCircuitVoltageGroup = new QStandardItemModel(battery[iBatteryIndex].group_num, 1);
    for (int r = 0; r < battery[iBatteryIndex].group_num; r++)
    {
        QStandardItem* item;
        item = new QStandardItem(QString("%0").arg(battery[iBatteryIndex].circuitgroup[r]));
        item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        item->setData(Qt::Checked, Qt::CheckStateRole);
        modelOpenCircuitVoltageGroup->setItem(r+1, 0, item);
    }
    ui->cbOpenCircuitVoltageGroup->setModel(modelOpenCircuitVoltageGroup);
    ui->cbOpenCircuitVoltageGroup->setItemData(0, "DISABLE", Qt::UserRole-1);
    ui->cbOpenCircuitVoltageGroup->setItemText(0, tr("Выбрано: %0 из %1").arg(battery[iBatteryIndex].group_num).arg(battery[iBatteryIndex].group_num));
    //ui->cbOpenCircuitVoltageGroup->setCurrentIndex(0);
    connect(modelOpenCircuitVoltageGroup, SIGNAL(itemChanged(QStandardItem*)), this, SLOT(itemChangedOpenCircuitVoltageGroup(QStandardItem*)));

    /// 3а. Напряжение разомкнутой цепи батареи
    ui->cbParamsAutoMode->addItem(tr("3а. %0").arg(ui->rbOpenCircuitVoltageBattery->text()));
    ui->cbOpenCircuitVoltageBattery->clear();
    ui->cbOpenCircuitVoltageBattery->addItem(battery[iBatteryIndex].circuitbattery);

    /// 4. Напряжение замкнутой цепи группы
    ui->cbParamsAutoMode->addItem(tr("4. %0").arg(ui->rbClosedCircuitVoltageGroup->text()));
    ui->cbClosedCircuitVoltageGroup->clear();
    modelClosedCircuitVoltageGroup = new QStandardItemModel(battery[iBatteryIndex].group_num, 1);
    for (int r = 0; r < battery[iBatteryIndex].group_num; r++)
    {
        QStandardItem* item;
        item = new QStandardItem(QString("%0").arg(battery[iBatteryIndex].circuitgroup[r]));
        item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        item->setData(Qt::Checked, Qt::CheckStateRole);
        modelClosedCircuitVoltageGroup->setItem(r+1, 0, item);
    }
    ui->cbClosedCircuitVoltageGroup->setModel(modelClosedCircuitVoltageGroup);
    ui->cbClosedCircuitVoltageGroup->setItemData(0, "DISABLE", Qt::UserRole-1);
    ui->cbClosedCircuitVoltageGroup->setItemText(0, tr("Выбрано: %0 из %1").arg(battery[iBatteryIndex].group_num).arg(battery[iBatteryIndex].group_num));
    //ui->cbClosedCircuitVoltageGroup->setCurrentIndex(0);
    connect(modelClosedCircuitVoltageGroup, SIGNAL(itemChanged(QStandardItem*)), this, SLOT(itemChangedClosedCircuitVoltageGroup(QStandardItem*)));

    /// 4а. Распассивация
    ui->cbDepassivation->clear();

    /// 5. Напряжение замкнутой цепи батареи
    ui->cbParamsAutoMode->addItem(tr("5. %0").arg(ui->rbClosedCircuitVoltageBattery->text()));
    ui->cbClosedCircuitVoltageBattery->clear();
    ui->cbClosedCircuitVoltageBattery->addItem(battery[iBatteryIndex].circuitbattery);

    /// только для батарей 9ER20P_20 или 9ER14PS_24
    if (ui->cbIsUUTBB->isChecked()) {
        /// 6. Сопротивление изоляции УУТББ
        ui->cbParamsAutoMode->addItem(tr("6. %0").arg(ui->rbInsulationResistanceMeasuringBoardUUTBB->text()));
        ui->cbInsulationResistanceMeasuringBoardUUTBB->clear();
        modelInsulationResistanceMeasuringBoardUUTBB = new QStandardItemModel(battery[iBatteryIndex].i_uutbb_resist_num, 1);
        for (int r = 0; r < battery[iBatteryIndex].i_uutbb_resist_num; r++)
        {
            QStandardItem* item;
            item = new QStandardItem(QString("%0").arg(battery[iBatteryIndex].uutbb_resist[r]));
            item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
            item->setData(Qt::Checked, Qt::CheckStateRole);
            modelInsulationResistanceMeasuringBoardUUTBB->setItem(r+1, 0, item);
        }
        ui->cbInsulationResistanceMeasuringBoardUUTBB->setModel(modelInsulationResistanceMeasuringBoardUUTBB);
        ui->cbInsulationResistanceMeasuringBoardUUTBB->setItemData(0, "DISABLE", Qt::UserRole-1);
        ui->cbInsulationResistanceMeasuringBoardUUTBB->setItemText(0, tr("Выбрано: %0 из %1").arg(battery[iBatteryIndex].i_uutbb_resist_num).arg(battery[iBatteryIndex].i_uutbb_resist_num));
        //ui->cbOpenCircuitVoltageGroup->setCurrentIndex(0);
        connect(modelInsulationResistanceMeasuringBoardUUTBB, SIGNAL(itemChanged(QStandardItem*)), this, SLOT(itemChangedInsulationResistanceMeasuringBoardUUTBB(QStandardItem*)));

        /// 7. Напряжение разомкнутой цепи БП
        ui->cbParamsAutoMode->addItem(tr("7. %0").arg(ui->rbOpenCircuitVoltagePowerSupply->text()));
        ui->cbOpenCircuitVoltagePowerSupply->clear();
        ui->cbOpenCircuitVoltagePowerSupply->addItem(battery[iBatteryIndex].uutbb_closecircuitpower[0]);

        /// 8. Напряжение замкнутой цепи БП
        ui->cbParamsAutoMode->addItem(tr("8. %0").arg(ui->rbClosedCircuitVoltagePowerSupply->text()));
        ui->cbClosedCircuitVoltagePowerSupply->clear();
        ui->cbClosedCircuitVoltagePowerSupply->addItem(battery[iBatteryIndex].uutbb_closecircuitpower[0]);
        ui->cbClosedCircuitVoltagePowerSupply->addItem(battery[iBatteryIndex].uutbb_closecircuitpower[1]);
    }
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
 * Напряжение на корпусе батареи
 */
void MainWindow::checkVoltageOnTheHousing()
{
    /*if (((QPushButton*)sender())->objectName() == "btnVoltageOnTheHousing") {
        if (bState) {
            ((QPushButton*)sender())->setText(tr("Стоп"));
        } else {
            ((QPushButton*)sender())->setText(tr("Пуск"));
        }
        iStepVoltageOnTheHousing = 1;
        bState = false;
        ui->btnVoltageOnTheHousing_2->setEnabled(false);
    }*/
    /*if (((QPushButton*)sender())->objectName() == "btnVoltageOnTheHousing_2")
        bState = false;*/
    //if (!bState) return;
    ui->groupBoxCOMPort->setEnabled(false);
    ui->groupBoxDiagnosticDevice->setEnabled(false);
    ui->groupBoxDiagnosticMode->setEnabled(false);
    ui->tabWidget->addTab(ui->tabVoltageOnTheHousing, ui->rbVoltageOnTheHousing->text());
    ui->tabWidget->setCurrentIndex(ui->tabWidget->count()-1);
    Log(tr("Проверка начата - %1").arg(ui->rbVoltageOnTheHousing->text()), "blue");
    iCurrentStep = (ui->rbModeDiagnosticAuto->isChecked()) ? ui->cbStartSubParametrAutoMode->currentIndex() : ui->cbVoltageOnTheHousing->currentIndex();
    iMaxSteps = (ui->rbModeDiagnosticAuto->isChecked()) ? ui->cbStartSubParametrAutoMode->count() : ui->cbVoltageOnTheHousing->count();
    ui->progressBar->setMaximum(iMaxSteps);
    ui->progressBar->setValue(iCurrentStep);
    switch (iBatteryIndex) {
    case 0: //9ER20P-20
        for (int i = iCurrentStep; i < iMaxSteps; i++) {
            if (!bState) return;
            switch (i) {
            case 0:
                delay(1000);
                param = qrand()%3; //число полученное с COM-порта
                break;
            case 1:
                delay(1000);
                param = qrand()%3;; //число полученное с COM-порта
                break;
            default:
                return;
                break;
            }
            str = tr("%0 = <b>%1</b> В").arg(battery[iBatteryIndex].str_voltage_corpus[i]).arg(QString::number(param));
            QLabel * label = findChild<QLabel*>(tr("labelVoltageOnTheHousing%0").arg(i));
            color = (param > settings.voltage_corpus_limit) ? "red" : "green";
            label->setText(str);
            label->setStyleSheet("QLabel { color : "+color+"; }");
            Log(str, color);
            ui->btnBuildReport->setEnabled(true);
            if (param > settings.voltage_corpus_limit) {
                if (QMessageBox::question(this, "Внимание - "+ui->rbVoltageOnTheHousing->text(), tr("%1 \nпродолжить?").arg(str), tr("Да"), tr("Нет"))) {
                    bState = false;
                    return;
                } /*else {
                    ui->rbModeDiagnosticManual->setChecked(true);
                    ui->rbModeDiagnosticAuto->setEnabled(false);
                    ui->btnVoltageOnTheHousing_2->setEnabled(true);
                }*/
            }
            ui->cbStartSubParametrAutoMode->setCurrentIndex(i+1);
            ui->progressBar->setValue(i+1);
            //iStepVoltageOnTheHousing++;
        }
        /*if (ui->rbModeDiagnosticAuto->isChecked())
            bCheckCompleteVoltageOnTheHousing = true;
        return true;*/
        break;
    case 1:
        //if (!bState) return;
        Log("Действия проверки.", "green");
        delay(1000);
        break;
    case 2:
        //if (!bState) return;
        Log("Действия проверки.", "green");
        delay(1000);
        break;
    case 3:
        //if (!bState) return;
        Log("Действия проверки.", "green");
        delay(1000);
        break;
    default:
        break;
    }
    Log(tr("Проверка завершена - %1").arg(ui->rbVoltageOnTheHousing->text()), "blue");
    //iStepVoltageOnTheHousing = 1;
    if(ui->rbModeDiagnosticManual->isChecked()) {
        ui->rbInsulationResistance->setEnabled(true);
        ui->groupBoxCOMPort->setEnabled(true);
        ui->groupBoxDiagnosticDevice->setEnabled(true);
        ui->groupBoxDiagnosticMode->setEnabled(true);
    }
    ui->cbParamsAutoMode->setCurrentIndex(ui->cbParamsAutoMode->currentIndex()+1); // переключаем комбокс на следующий режим
}

/*
 * Сопротивление изоляции
 */
void MainWindow::checkInsulationResistance()
{
    /*if (((QPushButton*)sender())->objectName() == "btnInsulationResistance") {
        iStepInsulationResistance = 1;
        bState = false;
        ui->btnInsulationResistance_2->setEnabled(false);
    }
    if (((QPushButton*)sender())->objectName() == "btnInsulationResistance_2")
        bState = false;*/
    if (!bState) return;
    ui->groupBoxCOMPort->setEnabled(false);
    ui->groupBoxDiagnosticDevice->setEnabled(false);
    ui->groupBoxDiagnosticMode->setEnabled(false);
    ui->tabWidget->addTab(ui->tabInsulationResistance, ui->rbInsulationResistance->text());
    ui->tabWidget->setCurrentIndex(ui->tabWidget->count()-1);
    Log(tr("Проверка начата - %1").arg(ui->rbInsulationResistance->text()), "blue");
    iCurrentStep = (ui->rbModeDiagnosticAuto->isChecked()) ? ui->cbStartSubParametrAutoMode->currentIndex() : ui->cbInsulationResistance->currentIndex();
    iMaxSteps = (ui->rbModeDiagnosticAuto->isChecked()) ? ui->cbStartSubParametrAutoMode->count() : ui->cbInsulationResistance->count();
    ui->progressBar->setMaximum(iMaxSteps);
    ui->progressBar->setValue(iCurrentStep);
    switch (iBatteryIndex) {
    case 0: //9ER20P-20
        for (int i = iCurrentStep; i < iMaxSteps; i++) {
            if (!bState) return;
            switch (i) {
            case 0:
                delay(1000);
                param = qrand()%25; //число полученное с COM-порта
                break;
            case 1:
                delay(1000);
                param = qrand()%25;; //число полученное с COM-порта
                break;
            case 2:
                delay(1000);
                param = qrand()%25; //число полученное с COM-порта
                break;
            case 3:
                delay(1000);
                param = qrand()%25;; //число полученное с COM-порта
                break;
            default:
                return;
                break;
            }
            str = tr("%0 = <b>%1</b> В").arg(battery[iBatteryIndex].str_isolation_resistance[i]).arg(QString::number(param));
            QLabel * label = findChild<QLabel*>(tr("labelInsulationResistance%0").arg(i));
            color = (param > settings.isolation_resistance_limit) ? "red" : "green";
            label->setText(str);
            label->setStyleSheet("QLabel { color : "+color+"; }");
            Log(str, color);
            ui->btnBuildReport->setEnabled(true);
            if (param > settings.isolation_resistance_limit) {
                if (QMessageBox::question(this, "Внимание - "+ui->rbInsulationResistance->text(), tr("%1 \nпродолжить?").arg(str), tr("Да"), tr("Нет"))) {
                    bState = false;
                    return;
                } /*else {
                    ui->rbModeDiagnosticManual->setChecked(true);
                    ui->rbModeDiagnosticAuto->setEnabled(false);
                    ui->btnVoltageOnTheHousing_2->setEnabled(true);
                }*/
            }
            ui->cbStartSubParametrAutoMode->setCurrentIndex(i+1);
            ui->progressBar->setValue(i+1);
            //iStepVoltageOnTheHousing++;
        }
        if (ui->rbModeDiagnosticAuto->isChecked())
             bCheckCompleteInsulationResistance = true;
        break;
    case 1:
        if (!bState) return;
        Log("Действия проверки.", "green");
        delay(1000);
        break;
    case 2:
        if (!bState) return;
        Log("Действия проверки.", "green");
        delay(1000);
        break;
    case 3:
        if (!bState) return;
        Log("Действия проверки.", "green");
        delay(1000);
        break;
    default:
        break;
    }
    Log(tr("Проверка завершена - %1").arg(ui->rbInsulationResistance->text()), "blue");
    //iStepInsulationResistance = 1;
    ui->rbOpenCircuitVoltageGroup->setEnabled(true);
    ui->groupBoxCOMPort->setEnabled(true);
    ui->groupBoxDiagnosticDevice->setEnabled(true);
    ui->groupBoxDiagnosticMode->setEnabled(true);
    ui->cbParamsAutoMode->setCurrentIndex(ui->cbParamsAutoMode->currentIndex()+1); // переключаем комбокс на следующий режим
}

/*
 * Напряжение разомкнутой цепи группы
 */
void MainWindow::checkOpenCircuitVoltageGroup()
{
    /*if (((QPushButton*)sender())->objectName() == "btnOpenCircuitVoltageGroup") {
        iStepOpenCircuitVoltageGroup = 1;
        bState = false;
        ui->btnOpenCircuitVoltageGroup_2->setEnabled(false);
    }
    if (((QPushButton*)sender())->objectName() == "btnOpenCircuitVoltageGroup_2")
        bState = false;*/
    if (!bState) return;
    ui->groupBoxCOMPort->setEnabled(false);
    ui->groupBoxDiagnosticDevice->setEnabled(false);
    ui->groupBoxDiagnosticMode->setEnabled(false);
    ui->tabWidget->addTab(ui->tabOpenCircuitVoltageGroup, ui->rbOpenCircuitVoltageGroup->text());
    ui->tabWidget->setCurrentIndex(ui->tabWidget->count()-1);
    Log(tr("Проверка начата - %1").arg(ui->rbOpenCircuitVoltageGroup->text()), "blue");
    iCurrentStep = (ui->rbModeDiagnosticAuto->isChecked()) ? ui->cbStartSubParametrAutoMode->currentIndex() : ui->cbOpenCircuitVoltageGroup->currentIndex();
    iMaxSteps = (ui->rbModeDiagnosticAuto->isChecked()) ? ui->cbStartSubParametrAutoMode->count() : ui->cbOpenCircuitVoltageGroup->count();
    ui->progressBar->setMaximum(iMaxSteps);
    ui->progressBar->setValue(iCurrentStep);
    switch (iBatteryIndex) {
    case 0: //9ER20P-20
        for (int i = iCurrentStep; i < iMaxSteps; i++) {
            if (!bState) return;
            switch (i) {
            case 0:
                delay(1000);
                param = qrand()%40+10; //число полученное с COM-порта
                break;
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
            default:
                //return;
                break;
            }
            str = tr("%0 = <b>%1</b> В").arg(battery[iBatteryIndex].circuitgroup[i]).arg(QString::number(param));
            QLabel * label = findChild<QLabel*>(tr("labelOpenCircuitVoltageGroup%0").arg(i));
            color = (param > settings.closecircuitgroup_limit) ? "red" : "green";
            label->setText(str);
            label->setStyleSheet("QLabel { color : "+color+"; }");
            Log(str, color);
            ui->btnBuildReport->setEnabled(true);
            if (param > settings.closecircuitgroup_limit) {
                if (QMessageBox::question(this, "Внимание - "+ui->rbOpenCircuitVoltageGroup->text(), tr("%1 \nпродолжить?").arg(str), tr("Да"), tr("Нет"))) {
                    bState = false;
                    return;
                } /*else {
                    ui->rbModeDiagnosticManual->setChecked(true);
                    ui->rbModeDiagnosticAuto->setEnabled(false);
                    ui->btnVoltageOnTheHousing_2->setEnabled(true);
                }*/
            }
            ui->cbStartSubParametrAutoMode->setCurrentIndex(i+1);
            ui->progressBar->setValue(i+1);
            //iStepVoltageOnTheHousing++;
        }
        if (ui->rbModeDiagnosticAuto->isChecked())
             bCheckCompleteInsulationResistance = true;
        break;
    case 1:
        if (!bState) return;
        Log("Действия проверки.", "green");
        delay(1000);
        break;
    case 2:
        if (!bState) return;
        Log("Действия проверки.", "green");
        delay(1000);
        break;
    case 3:
        if (!bState) return;
        Log("Действия проверки.", "green");
        delay(1000);
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
    ui->cbParamsAutoMode->setCurrentIndex(ui->cbParamsAutoMode->currentIndex()+1); // переключаем комбокс на следующий режим
}
/*
 * Напряжение разомкнутой цепи батареи
 */
void MainWindow::checkOpenCircuitVoltageBattery()
{
    qDebug() << "checkOpenCircuitVoltageBattery()";
    ui->cbParamsAutoMode->setCurrentIndex(ui->cbParamsAutoMode->currentIndex()+1); // переключаем комбокс на следующий режим
}

/*
 * Напряжение замкнутой цепи группы
 */
/* Ed void MainWindow::checkClosedCircuitVoltageGroup()
{
    move to closecircuitgroup.cpp
}*/

/*
 * Распассивация
 */
void MainWindow::checkDepassivation()
{
    if (((QPushButton*)sender())->objectName() == "btnDepassivation") {
        iStepDepassivation = 1;
        bState = false;
        //ui->btnDepassivation_2->setEnabled(false);
    }
    if (((QPushButton*)sender())->objectName() == "btnDepassivation_2")
        bState = false;
    if (!bState) return;
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
            if (!bState) return;
            delay(1000);
            Log(tr("%1) между контактом 1 соединителя Х3 «Х3-» и контактом %1 соединителя Х4 «4»").arg(imDepassivation.at(iStepDepassivation-1)), "green");

            iStepDepassivation++;
        }
        break;
    default:
        break;
    }
    Log(tr("Проверка завершена - %1").arg(ui->rbDepassivation->text()), "blue");
    iStepDepassivation = 1;
    //ui->rbDepassivation->setEnabled(false);
    //ui->btnDepassivation_2->setEnabled(false);
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
        bState = false;
        //ui->btnClosedCircuitVoltageBattery_2->setEnabled(false);
    }
    if (((QPushButton*)sender())->objectName() == "btnClosedCircuitVoltageBattery_2")
        bState = false;
    if (!bState) return;
    ui->groupBoxCOMPort->setEnabled(false);
    ui->groupBoxDiagnosticDevice->setEnabled(false);
    ui->groupBoxDiagnosticMode->setEnabled(false);
    ui->tabWidget->addTab(ui->tabClosedCircuitVoltageBattery, ui->rbClosedCircuitVoltageBattery->text());
    ui->tabWidget->setCurrentIndex(ui->tabWidget->count()-1);
    Log(tr("Проверка начата - %1").arg(ui->rbClosedCircuitVoltageBattery->text()), "blue");
    switch (iBatteryIndex) {
    case 0: //9ER20P-20
        if (!bState) return;
        delay(1000);
        ui->labelClosedCircuitVoltageBattery->setText(tr("1) %2").arg(QString::number(param)));
        str = tr("1) между контактом 1 соединителя Х1 «1+» и контактом 1 соединителя Х3 «3-» = <b>%2</b>").arg(QString::number(param));
        Log(str, (param > 30.0) ? "red" : "green");

        if (param > 30.0) {
            ui->rbModeDiagnosticManual->setChecked(true);
            ui->rbModeDiagnosticAuto->setEnabled(false);
            if (QMessageBox::question(this, "Внимание - "+ui->rbClosedCircuitVoltageBattery->text(), tr("%1 \nпродолжить?").arg(str), tr("Да"), tr("Нет"))) {
                //ui->btnClosedCircuitVoltageBattery_2->setEnabled(true);
                bState = true;
                return;
            }
        }
        //ui->btnClosedCircuitVoltageBattery_2->setEnabled(false);
        if (ui->rbModeDiagnosticAuto->isChecked())
            bCheckCompleteClosedCircuitVoltageBattery = true;
        break;
    case 1:
        if (!bState) return;
        Log("Действия проверки.", "green");
        delay(1000);

        break;
    case 2:
        if (!bState) return;
        Log("Действия проверки.", "green");
        delay(1000);

        break;
    case 3:
        if (!bState) return;
        Log("Действия проверки.", "green");
        delay(1000);

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
    if (!bState) return;
    ui->groupBoxCOMPort->setEnabled(false);
    ui->groupBoxDiagnosticDevice->setEnabled(false);
    ui->groupBoxDiagnosticMode->setEnabled(false);
    ui->tabWidget->addTab(ui->tabInsulationResistanceMeasuringBoardUUTBB, ui->rbInsulationResistanceMeasuringBoardUUTBB->text());
    ui->tabWidget->setCurrentIndex(ui->tabWidget->count()-1);
    Log(tr("Проверка начата - %1").arg(ui->rbInsulationResistanceMeasuringBoardUUTBB->text()), "blue");
    switch (iBatteryIndex) {
    case 0: //9ER20P-20
        while (iStepInsulationResistanceMeasuringBoardUUTBB <= 1) {
            if (!bState) return;
            switch (iStepInsulationResistanceMeasuringBoardUUTBB) {
            case 1:
                delay(1000);
                //Log(tr("1) между точкой металлизации и контактом 1 соединителя Х1 «Х1+» = <b>%1</b>").arg(QString::number(paramInsulationResistance1)), color);
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
        if (!bState) return;
        Log("Действия проверки.", "green");
        delay(1000);
        break;
    case 2:
        if (!bState) return;
        Log("Действия проверки.", "green");
        delay(1000);
        break;
    case 3:
        if (!bState) return;
        Log("Действия проверки.", "green");
        delay(1000);
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
    if (!bState) return;
    ui->groupBoxCOMPort->setEnabled(false);
    ui->groupBoxDiagnosticDevice->setEnabled(false);
    ui->groupBoxDiagnosticMode->setEnabled(false);
    ui->tabWidget->addTab(ui->tabOpenCircuitVoltagePowerSupply, ui->rbOpenCircuitVoltagePowerSupply->text());
    ui->tabWidget->setCurrentIndex(ui->tabWidget->count()-1);
    Log(tr("Проверка начата - %1").arg(ui->rbOpenCircuitVoltagePowerSupply->text()), "blue");
    switch (iBatteryIndex) {
    case 0: //9ER20P-20
        while (iStepOpenCircuitVoltagePowerSupply <= 1) {
            if (!bState) return;
            switch (iStepOpenCircuitVoltagePowerSupply) {
            case 1:
                delay(1000);
                //Log(tr("1) между точкой металлизации и контактом 1 соединителя Х1 «Х1+» = <b>%1</b>").arg(QString::number(paramInsulationResistance1)), color);
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
        if (!bState) return;
        Log("Действия проверки.", "green");
        delay(1000);
        break;
    case 2:
        if (!bState) return;
        Log("Действия проверки.", "green");
        delay(1000);
        break;
    case 3:
        if (!bState) return;
        Log("Действия проверки.", "green");
        delay(1000);
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
    if (!bState) return;
    ui->groupBoxCOMPort->setEnabled(false);
    ui->groupBoxDiagnosticDevice->setEnabled(false);
    ui->groupBoxDiagnosticMode->setEnabled(false);
    ui->tabWidget->addTab(ui->tabClosedCircuitVoltagePowerSupply, ui->rbClosedCircuitVoltagePowerSupply->text());
    ui->tabWidget->setCurrentIndex(ui->tabWidget->count()-1);
    Log(tr("Проверка начата - %1").arg(ui->rbClosedCircuitVoltagePowerSupply->text()), "blue");
    switch (iBatteryIndex) {
    case 0: //9ER20P-20
        while (iStepClosedCircuitVoltagePowerSupply <= 1) {
            if (!bState) return;
            switch (iStepClosedCircuitVoltagePowerSupply) {
            case 1:
                delay(1000);
                //Log(tr("1) между точкой металлизации и контактом 1 соединителя Х1 «Х1+» = <b>%1</b>").arg(QString::number(paramInsulationResistance1)), color);
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
        if (!bState) return;
        Log("Действия проверки.", "green");
        delay(1000);
        break;
    case 2:
        if (!bState) return;
        Log("Действия проверки.", "green");
        delay(1000);
        break;
    case 3:
        if (!bState) return;
        Log("Действия проверки.", "green");
        delay(1000);
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
    ui->btnStartStopAutoModeDiagnostic->setEnabled(checked);
}

void MainWindow::on_rbModeDiagnosticManual_toggled(bool checked)
{
    ui->rbVoltageOnTheHousing->setEnabled(checked);
}

void MainWindow::on_rbVoltageOnTheHousing_toggled(bool checked)
{
    ui->btnVoltageOnTheHousing->setEnabled(checked);
    ui->cbVoltageOnTheHousing->setEnabled(checked);
    //ui->btnVoltageOnTheHousing_2->setEnabled((checked and iStepVoltageOnTheHousing > 1) ? true : false);
}

void MainWindow::on_rbInsulationResistance_toggled(bool checked)
{
    ui->btnInsulationResistance->setEnabled(checked);
    ui->cbInsulationResistance->setEnabled(checked);
    //ui->btnInsulationResistance_2->setEnabled((checked and iStepInsulationResistance > 1) ? true : false);
}

void MainWindow::on_rbOpenCircuitVoltageGroup_toggled(bool checked)
{
    ui->btnOpenCircuitVoltageGroup->setEnabled(checked);
    ui->cbOpenCircuitVoltageGroup->setEnabled(checked);
    //ui->btnOpenCircuitVoltageGroup_2->setEnabled((checked and iStepOpenCircuitVoltageGroup > 1) ? true : false);
}

void MainWindow::on_rbClosedCircuitVoltageGroup_toggled(bool checked)
{
    ui->btnClosedCircuitVoltageGroup->setEnabled(checked);
    ui->cbClosedCircuitVoltageGroup->setEnabled(checked);
    //ui->btnClosedCircuitVoltageGroup_2->setEnabled((checked and iStepClosedCircuitVoltageGroup > 1) ? true : false);
}

void MainWindow::on_rbDepassivation_toggled(bool checked)
{
    ui->btnDepassivation->setEnabled(checked);
    ui->cbDepassivation->setEnabled(checked);
    //ui->btnDepassivation_2->setEnabled((checked and iStepDepassivation > 1) ? true : false);
}

void MainWindow::on_rbClosedCircuitVoltageBattery_toggled(bool checked)
{
    ui->btnClosedCircuitVoltageBattery->setEnabled(checked);
    ui->cbClosedCircuitVoltageBattery->setEnabled(checked);
    //ui->btnClosedCircuitVoltageBattery_2->setEnabled((checked and iStepClosedCircuitVoltageBattery > 1) ? true : false);
}

void MainWindow::on_rbInsulationResistanceMeasuringBoardUUTBB_toggled(bool checked)
{
    ui->btnInsulationResistanceMeasuringBoardUUTBB->setEnabled(checked);
    ui->cbInsulationResistanceMeasuringBoardUUTBB->setEnabled(checked);
    //ui->btnInsulationResistanceMeasuringBoardUUTBB_2->setEnabled((checked and iStepInsulationResistanceMeasuringBoardUUTBB > 1) ? true : false);
}

void MainWindow::on_cbIsUUTBB_toggled(bool checked)
{
    if (checked) {
        ui->rbInsulationResistanceMeasuringBoardUUTBB->show();
        ui->cbInsulationResistanceMeasuringBoardUUTBB->show();
        ui->btnInsulationResistanceMeasuringBoardUUTBB->show();
        //ui->btnInsulationResistanceMeasuringBoardUUTBB_2->show();
        ui->rbOpenCircuitVoltagePowerSupply->show();
        ui->cbOpenCircuitVoltagePowerSupply->show();
        ui->btnOpenCircuitVoltagePowerSupply->show();
        //ui->btnOpenCircuitVoltagePowerSupply_2->show();
        ui->rbClosedCircuitVoltagePowerSupply->show();
        ui->cbClosedCircuitVoltagePowerSupply->show();
        ui->btnClosedCircuitVoltagePowerSupply->show();
        //ui->btnClosedCircuitVoltagePowerSupply_2->show();
    } else {
        ui->rbInsulationResistanceMeasuringBoardUUTBB->hide();
        ui->cbInsulationResistanceMeasuringBoardUUTBB->hide();
        ui->btnInsulationResistanceMeasuringBoardUUTBB->hide();
        //ui->btnInsulationResistanceMeasuringBoardUUTBB_2->hide();
        ui->rbOpenCircuitVoltagePowerSupply->hide();
        ui->cbOpenCircuitVoltagePowerSupply->hide();
        ui->btnOpenCircuitVoltagePowerSupply->hide();
        //ui->btnOpenCircuitVoltagePowerSupply_2->hide();
        ui->rbClosedCircuitVoltagePowerSupply->hide();
        ui->cbClosedCircuitVoltagePowerSupply->hide();
        ui->btnClosedCircuitVoltagePowerSupply->hide();
        //ui->btnClosedCircuitVoltagePowerSupply_2->hide();
    }
    comboxSetData();
}


void MainWindow::on_comboBoxBatteryList_currentIndexChanged(int index)
{
    if (index == 0 or index == 1) {
        ui->cbIsUUTBB->setEnabled(true);
    } else {
        ui->cbIsUUTBB->setEnabled(false);
        ui->cbIsUUTBB->setChecked(false);
    }
    iBatteryIndex = index; /// QString::number(index).toInt();
    comboxSetData();
}


void MainWindow::on_btnStartStopAutoModeDiagnostic_clicked()
{
    qDebug() << ((QPushButton*)sender())->objectName();
    if (!bState) {
        Log("Начало проверки - Автоматический режим", "blue");
        bState = true;
        ui->groupBoxCOMPort->setDisabled(bState);
        ui->groupBoxDiagnosticMode->setDisabled(bState);
        ui->groupBoxCheckParams->setDisabled(bState);
        ui->cbParamsAutoMode->setDisabled(bState);
        ui->cbStartSubParametrAutoMode->setDisabled(bState);
        ((QPushButton*)sender())->setText("Стоп");
        ui->cbParamsAutoMode->currentIndex();
        for (int i = ui->cbParamsAutoMode->currentIndex(); i < ui->cbParamsAutoMode->count(); i++) {
            switch (i) {
            case 0:
                checkVoltageOnTheHousing();
                break;
            case 1:
                checkInsulationResistance();
                break;
            case 2:
                checkOpenCircuitVoltageGroup();
                break;
            case 3:
                Log("checkOpenCircuitVoltageBattery()", "blue");
                break;
            case 4:
                Log("checkClosedCircuitVoltageGroup()", "blue");
                break;
            case 5:
                Log("checkClosedCircuitVoltageBattery()", "blue");
                break;
            case 6:
                Log("checkInsulationResistanceMeasuringBoardUUTBB()", "blue");
                break;
            case 7:
                Log("checkOpenCircuitVoltagePowerSupply()", "blue");
                break;
            case 8:
                Log("rbClosedCircuitVoltagePowerSupply()", "blue");
                break;
            default:
                break;
            }
        }
        Log("Проверка завершена - Автоматический режим", "blue");
        bState = false;
        ui->groupBoxCOMPort->setDisabled(bState);
        ui->groupBoxDiagnosticMode->setDisabled(bState);
        ui->groupBoxCheckParams->setDisabled(bState);
        ui->cbParamsAutoMode->setDisabled(bState);
        ui->cbStartSubParametrAutoMode->setDisabled(bState);
        ((QPushButton*)sender())->setText("Старт");
    } else {
        bState = false;
        ui->groupBoxCOMPort->setDisabled(bState);
        ui->groupBoxDiagnosticMode->setDisabled(bState);
        ui->groupBoxCheckParams->setDisabled(bState);
        ui->cbParamsAutoMode->setDisabled(bState);
        ui->cbStartSubParametrAutoMode->setDisabled(bState);
        ((QPushButton*)sender())->setText("Старт");
    }
}

void MainWindow::on_btnContinueAutoModeDiagnostic_clicked()
{
    qDebug() << ((QPushButton*)sender())->objectName();
}

void MainWindow::on_cbParamsAutoMode_currentIndexChanged(int index)
{
    ui->cbStartSubParametrAutoMode->clear();
    switch (index) {
    case 0:
        /// 1. Напряжения на корпусе
        ui->cbStartSubParametrAutoMode->addItem(battery[iBatteryIndex].str_voltage_corpus[0]);
        ui->cbStartSubParametrAutoMode->addItem(battery[iBatteryIndex].str_voltage_corpus[1]);
        break;
    case 1:
        /// 2. Сопротивление изоляции
        ui->cbStartSubParametrAutoMode->addItem(battery[iBatteryIndex].str_isolation_resistance[0]);
        ui->cbStartSubParametrAutoMode->addItem(battery[iBatteryIndex].str_isolation_resistance[1]);
        if (iBatteryIndex == 0 or iBatteryIndex == 3) { /// еще две пары если батарея 9ER20P_20 или 9ER20P_28
            ui->cbStartSubParametrAutoMode->addItem(battery[iBatteryIndex].str_isolation_resistance[2]);
            ui->cbStartSubParametrAutoMode->addItem(battery[iBatteryIndex].str_isolation_resistance[3]);
        }
        break;
    case 2:
        /// 3. Напряжение разомкнутой цепи группы
        for (int r = 0; r < battery[iBatteryIndex].group_num; r++)
            ui->cbStartSubParametrAutoMode->addItem(battery[iBatteryIndex].circuitgroup[r]);
        break;
    case 3:
        /// 3а. Напряжение разомкнутой цепи батареи
        ui->cbStartSubParametrAutoMode->addItem(battery[iBatteryIndex].circuitbattery);
        break;
    case 4:
        /// 4. Напряжение замкнутой цепи группы
        for (int r = 0; r < battery[iBatteryIndex].group_num; r++)
            ui->cbStartSubParametrAutoMode->addItem(battery[iBatteryIndex].circuitgroup[r]);
        break;
    case 5:
        /// 5. Напряжение замкнутой цепи батареи
        ui->cbStartSubParametrAutoMode->addItem(battery[iBatteryIndex].circuitbattery);
        break;
    case 6:
        /// 6. Сопротивление изоляции УУТББ
        for (int r = 0; r < battery[iBatteryIndex].i_uutbb_resist_num; r++)
            ui->cbStartSubParametrAutoMode->addItem(battery[iBatteryIndex].uutbb_resist[r]);
        break;
    case 7:
        /// 7. Напряжение разомкнутой цепи БП
        ui->cbStartSubParametrAutoMode->addItem(battery[iBatteryIndex].uutbb_closecircuitpower[0]);
        break;
    case 8:
        /// 8. Напряжение замкнутой цепи БП
        ui->cbStartSubParametrAutoMode->addItem(battery[iBatteryIndex].uutbb_closecircuitpower[0]);
        ui->cbStartSubParametrAutoMode->addItem(battery[iBatteryIndex].uutbb_closecircuitpower[1]);
        break;
    default:
        break;
    }
}


/*!
 * \brief MainWindow::on_actionSave_triggered
 */
void MainWindow::on_actionSave_triggered()
{
    qDebug() << "on_actionSave_triggered()";
}


/*!
 * \brief MainWindow::on_actionLoad_triggered
 */
void MainWindow::on_actionLoad_triggered()
{
    qDebug() << "on_actionLoad_triggered()";
}


/*!
 * \brief MainWindow::on_actionExit_triggered
 */
void MainWindow::on_actionExit_triggered()
{
    qDebug() << "on_actionExit_triggered()";
}
