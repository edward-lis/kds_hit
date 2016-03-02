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

На вкладках стирать предыдущие или добавлять новые строки с результатами измерения.
Потому что при повторных измерения нихрена не понятно, что измеряется.
При начале режима писать везде: цепь такая-то не измерялась, потом идёт измерение, потом результат.

dArrayOpenCircuitVoltageGroup и другие массивы перетащить в battery[]

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
    settings(0),
    ui(new Ui::MainWindow),
    serialPort(new SerialPort), bPortOpen(false),
    timeoutResponse(NULL), timerPing(NULL), timerSend(NULL),
    loop(0),
    baRecvArray(0),
    baSendArray(0), baSendCommand(0),
    bModeManual(true), // признак ручной режим.
    bCheckInProgress(false) // признак - в состоянии проверки

{
    ui->setupUi(this);

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
    //  Таймер посылки в последовательный порт команды для коробочки, с некоторой задержкой от текущего времени. непериодический
    timerSend = new QTimer;
    timerSend->setSingleShot(true);
    connect(timerSend, SIGNAL(timeout()), this, SLOT(sendSerialData())); // послать baSendArray в порт через некоторое время

    //ui->btnCheckConnectedBattery->setEnabled(false); // по началу работы проверять нечего

    //ui->groupBoxDiagnosticDevice->setDisabled(false); // пусть сразу разрешена вся группа. а вот кнопка "проверить тип батареи" разрешится после установления связи с коробком
    //ui->groupBoxDiagnosticMode->setDisabled(false); //
    //ui->groupBoxCheckParams->setDisabled(false);// !!!
    ui->rbInsulationResistanceUUTBB->hide();
    ui->cbInsulationResistanceUUTBB->hide();
    ui->btnInsulationResistanceUUTBB->hide();
    ui->rbOpenCircuitVoltagePowerSupply->hide();
    ui->cbOpenCircuitVoltagePowerSupply->hide();
    ui->btnOpenCircuitVoltagePowerSupply->hide();
    ui->rbClosedCircuitVoltagePowerSupply->hide();
    ui->cbClosedCircuitVoltagePowerSupply->hide();
    ui->btnClosedCircuitVoltagePowerSupply->hide();

    bModeManual = ui->rbModeDiagnosticManual->isChecked(); // ручной/авто режим

    /// описание графика для "Напряжения замкнутой цепи группы"
    ui->widgetClosedCircuitVoltageGroup->addGraph(); // blue line
    ui->widgetClosedCircuitVoltageGroup->graph(0)->setPen(QPen(Qt::blue));
    ui->widgetClosedCircuitVoltageGroup->graph(0)->clearData();
    ui->widgetClosedCircuitVoltageGroup->addGraph(); // blue dot
    ui->widgetClosedCircuitVoltageGroup->graph(1)->clearData();
    ui->widgetClosedCircuitVoltageGroup->graph(1)->setLineStyle(QCPGraph::lsNone);
    //ui->widgetClosedCircuitVoltageGroup->graph(1)->setPen(QPen(Qt::green));
    ui->widgetClosedCircuitVoltageGroup->graph(1)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, Qt::blue, Qt::white, 7));
    ui->widgetClosedCircuitVoltageGroup->addGraph(); // red line
    ui->widgetClosedCircuitVoltageGroup->graph(2)->setPen(QPen(Qt::red));
    ui->widgetClosedCircuitVoltageGroup->graph(2)->setBrush(QBrush(QColor(255, 0, 0, 20)));
    ui->widgetClosedCircuitVoltageGroup->graph(2)->clearData();
    ui->widgetClosedCircuitVoltageGroup->graph(2)->addData(0, settings.closecircuitgroup_limit);
    ui->widgetClosedCircuitVoltageGroup->graph(2)->addData(settings.time_depassivation[2]+1, settings.closecircuitgroup_limit);
    ui->widgetClosedCircuitVoltageGroup->xAxis->setLabel(tr("Время, c"));
    ui->widgetClosedCircuitVoltageGroup->xAxis->setRange(0, settings.time_depassivation[2]+1);
    ui->widgetClosedCircuitVoltageGroup->yAxis->setLabel(tr("Напряжение, В"));
    ui->widgetClosedCircuitVoltageGroup->yAxis->setRange(24, 33);

    /// описание графика для "Распассивации"
    ui->widgetDepassivation->addGraph(); // blue line
    ui->widgetDepassivation->graph(0)->setPen(QPen(Qt::blue));
    ui->widgetDepassivation->graph(0)->clearData();
    ui->widgetDepassivation->addGraph(); // blue dot
    ui->widgetDepassivation->graph(1)->clearData();
    ui->widgetDepassivation->graph(1)->setLineStyle(QCPGraph::lsNone);
    ui->widgetDepassivation->graph(1)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, Qt::blue, Qt::white, 7));
    ui->widgetDepassivation->xAxis->setLabel(tr("Время, c"));
    ui->widgetDepassivation->yAxis->setLabel(tr("Напряжение, В"));
    ui->widgetDepassivation->yAxis->setRange(24, 33);

    /// удаляем все вкладки кроме первой - журнала событий(0), каждый раз удаляем вторую(1)
    int tab_count = ui->tabWidget->count();
    for (int i = 1; i < tab_count; i++) {
        ui->tabWidget->removeTab(1);
    }

    bState = false;
    /// Заполняем масивы чтобы в дальнейшем можно обратиться к конкретному индексу
    for (int i = 0; i < 35; i++) {
        dArrayVoltageOnTheHousing.append(-1);
        dArrayInsulationResistance.append(-1);
        dArrayOpenCircuitVoltageGroup.append(-1);
        dArrayOpenCircuitVoltageBattery.append(-1);
        dArrayClosedCircuitVoltageGroup.append(-1);
        dArrayDepassivation.append(-1);
        dArrayClosedCircuitVoltageBattery.append(-1);
        dArrayInsulationResistanceUUTBB.append(-1);
        dArrayOpenCircuitVoltagePowerSupply.append(-1);
        dArrayClosedCircuitVoltagePowerSupply.append(-1);
    }

    iBatteryIndex = 0;
    //iStepOpenCircuitVoltageGroup = 0;

    getCOMPorts();
    comboxSetData();

    // состояния виджетов в зависимости от признака отладки
    bDeveloperState = !settings.bDeveloperState;// тут флаг, установленный из конф.файла, инвертируем
    //triggerDeveloperState(); // потому что там внутри ф-ии он инвертируется обратно.
}

MainWindow::~MainWindow()
{
    delete ui;
}

// перегруз события закрытия главного окна (крестик закрытия, Alt-F4 и close())
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
    double val = M + (rand() / ( RAND_MAX / (N-M) ) ) ;
    QString str= QString::number(val, 'f', 1);
    return str.toDouble();
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
    modelDepassivation = new QStandardItemModel(battery[iBatteryIndex].group_num, 1);
    for (int i = 0; i < battery[iBatteryIndex].group_num; i++)
    {
        QStandardItem* item;
        item = new QStandardItem(QString("%0").arg(battery[iBatteryIndex].circuitgroup[i]));
        item->setFlags(Qt::NoItemFlags);
        item->setData(Qt::Unchecked, Qt::CheckStateRole);
        modelDepassivation->setItem(i+1, 0, item);
    }
    ui->cbDepassivation->setModel(modelDepassivation);
    ui->cbDepassivation->setItemData(0, "DISABLE", Qt::UserRole-1);
    ui->cbDepassivation->setItemText(0, tr("Выбрано: %0 из %1").arg(0).arg(battery[iBatteryIndex].group_num));
    connect(modelDepassivation, SIGNAL(itemChanged(QStandardItem*)), this, SLOT(itemChangedDepassivation(QStandardItem*)));

    /// 5. Напряжение замкнутой цепи батареи
    ui->cbParamsAutoMode->addItem(tr("5. %0").arg(ui->rbClosedCircuitVoltageBattery->text()));
    ui->cbClosedCircuitVoltageBattery->clear();
    ui->cbClosedCircuitVoltageBattery->addItem(battery[iBatteryIndex].circuitbattery);

    /// только для батарей 9ER20P_20 или 9ER14PS_24
    if (iBatteryIndex == 0 or iBatteryIndex == 1) {
        /// 6. Сопротивление изоляции УУТББ
        ui->cbInsulationResistanceUUTBB->clear();
        modelInsulationResistanceUUTBB = new QStandardItemModel(battery[iBatteryIndex].i_uutbb_resist_num, 1);
        for (int r = 0; r < battery[iBatteryIndex].i_uutbb_resist_num; r++)
        {
            QStandardItem* item;
            item = new QStandardItem(QString("%0").arg(battery[iBatteryIndex].uutbb_resist[r]));
            item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
            item->setData(Qt::Checked, Qt::CheckStateRole);
            modelInsulationResistanceUUTBB->setItem(r+1, 0, item);
        }
        ui->cbInsulationResistanceUUTBB->setModel(modelInsulationResistanceUUTBB);
        ui->cbInsulationResistanceUUTBB->setItemData(0, "DISABLE", Qt::UserRole-1);
        ui->cbInsulationResistanceUUTBB->setItemText(0, tr("Выбрано: %0 из %1").arg(battery[iBatteryIndex].i_uutbb_resist_num).arg(battery[iBatteryIndex].i_uutbb_resist_num));
        //ui->cbOpenCircuitVoltageGroup->setCurrentIndex(0);
        connect(modelInsulationResistanceUUTBB, SIGNAL(itemChanged(QStandardItem*)), this, SLOT(itemChangedInsulationResistanceUUTBB(QStandardItem*)));

        /// 7. Напряжение разомкнутой цепи БП
        ui->cbOpenCircuitVoltagePowerSupply->clear();
        ui->cbOpenCircuitVoltagePowerSupply->addItem(battery[iBatteryIndex].uutbb_closecircuitpower[0]);

        /// 8. Напряжение замкнутой цепи БП
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

// нажата радиокнопка режим Авто
void MainWindow::on_rbModeDiagnosticAuto_toggled(bool checked)
{
    ui->groupBoxCheckParamsAutoMode->setEnabled(checked);
    bModeManual = false;
}

// нажата радиокнопка режим Ручной
void MainWindow::on_rbModeDiagnosticManual_toggled(bool checked)
{
    ui->groupBoxCheckParams->setEnabled(checked);
    bModeManual = true;
}

void MainWindow::on_rbVoltageOnTheHousing_toggled(bool checked)
{
    ui->btnVoltageOnTheHousing->setEnabled(checked);
    ui->cbVoltageOnTheHousing->setEnabled(checked);
}

void MainWindow::on_rbInsulationResistance_toggled(bool checked)
{
    ui->btnInsulationResistance->setEnabled(checked);
    ui->cbInsulationResistance->setEnabled(checked);
}

void MainWindow::on_rbOpenCircuitVoltageGroup_toggled(bool checked)
{
    ui->btnOpenCircuitVoltageGroup->setEnabled(checked);
    ui->cbOpenCircuitVoltageGroup->setEnabled(checked);
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
}

void MainWindow::on_rbDepassivation_toggled(bool checked)
{
    ui->btnDepassivation->setEnabled(checked);
    ui->cbDepassivation->setEnabled(checked);
}

void MainWindow::on_rbClosedCircuitVoltageBattery_toggled(bool checked)
{
    ui->btnClosedCircuitVoltageBattery->setEnabled(checked);
    ui->cbClosedCircuitVoltageBattery->setEnabled(checked);
}

void MainWindow::on_rbInsulationResistanceUUTBB_toggled(bool checked)
{
    ui->btnInsulationResistanceUUTBB->setEnabled(checked);
    ui->cbInsulationResistanceUUTBB->setEnabled(checked);
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
        ui->rbInsulationResistanceUUTBB->show();
        ui->cbInsulationResistanceUUTBB->show();
        ui->btnInsulationResistanceUUTBB->show();
        ui->rbOpenCircuitVoltagePowerSupply->show();
        ui->cbOpenCircuitVoltagePowerSupply->show();
        ui->btnOpenCircuitVoltagePowerSupply->show();
        ui->rbClosedCircuitVoltagePowerSupply->show();
        ui->cbClosedCircuitVoltagePowerSupply->show();
        ui->btnClosedCircuitVoltagePowerSupply->show();
        ui->tabWidget->addTab(ui->tabInsulationResistanceUUTBB, ui->rbInsulationResistanceUUTBB->text());
        ui->tabWidget->addTab(ui->tabOpenCircuitVoltagePowerSupply, ui->rbOpenCircuitVoltagePowerSupply->text());
        ui->tabWidget->addTab(ui->tabClosedCircuitVoltagePowerSupply, ui->rbClosedCircuitVoltagePowerSupply->text());
        ui->cbParamsAutoMode->addItem(tr("6. %0").arg(ui->rbInsulationResistanceUUTBB->text()));
        ui->cbParamsAutoMode->addItem(tr("7. %0").arg(ui->rbOpenCircuitVoltagePowerSupply->text()));
        ui->cbParamsAutoMode->addItem(tr("8. %0").arg(ui->rbClosedCircuitVoltagePowerSupply->text()));
    } else {
        ui->rbInsulationResistanceUUTBB->hide();
        ui->cbInsulationResistanceUUTBB->hide();
        ui->btnInsulationResistanceUUTBB->hide();
        ui->rbOpenCircuitVoltagePowerSupply->hide();
        ui->cbOpenCircuitVoltagePowerSupply->hide();
        ui->btnOpenCircuitVoltagePowerSupply->hide();
        ui->rbClosedCircuitVoltagePowerSupply->hide();
        ui->cbClosedCircuitVoltagePowerSupply->hide();
        ui->btnClosedCircuitVoltagePowerSupply->hide();
        int tab_count = ui->tabWidget->count();
        for (int i = tab_count; ; i--) {
            if (i < 1)
                break;
            if (ui->tabWidget->tabText(i) == ui->rbInsulationResistanceUUTBB->text() or ui->tabWidget->tabText(i) == ui->rbOpenCircuitVoltagePowerSupply->text() or ui->tabWidget->tabText(i) == ui->rbClosedCircuitVoltagePowerSupply->text()) {
                ui->tabWidget->removeTab(i);
            }
        }
        ui->cbParamsAutoMode->removeItem(8);
        ui->cbParamsAutoMode->removeItem(7);
        ui->cbParamsAutoMode->removeItem(6);
    }
}

void MainWindow::on_comboBoxBatteryList_currentIndexChanged(int index)
{
    if (index == 0 or index == 1) {
        ui->cbIsUUTBB->setEnabled(true);
    } else {
        ui->cbIsUUTBB->setEnabled(false);
        ui->cbIsUUTBB->setChecked(false);
    }
    iBatteryIndex = index;
    comboxSetData();
}

// нажата кнопка Старт(Стоп) автоматического режима диагностики
void MainWindow::on_btnStartStopAutoModeDiagnostic_clicked()
{
    qDebug() << ((QPushButton*)sender())->objectName();

    if(bCheckInProgress) // если зашли в эту ф-ию по нажатию кнопки btnStartStopAutoModeDiagnostic ("Стоп"), будучи уже в состоянии проверки, значит стоп режима
    {
        // остановить текущую проверку, выход
        bCheckInProgress = false;
        timerSend->stop(); // остановить посылку очередной команды в порт
        timeoutResponse->stop(); // остановить предыдущий таймаут (если был, конечно)
        //qDebug()<<"loop.isRunning()"<<loop.isRunning();
        if(loop.isRunning())
        {
            loop.exit(KDS_STOP); // прекратить цикл ожидания посылки/ожидания ответа от коробочки
        }
        bState = false;

        return;
    }

    if (!bState) {
        Log("Начало проверки - Автоматический режим", "blue");
        bState = true;

        ui->groupBoxCOMPort->setDisabled(bState); // запрещаем комбобокс COM-порта
        ui->groupBoxDiagnosticMode->setDisabled(bState); // запрещаем бокс выбора режима диагностики
        ui->cbParamsAutoMode->setDisabled(bState); // запрещаем бокс выбора начального параметра проверки автоматического режима
        ui->cbSubParamsAutoMode->setDisabled(bState); // запрещаем бокс выбора начального под-параметра проверки автоматического режима
        ((QPushButton*)sender())->setText("Стоп");

        for (int i = ui->cbParamsAutoMode->currentIndex(); i < ui->cbParamsAutoMode->count(); i++) {
            if (!bState) break; // если прожали Стоп выходим из цикла
            switch (i) {
            case 0:
                //checkVoltageOnTheHousing();
                on_btnVoltageOnTheHousing_clicked();
                break;
            case 1:
                //checkInsulationResistance();
                //Log("checkInsulationResistance()", "blue");
                on_btnInsulationResistance_clicked();
                break;
            case 2:
                //checkOpenCircuitVoltageGroup();
                //Log("checkOpenCircuitVoltageGroup()", "blue");
                on_btnOpenCircuitVoltageGroup_clicked();
                break;
            case 3:
                //checkOpenCircuitVoltageBattery();
                on_btnOpenCircuitVoltageBattery_clicked();
                break;
            case 4:
                //checkClosedCircuitVoltageGroup();
                on_btnClosedCircuitVoltageGroup_clicked();
                break;
            case 5:
                checkClosedCircuitVoltageBattery();
                break;
            case 6:
                checkInsulationResistanceUUTBB();
                break;
            case 7:
                checkOpenCircuitVoltagePowerSupply();
                break;
            case 8:
                checkClosedCircuitVoltagePowerSupply();
                break;
            default:
                break;
            }
        }
        Log("Проверка завершена - Автоматический режим", "blue");
        QMessageBox::information(this, ui->rbModeDiagnosticAuto->text(), "Проверка завершена!"); // выводим сообщение о завершении проверки
    }

    bState = false;
    ui->groupBoxCOMPort->setDisabled(bState); // разрешаем комбобокс COM-порта
    ui->groupBoxDiagnosticMode->setDisabled(bState); // разрешаем бокс выбора режима диагностики
    ui->cbParamsAutoMode->setDisabled(bState); // разрешаем бокс выбора начального параметра проверки автоматического режима
    ui->cbSubParamsAutoMode->setDisabled(bState); // разрешаем бокс выбора начального под-параметра проверки автоматического режима
    ((QPushButton*)sender())->setText("Пуск");
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

void MainWindow::triggerDeveloperState() {
    bDeveloperState=!bDeveloperState;
    //qDebug() << "bDeveloperState =" << bDeveloperState;
    ui->btnCheckConnectedBattery->setEnabled(bDeveloperState);
    ui->groupBoxDiagnosticDevice->setEnabled(bDeveloperState);
    ui->groupBoxDiagnosticMode->setEnabled(bDeveloperState);
    ui->groupBoxCheckParams->setEnabled(bDeveloperState);
    ui->rbVoltageOnTheHousing->setEnabled(bDeveloperState);
    ui->rbInsulationResistance->setEnabled(bDeveloperState);
    ui->rbOpenCircuitVoltageGroup->setEnabled(bDeveloperState);
    ui->rbOpenCircuitVoltageBattery->setEnabled(bDeveloperState);
    ui->rbClosedCircuitVoltageGroup->setEnabled(bDeveloperState);
    ui->rbDepassivation->setEnabled(bDeveloperState);
    ui->rbClosedCircuitVoltageBattery->setEnabled(bDeveloperState);
    ui->rbInsulationResistanceUUTBB->setEnabled(bDeveloperState);
    ui->rbOpenCircuitVoltagePowerSupply->setEnabled(bDeveloperState);
    ui->rbClosedCircuitVoltagePowerSupply->setEnabled(bDeveloperState);
    ui->cbParamsAutoMode->setEnabled(bDeveloperState);
    ui->cbSubParamsAutoMode->setEnabled(bDeveloperState);
    ui->btnStartStopAutoModeDiagnostic->setEnabled(bDeveloperState);
    ui->btnBuildReport->setEnabled(bDeveloperState);
    if (bDeveloperState) {
        ui->tabWidget->addTab(ui->tabVoltageOnTheHousing, ui->rbVoltageOnTheHousing->text());
        ui->tabWidget->addTab(ui->tabInsulationResistance, ui->rbInsulationResistance->text());
        ui->tabWidget->addTab(ui->tabOpenCircuitVoltageGroup, ui->rbOpenCircuitVoltageGroup->text());
        ui->tabWidget->addTab(ui->tabOpenCircuitVoltageBattery, ui->rbOpenCircuitVoltageBattery->text());
        ui->tabWidget->addTab(ui->tabClosedCircuitVoltageGroup, ui->rbClosedCircuitVoltageGroup->text());
        ui->tabWidget->addTab(ui->tabDepassivation, ui->rbDepassivation->text());
        ui->tabWidget->addTab(ui->tabClosedCircuitVoltageBattery, ui->rbClosedCircuitVoltageBattery->text());
        ui->tabWidget->addTab(ui->tabInsulationResistanceUUTBB, ui->rbInsulationResistanceUUTBB->text());
        ui->tabWidget->addTab(ui->tabOpenCircuitVoltagePowerSupply, ui->rbOpenCircuitVoltagePowerSupply->text());
        ui->tabWidget->addTab(ui->tabClosedCircuitVoltagePowerSupply, ui->rbClosedCircuitVoltagePowerSupply->text());

        ui->rbInsulationResistanceUUTBB->show();
        ui->cbInsulationResistanceUUTBB->show();
        ui->btnInsulationResistanceUUTBB->show();
        ui->rbOpenCircuitVoltagePowerSupply->show();
        ui->cbOpenCircuitVoltagePowerSupply->show();
        ui->btnOpenCircuitVoltagePowerSupply->show();
        ui->rbClosedCircuitVoltagePowerSupply->show();
        ui->cbClosedCircuitVoltagePowerSupply->show();
        ui->btnClosedCircuitVoltagePowerSupply->show();
    } else {
        ui->rbInsulationResistanceUUTBB->hide();
        ui->cbInsulationResistanceUUTBB->hide();
        ui->btnInsulationResistanceUUTBB->hide();
        ui->rbOpenCircuitVoltagePowerSupply->hide();
        ui->cbOpenCircuitVoltagePowerSupply->hide();
        ui->btnOpenCircuitVoltagePowerSupply->hide();
        ui->rbClosedCircuitVoltagePowerSupply->hide();
        ui->cbClosedCircuitVoltagePowerSupply->hide();
        ui->btnClosedCircuitVoltagePowerSupply->hide();

        int tab_count = ui->tabWidget->count();
        for (int i = 1; i < tab_count; i++) {
            ui->tabWidget->removeTab(1);
        }
    }

    for (int i = 0; i < battery[iBatteryIndex].group_num; i++)
    {
        QStandardItem* item;
        item = new QStandardItem(QString("%0").arg(battery[iBatteryIndex].circuitgroup[i]));
        if(bDeveloperState)
            item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        else
            item->setFlags(Qt::NoItemFlags);
        item->setData(Qt::Unchecked, Qt::CheckStateRole);
        modelDepassivation->setItem(i+1, 0, item);
    }
}

