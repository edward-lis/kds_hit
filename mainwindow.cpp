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

    ui->groupBoxDiagnosticDevice->setDisabled(false); // пусть сразу разрешена вся группа. а вот кнопка "проверить тип батареи" разрешится после установления связи с коробком
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

    /// удаляем все вкладки кроме первой - журнала событий(0), каждый раз удаляем вторую(1)
    int tab_count = ui->tabWidget->count();
    for (int i = 1; i < tab_count; i++) {
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

MainWindow::~MainWindow()
{
    delete ui;
}

// перегруз крестика закрытия и Alt-F4
void MainWindow::closeEvent(QCloseEvent *event)
 {
    // по умолчанию в месседжбоксе кнопка Нет и по Esc она тоже.
    int ret = QMessageBox::question( this,  tr("КДС ХИТ"), tr(" Вы действительно хотите выйти из программы? "), tr("Да"), tr("Нет"), QString::null, 1, 1);
    if (ret == 1)
        event->ignore();
    else
    {
        event->accept();
        qApp->quit(); // qApp - это глобальный (доступный из любого места приложения) указатель на объект текущего приложения
        exit(0);
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

void MainWindow::on_rbOpenCircuitVoltageBattery_toggled(bool checked)
{
    ui->btnOpenCircuitVoltageBattery->setEnabled(checked);
    ui->cbOpenCircuitVoltageBattery->setEnabled(checked);
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

void MainWindow::on_rbOpenCircuitVoltagePowerSupply_toggled(bool checked)
{
    ui->btnOpenCircuitVoltagePowerSupply->setEnabled(checked);
    ui->cbOpenCircuitVoltagePowerSupply->setEnabled(checked);
}

void MainWindow::on_rbClosedCircuitVoltagePowerSupply_toggled(bool checked)
{
    ui->btnClosedCircuitVoltagePowerSupply->setEnabled(checked);
    ui->cbClosedCircuitVoltagePowerSupply->setEnabled(checked);
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
        ui->cbSubParamsAutoMode->setDisabled(bState);
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
        ui->cbSubParamsAutoMode->setDisabled(bState);
        ((QPushButton*)sender())->setText("Старт");
    } else {
        bState = false;
        ui->groupBoxCOMPort->setDisabled(bState);
        ui->groupBoxDiagnosticMode->setDisabled(bState);
        ui->groupBoxCheckParams->setDisabled(bState);
        ui->cbParamsAutoMode->setDisabled(bState);
        ui->cbSubParamsAutoMode->setDisabled(bState);
        ((QPushButton*)sender())->setText("Старт");
    }
}

void MainWindow::on_cbParamsAutoMode_currentIndexChanged(int index)
{
    ui->cbSubParamsAutoMode->clear();
    switch (index) {
    case 0:
        /// 1. Напряжения на корпусе
        ui->cbSubParamsAutoMode->addItem(tr("1. %0").arg(battery[iBatteryIndex].str_voltage_corpus[0]));
        ui->cbSubParamsAutoMode->addItem(tr("2. %0").arg(battery[iBatteryIndex].str_voltage_corpus[1]));
        break;
    case 1:
        /// 2. Сопротивление изоляции
        ui->cbSubParamsAutoMode->addItem(tr("1. %0").arg(battery[iBatteryIndex].str_isolation_resistance[0]));
        ui->cbSubParamsAutoMode->addItem(tr("2. %0").arg(battery[iBatteryIndex].str_isolation_resistance[1]));
        if (iBatteryIndex == 0 or iBatteryIndex == 3) { /// еще две пары если батарея 9ER20P_20 или 9ER20P_28
            ui->cbSubParamsAutoMode->addItem(tr("3. %0").arg(battery[iBatteryIndex].str_isolation_resistance[2]));
            ui->cbSubParamsAutoMode->addItem(tr("4. %0").arg(battery[iBatteryIndex].str_isolation_resistance[3]));
        }
        break;
    case 2:
        /// 3. Напряжение разомкнутой цепи группы
        for (int r = 0; r < battery[iBatteryIndex].group_num; r++)
            ui->cbSubParamsAutoMode->addItem(tr("%0. %1").arg(r+1).arg(battery[iBatteryIndex].circuitgroup[r]));
        break;
    case 3:
        /// 3а. Напряжение разомкнутой цепи батареи
        ui->cbSubParamsAutoMode->addItem(tr("1. %0").arg(battery[iBatteryIndex].circuitbattery));
        break;
    case 4:
        /// 4. Напряжение замкнутой цепи группы
        for (int r = 0; r < battery[iBatteryIndex].group_num; r++)
            ui->cbSubParamsAutoMode->addItem(tr("%0. %1").arg(r+1).arg(battery[iBatteryIndex].circuitgroup[r]));
        break;
    case 5:
        /// 5. Напряжение замкнутой цепи батареи
        ui->cbSubParamsAutoMode->addItem(tr("1. %0").arg(battery[iBatteryIndex].circuitbattery));
        break;
    case 6:
        /// 6. Сопротивление изоляции УУТББ
        for (int r = 0; r < battery[iBatteryIndex].i_uutbb_resist_num; r++)
            ui->cbSubParamsAutoMode->addItem(tr("%0. %1").arg(r+1).arg(battery[iBatteryIndex].uutbb_resist[r]));
        break;
    case 7:
        /// 7. Напряжение разомкнутой цепи БП
        ui->cbSubParamsAutoMode->addItem(tr("1. %0").arg(battery[iBatteryIndex].uutbb_closecircuitpower[0]));
        break;
    case 8:
        /// 8. Напряжение замкнутой цепи БП
        ui->cbSubParamsAutoMode->addItem(tr("1. %0").arg(battery[iBatteryIndex].uutbb_closecircuitpower[0]));
        ui->cbSubParamsAutoMode->addItem(tr("2. %0").arg(battery[iBatteryIndex].uutbb_closecircuitpower[1]));
        break;
    default:
        break;
    }
}
