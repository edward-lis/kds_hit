/* История изменений:
 * 03.02.2016 -
 * Добавил класс для работы с последовательным портом.
 * Добавил конечный автомат (КА).
 * Логика по открытию последовательного порта.
 * Добавлен Режим проверки соответствия типа выбранной оператором батареи и типа реально подключенной батареи.
 * 11.02.2016 - выкинул КА. Реализация через Qt механизм КА слишком объёмная получается.
 */

/* Замечания и план работы:

Где-то на виду сделать большое табло с буквами, в котором писать текущее состояние: типа идёт такая-то проверка.
Потому что в статус-баре нихрена не видно, там пусть останется инфа о состоянии связи,
ну и дублируется инфа из табло.

Сохранение текущего промежуточного состояния проверки, возобновление проверки с места предыдущей остановки сеанса проверки.
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

Settings settings; ///< Установки из ini-файла

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
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

    bDeveloperState = settings.bDeveloperState;

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
    ui->widgetClosedCircuitVoltageGroup->graph(2)->addData(settings.time_closecircuitgroup+1, settings.closecircuitgroup_limit);
    ui->widgetClosedCircuitVoltageGroup->xAxis->setLabel(tr("Время, c"));
    ui->widgetClosedCircuitVoltageGroup->xAxis->setRange(0, settings.time_closecircuitgroup+1);
    ui->widgetClosedCircuitVoltageGroup->yAxis->setLabel(tr("Напряжение, В"));
    ui->widgetClosedCircuitVoltageGroup->yAxis->setRange(24, 33);

    /// описание графика для "Распассивации"
    ui->widgetDepassivation->addGraph(); // blue line
    ui->widgetDepassivation->graph(0)->setPen(QPen(Qt::blue));
    ui->widgetDepassivation->graph(0)->clearData();
    //ui->widgetDepassivation->graph(0)->setName("Ток");
    //ui->widgetDepassivation->addGraph(); // blue dot
    //ui->widgetDepassivation->graph(1)->clearData();
    //ui->widgetDepassivation->graph(1)->setLineStyle(QCPGraph::lsNone);
    //ui->widgetDepassivation->graph(1)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, Qt::blue, Qt::white, 7));
    ui->widgetDepassivation->xAxis->setLabel(tr("Время, c"));
    ui->widgetDepassivation->yAxis->setLabel(tr("Напряжение, В"));
    ui->widgetDepassivation->yAxis->setRange(24, 36);
    //ui->widgetDepassivation->legend->setVisible(true);
    widgetDepassivationTextLabel = new QCPItemText(ui->widgetDepassivation);
    ui->widgetDepassivation->addItem(widgetDepassivationTextLabel);
    widgetDepassivationTextLabel->setPositionAlignment(Qt::AlignTop|Qt::AlignHCenter); // расположение вверху и по центру
    widgetDepassivationTextLabel->position->setType(QCPItemPosition::ptAxisRectRatio);
    widgetDepassivationTextLabel->position->setCoords(0.5, 0); // отступ сверху и от центра
    widgetDepassivationTextLabel->setText(" Ток ");
    widgetDepassivationTextLabel->setFont(QFont(font().family(), 16)); // шрифт и размер
    //widgetDepassivationTextLabel->setPen(QPen(Qt::black)); // черная рамка

    /// описание графика для "Напряжение замкнутой цепи батареи"
    ui->widgetClosedCircuitBattery->addGraph(); // blue line
    ui->widgetClosedCircuitBattery->graph(0)->setPen(QPen(Qt::blue));
    ui->widgetClosedCircuitBattery->graph(0)->clearData();
    ui->widgetClosedCircuitBattery->addGraph(); // blue dot
    ui->widgetClosedCircuitBattery->graph(1)->clearData();
    ui->widgetClosedCircuitBattery->graph(1)->setLineStyle(QCPGraph::lsNone);
    //ui->widgetClosedCircuitVoltagePowerUUTBB->graph(1)->setPen(QPen(Qt::green));
    ui->widgetClosedCircuitBattery->graph(1)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, Qt::blue, Qt::white, 7));
    ui->widgetClosedCircuitBattery->addGraph(); // red line
    ui->widgetClosedCircuitBattery->graph(2)->setPen(QPen(Qt::red));
    ui->widgetClosedCircuitBattery->graph(2)->setBrush(QBrush(QColor(255, 0, 0, 20)));
    ui->widgetClosedCircuitBattery->graph(2)->clearData();
    ui->widgetClosedCircuitBattery->graph(2)->addData(0, settings.closecircuitbattery_limit);
    ui->widgetClosedCircuitBattery->graph(2)->addData(settings.time_closecircuitbattery+2, settings.closecircuitbattery_limit);
    ui->widgetClosedCircuitBattery->xAxis->setLabel(tr("Время, c"));
    ui->widgetClosedCircuitBattery->xAxis->setRange(0, settings.time_closecircuitbattery+2);
    ui->widgetClosedCircuitBattery->yAxis->setLabel(tr("Напряжение, В"));
    ui->widgetClosedCircuitBattery->yAxis->setRange(24, 33);

    bState = false;
    bConnect = true; /// состояние связи (true - есть, false - нет связи)

    iBatteryIndex = 0;
    iPowerState = 0; /// 0 - неизвестное состояние;
    //iStepOpenCircuitVoltageGroup = 0;

    getCOMPorts();
    comboxSetData();

    /// временно скрываем графики
    ui->widgetClosedCircuitBattery->hide();
    ui->widgetClosedCircuitVoltageGroup->hide();
    ui->widgetClosedCircuitVoltagePowerUUTBB->hide();
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


/*!
 * Заполнение комбоксов
 * Вызывается при инициализации и выборе батареи.
 * Входные параметры: int iBatteryIndex
 */
void MainWindow::comboxSetData() {
    ui->cbParamsAutoMode->clear(); /// очищаем комбокс автоматического режима

    /// скрываем все вкладки кроме журнала событий
    int tab_count = ui->tabWidget->count();
    for (int i = 1; i < tab_count; i++) {
        ui->tabWidget->removeTab(1);
    }

    /// очищаем массивы проверок
    dArrayVoltageOnTheHousing.clear();
    dArrayInsulationResistance.clear();
    dArrayOpenCircuitVoltageGroup.clear();
    dArrayOpenCircuitVoltageBattery.clear();
    dArrayClosedCircuitVoltageGroup.clear();
    dArrayDepassivation.clear();
    dArrayClosedCircuitVoltageBattery.clear();
    dArrayInsulationResistanceUUTBB.clear();
    dArrayOpenCircuitVoltagePowerSupply.clear();
    dArrayClosedCircuitVoltagePowerSupply.clear();

    /// заполняем масивы чтобы в дальнейшем можно обратиться к конкретному индексу
    for (int i = 0; i < 35; i++) {
        //iFlagsCircuitGroup.append(-1);
        dArrayVoltageOnTheHousing.append(-1);
        dArrayInsulationResistance.append(-1);
        dArrayOpenCircuitVoltageGroup.append(-1);
        dArrayOpenCircuitVoltageBattery.append(-1);
        dArrayClosedCircuitVoltageGroup.append(-1);
        dArrayDepassivation.append(-1); /// не нужен
        dArrayClosedCircuitVoltageBattery.append(-1);
        dArrayInsulationResistanceUUTBB.append(-1);
        dArrayOpenCircuitVoltagePowerSupply.append(-1);
        dArrayClosedCircuitVoltagePowerSupply.append(-1);
    }

    /// очищаем массив графиков
    imgArrayReportGraph.clear();

    /// 1. Напряжения на корпусе
    ui->cbParamsAutoMode->addItem(tr("1. %0").arg(ui->rbVoltageOnTheHousing->text()));
    ui->cbVoltageOnTheHousing->clear();
    sArrayReport.clear(); /// очищаем массив проверок для отчета
    modelVoltageOnTheHousing = new QStandardItemModel(2, 1);

    /// очистка и заполнение label*ов на вкладке и очистка массива с полученными параметрами проверки
    for (int i = 0; i < 2; i++) {
        label = findChild<QLabel*>(tr("labelVoltageOnTheHousing%0").arg(i));
        label->setStyleSheet("QLabel { color : black; }");
        label->clear();
        item = new QStandardItem(QString("%0").arg(battery[iBatteryIndex].str_voltage_corpus[i]));
        item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        item->setData(Qt::Checked, Qt::CheckStateRole);
        modelVoltageOnTheHousing->setItem(i+1, 0, item);
        label->setText(tr("%0) \"%1\" не измерялось.").arg(i+1).arg(battery[iBatteryIndex].str_voltage_corpus[i]));

    }

    ui->cbVoltageOnTheHousing->setModel(modelVoltageOnTheHousing);
    ui->cbVoltageOnTheHousing->setItemData(0, "DISABLE", Qt::UserRole-1);
    ui->cbVoltageOnTheHousing->setItemText(0, tr("Выбрано: %0 из %1").arg(2).arg(2));
    //ui->cbVoltageOnTheHousing->setCurrentIndex(0);
    connect(modelVoltageOnTheHousing, SIGNAL(itemChanged(QStandardItem*)), this, SLOT(itemChangedVoltageOnTheHousing(QStandardItem*)));


    /// 2. Сопротивление изоляции
    ui->cbParamsAutoMode->addItem(tr("2. %0").arg(ui->rbInsulationResistance->text()));
    ui->cbInsulationResistance->clear();
    modelInsulationResistance = new QStandardItemModel(battery[iBatteryIndex].i_isolation_resistance_num, 1);

    /// проходимся по всем label'ам
    for (int i = 0; i < 4; i++) {
        label = findChild<QLabel*>(tr("labelInsulationResistance%0").arg(i));
        label->setStyleSheet("QLabel { color : black; }");
        label->clear();
        if (i < battery[iBatteryIndex].i_isolation_resistance_num) {
            item = new QStandardItem(QString("%0").arg(battery[iBatteryIndex].str_isolation_resistance[i]));
            item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
            item->setData(Qt::Checked, Qt::CheckStateRole);
            modelInsulationResistance->setItem(i+1, 0, item);
            label->setText(tr("%0) \"%1\" не измерялось.").arg(i+1).arg(battery[iBatteryIndex].str_isolation_resistance[i]));
        }
    }

    ui->cbInsulationResistance->setModel(modelInsulationResistance);
    ui->cbInsulationResistance->setItemData(0, "DISABLE", Qt::UserRole-1);
    ui->cbInsulationResistance->setItemText(0, tr("Выбрано: %0 из %1").arg(battery[iBatteryIndex].i_isolation_resistance_num).arg(battery[iBatteryIndex].i_isolation_resistance_num));
    //ui->cbInsulationResistance->setCurrentIndex(0);
    connect(modelInsulationResistance, SIGNAL(itemChanged(QStandardItem*)), this, SLOT(itemChangedInsulationResistance(QStandardItem*)));

    /// 3. Напряжение разомкнутой цепи группы
    ui->cbParamsAutoMode->addItem(tr("3. %0").arg(ui->rbOpenCircuitVoltageGroup->text()));
    //ui->cbOpenCircuitVoltageGroup->clear();
    modelOpenCircuitVoltageGroup = new QStandardItemModel(battery[iBatteryIndex].group_num, 1);

    /// проходимся по всем label'ам
    for (int i = 0; i < 28; i++) {
        label = findChild<QLabel*>(tr("labelOpenCircuitVoltageGroup%0").arg(i));
        label->setStyleSheet("QLabel { color : black; }");
        label->clear();
        if (i < battery[iBatteryIndex].group_num) {
            item = new QStandardItem(QString("%0").arg(battery[iBatteryIndex].circuitgroup[i]));
            item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
            item->setData(Qt::Checked, Qt::CheckStateRole);
            modelOpenCircuitVoltageGroup->setItem(i+1, 0, item);
            label->setText(tr("%0) \"%1\" не измерялось.").arg(i+1).arg(battery[iBatteryIndex].circuitgroup[i]));
        }
    }

    /// Выбрать все/Отменить все
    /*item = new QStandardItem(QString("Отменить все"));
    item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
    item->setData(Qt::Checked, Qt::CheckStateRole);
    modelOpenCircuitVoltageGroup->setItem(1, 0, item);*/

    ui->cbOpenCircuitVoltageGroup->setModel(modelOpenCircuitVoltageGroup);
    ui->cbOpenCircuitVoltageGroup->setItemData(0, "DISABLE", Qt::UserRole-1);
    ui->cbOpenCircuitVoltageGroup->setItemText(0, tr("Выбрано: %0 из %1").arg(battery[iBatteryIndex].group_num).arg(battery[iBatteryIndex].group_num));
    //ui->cbOpenCircuitVoltageGroup->setCurrentIndex(0);
    connect(modelOpenCircuitVoltageGroup, SIGNAL(itemChanged(QStandardItem*)), this, SLOT(itemChangedOpenCircuitVoltageGroup(QStandardItem*)));

    /// 4. Напряжение разомкнутой цепи батареи
    ui->cbParamsAutoMode->addItem(tr("4. %0").arg(ui->rbOpenCircuitVoltageBattery->text()));
    ui->cbOpenCircuitVoltageBattery->clear();
    ui->cbOpenCircuitVoltageBattery->addItem(battery[iBatteryIndex].circuitbattery);
    ui->labelOpenCircuitVoltageBattery0->setText(tr("%0) \"%1\" не измерялось.").arg(1).arg(battery[iBatteryIndex].circuitbattery));
    ui->labelOpenCircuitVoltageBattery0->setStyleSheet("QLabel { color : black; }");

    /// 5. Напряжение замкнутой цепи группы
    ui->cbParamsAutoMode->addItem(tr("5. %0").arg(ui->rbClosedCircuitVoltageGroup->text()));
    //ui->cbClosedCircuitVoltageGroup->clear();
    modelClosedCircuitVoltageGroup = new QStandardItemModel(battery[iBatteryIndex].group_num, 1);

    /// проходимся по всем label'ам
    for (int i = 0; i < 28; i++) {
        label = findChild<QLabel*>(tr("labelClosedCircuitVoltageGroup%0").arg(i));
        label->setStyleSheet("QLabel { color : black; }");
        label->clear();
        if (i < battery[iBatteryIndex].group_num) {
            item = new QStandardItem(QString("%0").arg(battery[iBatteryIndex].circuitgroup[i]));
            item->setFlags(Qt::NoItemFlags);
            item->setData(Qt::Unchecked, Qt::CheckStateRole);
            modelClosedCircuitVoltageGroup->setItem(i+1, 0, item);
            label->setText(tr("%0) \"%1\" не измерялось.").arg(i+1).arg(battery[iBatteryIndex].circuitgroup[i]));
        }
    }

    ui->cbClosedCircuitVoltageGroup->setModel(modelClosedCircuitVoltageGroup);
    ui->cbClosedCircuitVoltageGroup->setItemData(0, "DISABLE", Qt::UserRole-1);
    ui->cbClosedCircuitVoltageGroup->setItemText(0, tr("Выбрано: %0 из %1").arg(0).arg(battery[iBatteryIndex].group_num));
    //ui->cbClosedCircuitVoltageGroup->setCurrentIndex(0);
    connect(modelClosedCircuitVoltageGroup, SIGNAL(itemChanged(QStandardItem*)), this, SLOT(itemChangedClosedCircuitVoltageGroup(QStandardItem*)));

    /// 6. Распассивация
    modelDepassivation = new QStandardItemModel(battery[iBatteryIndex].group_num, 1);

    /// проходимся по всем label'ам
    for (int i = 0; i < 28; i++) {
        label = findChild<QLabel*>(tr("labelDepassivation%0").arg(i));
        label->setStyleSheet("QLabel { color : black; }");
        label->clear();
        if (i < battery[iBatteryIndex].group_num) {
            item = new QStandardItem(QString("%0").arg(battery[iBatteryIndex].circuitgroup[i]));
            item->setFlags(Qt::NoItemFlags);
            item->setData(Qt::Unchecked, Qt::CheckStateRole);
            modelDepassivation->setItem(i+1, 0, item);
            label->setText(tr("%0) \"%1\" не требуется.").arg(i+1).arg(battery[iBatteryIndex].circuitgroup[i]));
            //label->setStyleSheet("QLabel { color : green; }");
        }
    }

    ui->cbDepassivation->setModel(modelDepassivation);
    ui->cbDepassivation->setItemData(0, "DISABLE", Qt::UserRole-1);
    ui->cbDepassivation->setItemText(0, tr("Выбрано: %0 из %1").arg(0).arg(battery[iBatteryIndex].group_num));
    connect(modelDepassivation, SIGNAL(itemChanged(QStandardItem*)), this, SLOT(itemChangedDepassivation(QStandardItem*)));

    /// 7. Напряжение замкнутой цепи батареи
    ui->cbParamsAutoMode->addItem(tr("7. %0").arg(ui->rbClosedCircuitVoltageBattery->text()));
    ui->cbClosedCircuitVoltageBattery->clear();
    ui->cbClosedCircuitVoltageBattery->addItem(battery[iBatteryIndex].circuitbattery);
    ui->labelClosedCircuitVoltageBattery0->setText(tr("%0) \"%1\" не измерялось.").arg(1).arg(battery[iBatteryIndex].circuitbattery));

    /// только для батарей 9ER20P_20 или 9ER14PS_24
    if (iBatteryIndex == 0 or iBatteryIndex == 1) {
        /// 8. Сопротивление изоляции УУТББ
        if (ui->cbIsUUTBB->isChecked())
            ui->cbParamsAutoMode->addItem(tr("8. %0").arg(ui->rbInsulationResistanceUUTBB->text()));
        ui->cbInsulationResistanceUUTBB->clear();
        modelInsulationResistanceUUTBB = new QStandardItemModel(battery[iBatteryIndex].i_uutbb_resist_num, 1);

        /// проходимся по всем label'ам
        for (int i = 0; i < 33; i++) {
            label = findChild<QLabel*>(tr("labelInsulationResistanceUUTBB%0").arg(i));
            label->setStyleSheet("QLabel { color : black; }");
            label->clear();
            if (i < battery[iBatteryIndex].i_uutbb_resist_num) {
                item = new QStandardItem(QString("%0").arg(battery[iBatteryIndex].uutbb_resist[i]));
                item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
                item->setData(Qt::Checked, Qt::CheckStateRole);
                modelInsulationResistanceUUTBB->setItem(i+1, 0, item);
                label->setText(tr("%0) \"%1\" не измерялось.").arg(i+1).arg(battery[iBatteryIndex].uutbb_resist[i]));
            }
        }

        ui->cbInsulationResistanceUUTBB->setModel(modelInsulationResistanceUUTBB);
        ui->cbInsulationResistanceUUTBB->setItemData(0, "DISABLE", Qt::UserRole-1);
        ui->cbInsulationResistanceUUTBB->setItemText(0, tr("Выбрано: %0 из %1").arg(battery[iBatteryIndex].i_uutbb_resist_num).arg(battery[iBatteryIndex].i_uutbb_resist_num));
        //ui->cbOpenCircuitVoltageGroup->setCurrentIndex(0);
        connect(modelInsulationResistanceUUTBB, SIGNAL(itemChanged(QStandardItem*)), this, SLOT(itemChangedInsulationResistanceUUTBB(QStandardItem*)));

        /// 9. Напряжение разомкнутой цепи БП
        if (ui->cbIsUUTBB->isChecked())
            ui->cbParamsAutoMode->addItem(tr("9. %0").arg(ui->rbOpenCircuitVoltagePowerSupply->text()));
        ui->cbOpenCircuitVoltagePowerSupply->clear();
        ui->cbOpenCircuitVoltagePowerSupply->addItem(battery[iBatteryIndex].uutbb_closecircuitpower[0]);
        ui->labelOpenCircuitVoltagePowerSupply0->setText(tr("1) \"%0\" не измерялось.").arg(battery[iBatteryIndex].uutbb_closecircuitpower[0]));
        ui->labelOpenCircuitVoltagePowerSupply0->setStyleSheet("QLabel { color : black; }");

        /// 10. Напряжение замкнутой цепи БП
        if (ui->cbIsUUTBB->isChecked())
            ui->cbParamsAutoMode->addItem(tr("10. %0").arg(ui->rbClosedCircuitVoltagePowerSupply->text()));
        ui->cbClosedCircuitVoltagePowerSupply->clear();
        ui->cbClosedCircuitVoltagePowerSupply->addItem(battery[iBatteryIndex].uutbb_closecircuitpower[0]);
        ui->labelClosedCircuitVoltagePowerSupply0->setText(tr("1) \"%0\" не измерялось.").arg(battery[iBatteryIndex].uutbb_closecircuitpower[0]));
        ui->labelClosedCircuitVoltagePowerSupply0->setStyleSheet("QLabel { color : black; }");
    }
}

/*!
 * Запись событий в журнал
 */
void MainWindow::Log(QString message, QString color)
{
    dateTime = QDateTime::currentDateTime();
    str = (color == "green" or color == "red" or color == "blue") ? "<font color=\""+color+"\">"+message+"</font>" : message;
    ui->EventLog->appendHtml(tr("%0").arg(dateTime.toString("hh:mm:ss.zzz") + " " + str));
}

// нажата радиокнопка режим Авто
void MainWindow::on_rbModeDiagnosticAuto_toggled(bool checked)
{
    ui->groupBoxCheckParamsAutoMode->setEnabled(checked);
    ui->widgetCheckParamsButtons->setDisabled(checked);
    bModeManual = false;
}

// нажата радиокнопка режим Ручной
void MainWindow::on_rbModeDiagnosticManual_toggled(bool checked)
{
    ui->groupBoxCheckParams->setEnabled(checked);
    ui->widgetCheckParamsButtons->setEnabled(checked);
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
        ui->cbParamsAutoMode->addItem(tr("8. %0").arg(ui->rbInsulationResistanceUUTBB->text()));
        ui->cbParamsAutoMode->addItem(tr("9. %0").arg(ui->rbOpenCircuitVoltagePowerSupply->text()));
        ui->cbParamsAutoMode->addItem(tr("10. %0").arg(ui->rbClosedCircuitVoltagePowerSupply->text()));
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
        ui->cbParamsAutoMode->removeItem(8);
        ui->cbParamsAutoMode->removeItem(7);
        ui->cbParamsAutoMode->removeItem(6);
    }

    /// запрещаем панели
    ui->groupBoxDiagnosticMode->setDisabled(true);
    ui->groupBoxCheckParams->setDisabled(true);
    ui->groupBoxCheckParamsAutoMode->setDisabled(true);
    ui->menuPUTSU->setDisabled(true); /// запрещаем меню ПУ ТСУ
}

void MainWindow::on_comboBoxBatteryList_currentIndexChanged(int index)
{
    if (index == 0 or index == 1) {
        ui->cbIsUUTBB->setEnabled(true);
    } else {
        ui->cbIsUUTBB->setDisabled(true);
        ui->cbIsUUTBB->setChecked(false);
    }
    iBatteryIndex = index;
    comboxSetData();

    /// запрещаем панели
    ui->groupBoxDiagnosticMode->setDisabled(true);
    ui->groupBoxCheckParams->setDisabled(true);
    ui->groupBoxCheckParamsAutoMode->setDisabled(true);
    ui->btnBuildReport->setDisabled(true);
    ui->widgetCheckParamsButtons->setDisabled(true);
    ui->menuPUTSU->setDisabled(true); /// запрещаем меню ПУ ТСУ
}

// нажата кнопка Старт(Стоп) автоматического режима диагностики
void MainWindow::on_btnStartStopAutoModeDiagnostic_clicked()
{
    if(bCheckInProgress) { // если зашли в эту ф-ию по нажатию кнопки btnStartStopAutoModeDiagnostic ("Стоп"), будучи уже в состоянии проверки, значит стоп режима
        // остановить текущую проверку, выход
        bCheckInProgress = false;
        timerSend->stop(); // остановить посылку очередной команды в порт
        timeoutResponse->stop(); // остановить предыдущий таймаут (если был, конечно)
        //qDebug()<<"loop.isRunning()"<<loop.isRunning();
        if(loop.isRunning()) {
            loop.exit(KDS_STOP); // прекратить цикл ожидания посылки/ожидания ответа от коробочки
        }

        bState = false;
        return;
    }

    setGUI(false); /// отключаем интерфейс

    if (!bState) {
        Log("Начало проверки - Автоматический режим", "blue");
        bState = true;

        for (int i = ui->cbParamsAutoMode->currentIndex(); i < ui->cbParamsAutoMode->count(); i++)
        {
            if (!bState) break; // если прожали Стоп выходим из цикла
            switch (i) {
            case 0:
                on_btnVoltageOnTheHousing_clicked();
                break;
            case 1:
                on_btnInsulationResistance_clicked();
                break;
            case 2:
                on_btnOpenCircuitVoltageGroup_clicked();
                break;
            case 3:
                on_btnOpenCircuitVoltageBattery_clicked();
                break;
            case 4:
                on_btnClosedCircuitVoltageGroup_clicked();
                break;
            case 5:
                on_btnClosedCircuitVoltageBattery_clicked();
                break;
            case 6:
                on_btnInsulationResistanceUUTBB_clicked();
                break;
            case 7:
                on_btnOpenCircuitVoltagePowerSupply_clicked();
                break;
            case 8:
                on_btnClosedCircuitVoltagePowerSupply_clicked();
                break;
            default:
                break;
            }
        }
        Log("Проверка завершена - Автоматический режим", "blue");
        QMessageBox::information(this, ui->rbModeDiagnosticAuto->text(), "Проверка завершена!"); // выводим сообщение о завершении проверки
    }

    setGUI(true); /// включаем интерфейс
    bState = false;
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
        /// 4. Напряжение разомкнутой цепи батареи
        ui->cbSubParamsAutoMode->addItem(tr("1. %0").arg(battery[iBatteryIndex].circuitbattery));
        break;
    case 4:
        /// 5. Напряжение замкнутой цепи группы
        for (int r = 0; r < battery[iBatteryIndex].group_num; r++)
            ui->cbSubParamsAutoMode->addItem(tr("%0. %1").arg(r+1).arg(battery[iBatteryIndex].circuitgroup[r]));
        break;
    case 5:
        /// 7. Напряжение замкнутой цепи батареи
        ui->cbSubParamsAutoMode->addItem(tr("1. %0").arg(battery[iBatteryIndex].circuitbattery));
        break;
    case 6:
        /// 8. Сопротивление изоляции УУТББ
        for (int r = 0; r < battery[iBatteryIndex].i_uutbb_resist_num; r++)
            ui->cbSubParamsAutoMode->addItem(tr("%0. %1").arg(r+1).arg(battery[iBatteryIndex].uutbb_resist[r]));
        break;
    case 7:
        /// 9. Напряжение разомкнутой цепи БП
        ui->cbSubParamsAutoMode->addItem(tr("1. %0").arg(battery[iBatteryIndex].uutbb_closecircuitpower[0]));
        break;
    case 8:
        /// 10. Напряжение замкнутой цепи БП
        ui->cbSubParamsAutoMode->addItem(tr("1. %0").arg(battery[iBatteryIndex].uutbb_closecircuitpower[0]));
        ui->cbSubParamsAutoMode->addItem(tr("2. %0").arg(battery[iBatteryIndex].uutbb_closecircuitpower[1]));
        break;
    default:
        break;
    }
}

void MainWindow::triggerDeveloperState() {
    bDeveloperState=!bDeveloperState;
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
        ui->widgetClosedCircuitBattery->show();
        ui->widgetClosedCircuitVoltageGroup->show();
        ui->widgetClosedCircuitVoltagePowerUUTBB->show();
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
        ui->widgetClosedCircuitBattery->hide();
        ui->widgetClosedCircuitVoltageGroup->hide();
        ui->widgetClosedCircuitVoltagePowerUUTBB->hide();

        int tab_count = ui->tabWidget->count();
        for (int i = 1; i < tab_count; i++) {
            ui->tabWidget->removeTab(1);
        }
    }

    for (int i = 0; i < battery[iBatteryIndex].group_num; i++)
    {
        item = new QStandardItem(QString("%0").arg(battery[iBatteryIndex].circuitgroup[i]));
        if(bDeveloperState)
            item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        else
            item->setFlags(Qt::NoItemFlags);
        item->setData(Qt::Unchecked, Qt::CheckStateRole);
        modelDepassivation->setItem(i+1, 0, item);
    }
}


/*!
 * Сброс результатов проверки
 */
void MainWindow::on_actionCheckReset_triggered()
{
    if (!QMessageBox::question(this, tr("КДС ХИТ"), tr(" Вы действительно хотите сбросить результаты проверки? "), tr("Да"), tr("Нет"))) {
        ui->EventLog->clear();
        ui->btnBuildReport->setDisabled(true);
        comboxSetData();
    }
}

/*!
 * Отключение/Включение интерфейса
 */
void MainWindow::setGUI(bool state)
{
    ui->menuBar->setEnabled(state);                                 /// запретить/разрешить верхнее меню
    ui->groupBoxCOMPort->setEnabled(state);                         /// запретить/разрешить группу последовательного порта
    ui->groupBoxDiagnosticDevice->setEnabled(state);                /// запретить/разрешить группу выбора батареи
    ui->groupBoxDiagnosticMode->setEnabled(state);                  /// запретить/разрешить группу выбора режима
    ui->btnBuildReport->setEnabled(state);                          /// запретить/разрешить кнопку формирования отчета
    ((QPushButton*)sender())->setText( (state) ? "Пуск" : "Стоп" ); /// поменять текст на кнопке
    if (ui->rbModeDiagnosticManual->isChecked()) {                  /// если - ручной режим
        ui->groupBoxCheckParams->setEnabled(state);                 ///  запретить/разрешить группу параметров проверки ручного режима
    } else {                                                        /// если - автоматический режим
        ui->cbParamsAutoMode->setEnabled(state);                    ///  запретить/разрешить комбобокс выбора пункта начала автоматического режима
        ui->cbSubParamsAutoMode->setEnabled(state);                 ///  запретить/разрешить комбобокс выбора подпункта начала автоматического режима
    }
}

/*!
 * \brief MainWindow::on_actionPUTSUOn_triggered
 */
void MainWindow::on_actionPUTSUOn_triggered()
{
    qDebug() << "PUTSU Send StubX6#";
    Log("[ПУ ТСУ]: включен.", "green");
}

/*!
 * \brief MainWindow::on_actionPUTSUOff_triggered
 */
void MainWindow::on_actionPUTSUOff_triggered()
{
    qDebug() << "PUTSU Send Idle#";
    Log("[ПУ ТСУ]: отключен.", "red");
}
