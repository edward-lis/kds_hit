// точки измерения параметров имитатора батареи - уточнить в ини-файле
// при старте заблокировать выбор батареи. добавить в кдс ф-ию инициализации батареи.

// сколько измерений изоляции УСХТИЛБ ? что значит поочерёдно?
//..!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//в последовательности проверок в приложении В отсутствует Напряжение разомкнутой цепи батареи (НРЦ) – UБНРЦ;

//  Если значение НЗЦ батареи более 30,0 В, то проверка продолжается в автоматическом режиме. - чо это значит в случае, когда это последняя проверка?

// подумать над остановкой автоматической проверки в любой точке циклограммы.
// сохранение циклограммы, возобновление циклограммы с точки останова.
// соответственно сохранение/возобовление состояния органов и контролов. отчёт и графики?
// отчёт формировать на основе списка - лога действий.  объект - события, время, рез-т


/*!
 * Описание:
 * В ини-файле параметры конкретных батарей.
 * В зависимости от типа батареи - разный интерфейс в части кол-ва цепей и точек измерения параметров
 * Связь по последовательному порту происходит через объект Kds. Который, в свою очередь, использует объект comportwidget.
 * При нахождении в режиме главного окна периодически посылается пинг. При переключении в какой-нибудь режим диагностики
 * пинг в главном окне отключается.
 * После первого пинга после отсутствия связи коробочка возвращает свой номер, который надо себе запомнить.
 *
 * Ctrl-R - перечитывает конфигурационный файл kds_hit.ini, который находится в том же каталоге, что и kds_hit.exe
 *
 * Глоссарий:
 * УУТББ - Унифицированное Устройство Телеметрии Бортовых Батарей
 * УСХТИЛБ - плата измерительная УУТББ
 *
 *
 *
*/

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QCloseEvent>
#include <QDebug>
#include <QMessageBox>
#include <QStringList>
#include <QTimer>
#include <QByteArray>
#include <QFileDialog>
#include <QTextDocumentWriter>
#include <QShortcut>
#include <QKeySequence>
#include <QMessageBox>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "battery.h"
#include "kds.h"

///
/// \brief параметры конкретных типов батарей
///
Battery simulator, b_9ER20P_20, b_9ER20P_20_v2, b_9ER14PS_24, b_9ER14PS_24_v2, b_9ER20P_28, b_9ER14P_24;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    settings(NULL),
    kds(0)
{
    ui->setupUi(this);

    // загрузить конфигурационные установки и параметры батарей из ini-файла.
    // файл находится в том же каталоге, что и исполняемый.
    settings.loadSettings();

    // по комбинации клавиш Ctrl-R перезагрузить ini-файл настроек settings.loadSettings();
    QShortcut *reloadSettings = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_R), this);
    connect(reloadSettings, SIGNAL(activated()), &settings, SLOT(loadSettings()));
    //reloadSettings->setContext(Qt::ShortcutContext::ApplicationShortcut);

    // Добавление ярлыка в строку статуса
    // create objects for the label
    statusLabel = new QLabel(this);
    // set text for the label
    statusLabel->setText("Status Label");
    ui->statusBar->addPermanentWidget(statusLabel,1);

    initKds(); // первоначальная инициализация объекта КДС
    // сигнал - передаём данные в объект кдс, который в свою очередь передаст в ком-порт
    connect(this, SIGNAL(sendSerialData(quint8, QByteArray)), this->kds, SLOT(send_request(quint8, QByteArray)));
    // по сигналу готовности данных примем их
    connect(this->kds, SIGNAL(sendSerialReceivedData(quint8, QByteArray)), this, SLOT(getSerialDataReceived(quint8, QByteArray)));

    //timer
    timer = new QTimer(this);
    timer->setInterval(500); // запуск периодического таймера в полсекунды
    connect(this->timer, SIGNAL(timeout()), this, SLOT(procPeriodicTimer()));
    timer->start();

    ping = true;
    //alarm=false;

    // для отладки уберём (!!!) на главном экране группа показа напряжения разомкнутой цепи батареи
    //ui->groupBox_OpenBat->setVisible(false);
    // при выборе имитатора или вторых версий двух батарей - подсветить дополнительные радиокнопки выбора проверок.
    connect(ui->radioButton_Simulator, SIGNAL(toggled(bool)), ui->radioButton_InsulateUUTB, SLOT(setEnabled(bool)));
    connect(ui->radioButton_Simulator, SIGNAL(toggled(bool)), ui->radioButton_OpenCircuitPowerUnit, SLOT(setEnabled(bool)));
    connect(ui->radioButton_Simulator, SIGNAL(toggled(bool)), ui->radioButton_ClosedCircuitPowerUnit, SLOT(setEnabled(bool)));
    connect(ui->radioButton_Battery_9ER20P_20_v2, SIGNAL(toggled(bool)), ui->radioButton_InsulateUUTB, SLOT(setEnabled(bool)));
    connect(ui->radioButton_Battery_9ER20P_20_v2, SIGNAL(toggled(bool)), ui->radioButton_OpenCircuitPowerUnit, SLOT(setEnabled(bool)));
    connect(ui->radioButton_Battery_9ER20P_20_v2, SIGNAL(toggled(bool)), ui->radioButton_ClosedCircuitPowerUnit, SLOT(setEnabled(bool)));
    connect(ui->radioButton_Battery_9ER14PS_24_v2, SIGNAL(toggled(bool)), ui->radioButton_InsulateUUTB, SLOT(setEnabled(bool)));
    connect(ui->radioButton_Battery_9ER14PS_24_v2, SIGNAL(toggled(bool)), ui->radioButton_OpenCircuitPowerUnit, SLOT(setEnabled(bool)));
    connect(ui->radioButton_Battery_9ER14PS_24_v2, SIGNAL(toggled(bool)), ui->radioButton_ClosedCircuitPowerUnit, SLOT(setEnabled(bool)));


    //спрятать на формате лишние электро-цепи
    connect(ui->radioButton_Simulator, SIGNAL(clicked(bool)), this, SLOT(click_radioButton_Simulator()));
    connect(ui->radioButton_Battery_9ER20P_20, SIGNAL(clicked(bool)), this, SLOT(click_radioButton_Battery_9ER20P_20()));
    connect(ui->radioButton_Battery_9ER20P_20_v2, SIGNAL(clicked(bool)), this, SLOT(click_radioButton_Battery_9ER20P_20_v2()));
    connect(ui->radioButton_Battery_9ER14PS_24, SIGNAL(clicked(bool)), this, SLOT(click_radioButton_Battery_9ER14PS_24()));
    connect(ui->radioButton_Battery_9ER14PS_24_v2, SIGNAL(clicked(bool)), this, SLOT(click_radioButton_Battery_9ER14PS_24_v2()));
    connect(ui->radioButton_Battery_9ER14P_24, SIGNAL(clicked(bool)), this, SLOT(click_radioButton_Battery_9ER14P_24()));
    connect(ui->radioButton_Battery_9ER20P_28, SIGNAL(clicked(bool)), this, SLOT(click_radioButton_Battery_9ER20P_28()));

    emit ui->radioButton_Simulator->click(); // кликнем радио-кнопкой, чтобы автоматом перерисовался интерфейс

    /*
    setWindowTitle(tr("Hello world!!!"));
    setMinimumSize(200, 80);

    QLabel * plb = new QLabel(tr("Test"), this);
    plb->setGeometry(20, 20, 80, 24);

    QLineEdit * ple = new QLineEdit(this);
    ple->setGeometry(110, 20, 80, 24);

    QPushButton * ppb = new QPushButton(tr("Ok"), this);
    ppb->setGeometry(20, 50, 80, 24);

    //=========
    QPushButton *btn = new QPushButton(this);
        QVBoxLayout *vbox = new QVBoxLayout(this);
        vbox->addWidget(btn);
        connect(btn, SIGNAL(clicked()), this, SLOT(pressbutton()));*/
}
// тестовая ф-ия, нахрен не нужна
void MainWindow::pressbutton()
{
    QLineEdit *le = new QLineEdit(this);
    le->setText("Hello");
    le->show();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::initKds()
{
    if(!this->kds)
    {
        this->kds = new Kds();
    }
    if(this->kds)
    {
/* !!!        this->kds->accum_type = UNDEFINED;
        //qDebug() << this->kds->accum_type;
        kds->battery_variant=BT_000;
        kds->battery_date="________";
        kds->battery_num="_________";*/
    }
}

void MainWindow::procPeriodicTimer()
{
    QDateTime dt = QDateTime::currentDateTime();
    if(ping) // если режим пинга (простоя)
    {
        if(!kds->online) // если нет связи, то
        {
            //statusLabel->setText(dt.toString("hh:mm:ss"));
            statusLabel->setText("Нет связи"); // напишем нет связи
            firstping = true;
        }
        sendSerialData(0x01, "PING"); // пошлём пинг
        kds->online = false; // сбросим флаг онлайна

    }
}
void MainWindow::click_radioButton_Battery_9ER14PS_24()
{
    ui->label_CloseGroup_28->hide();
    ui->label_CloseGroup_27->hide();
    ui->label_CloseGroup_26->hide();
    ui->label_CloseGroup_25->hide();
    ui->label_CloseGroup_24->show();
    ui->label_CloseGroup_23->show();
    ui->label_CloseGroup_22->show();
    ui->label_CloseGroup_21->show();

    ui->label_OpenGroup_28->hide();
    ui->label_OpenGroup_27->hide();
    ui->label_OpenGroup_26->hide();
    ui->label_OpenGroup_25->hide();
    ui->label_OpenGroup_24->show();
    ui->label_OpenGroup_23->show();
    ui->label_OpenGroup_22->show();
    ui->label_OpenGroup_21->show();

    ui->label_CorpusVoltage_1->setText(b_9ER14PS_24.str_voltage_corpus_1);
    ui->label_CorpusVoltage_2->setText(b_9ER14PS_24.str_voltage_corpus_2);
    ui->label_IsolationResistance_1->setText(b_9ER14PS_24.str_isolation_resistance_1);
    ui->label_IsolationResistance_2->setText(b_9ER14PS_24.str_isolation_resistance_2);
    ui->label_IsolationResistance_3->setText(b_9ER14PS_24.str_isolation_resistance_3);
    ui->label_IsolationResistance_4->setText(b_9ER14PS_24.str_isolation_resistance_4);

    ui->label_OpenGroup_1->setText(b_9ER14PS_24.opencircuitgroup_1);
    ui->label_OpenGroup_2->setText(b_9ER14PS_24.opencircuitgroup_2);
    ui->label_OpenGroup_3->setText(b_9ER14PS_24.opencircuitgroup_3);
    ui->label_OpenGroup_4->setText(b_9ER14PS_24.opencircuitgroup_4);
    ui->label_OpenGroup_5->setText(b_9ER14PS_24.opencircuitgroup_5);
    ui->label_OpenGroup_6->setText(b_9ER14PS_24.opencircuitgroup_6);
    ui->label_OpenGroup_7->setText(b_9ER14PS_24.opencircuitgroup_7);
    ui->label_OpenGroup_8->setText(b_9ER14PS_24.opencircuitgroup_8);
    ui->label_OpenGroup_9->setText(b_9ER14PS_24.opencircuitgroup_9);
    ui->label_OpenGroup_10->setText(b_9ER14PS_24.opencircuitgroup_10);
    ui->label_OpenGroup_11->setText(b_9ER14PS_24.opencircuitgroup_11);
    ui->label_OpenGroup_12->setText(b_9ER14PS_24.opencircuitgroup_12);
    ui->label_OpenGroup_13->setText(b_9ER14PS_24.opencircuitgroup_13);
    ui->label_OpenGroup_14->setText(b_9ER14PS_24.opencircuitgroup_14);
    ui->label_OpenGroup_15->setText(b_9ER14PS_24.opencircuitgroup_15);
    ui->label_OpenGroup_16->setText(b_9ER14PS_24.opencircuitgroup_16);
    ui->label_OpenGroup_17->setText(b_9ER14PS_24.opencircuitgroup_17);
    ui->label_OpenGroup_18->setText(b_9ER14PS_24.opencircuitgroup_18);
    ui->label_OpenGroup_19->setText(b_9ER14PS_24.opencircuitgroup_19);
    ui->label_OpenGroup_20->setText(b_9ER14PS_24.opencircuitgroup_20);
    ui->label_OpenGroup_21->setText(b_9ER14PS_24.opencircuitgroup_21);
    ui->label_OpenGroup_22->setText(b_9ER14PS_24.opencircuitgroup_22);
    ui->label_OpenGroup_23->setText(b_9ER14PS_24.opencircuitgroup_23);
    ui->label_OpenGroup_24->setText(b_9ER14PS_24.opencircuitgroup_24);

    ui->label_CloseGroup_1->setText(b_9ER14PS_24.opencircuitgroup_1);
    ui->label_CloseGroup_2->setText(b_9ER14PS_24.opencircuitgroup_2);
    ui->label_CloseGroup_3->setText(b_9ER14PS_24.opencircuitgroup_3);
    ui->label_CloseGroup_4->setText(b_9ER14PS_24.opencircuitgroup_4);
    ui->label_CloseGroup_5->setText(b_9ER14PS_24.opencircuitgroup_5);
    ui->label_CloseGroup_6->setText(b_9ER14PS_24.opencircuitgroup_6);
    ui->label_CloseGroup_7->setText(b_9ER14PS_24.opencircuitgroup_7);
    ui->label_CloseGroup_8->setText(b_9ER14PS_24.opencircuitgroup_8);
    ui->label_CloseGroup_9->setText(b_9ER14PS_24.opencircuitgroup_9);
    ui->label_CloseGroup_10->setText(b_9ER14PS_24.opencircuitgroup_10);
    ui->label_CloseGroup_11->setText(b_9ER14PS_24.opencircuitgroup_11);
    ui->label_CloseGroup_12->setText(b_9ER14PS_24.opencircuitgroup_12);
    ui->label_CloseGroup_13->setText(b_9ER14PS_24.opencircuitgroup_13);
    ui->label_CloseGroup_14->setText(b_9ER14PS_24.opencircuitgroup_14);
    ui->label_CloseGroup_15->setText(b_9ER14PS_24.opencircuitgroup_15);
    ui->label_CloseGroup_16->setText(b_9ER14PS_24.opencircuitgroup_16);
    ui->label_CloseGroup_17->setText(b_9ER14PS_24.opencircuitgroup_17);
    ui->label_CloseGroup_18->setText(b_9ER14PS_24.opencircuitgroup_18);
    ui->label_CloseGroup_19->setText(b_9ER14PS_24.opencircuitgroup_19);
    ui->label_CloseGroup_20->setText(b_9ER14PS_24.opencircuitgroup_20);
    ui->label_CloseGroup_21->setText(b_9ER14PS_24.opencircuitgroup_21);
    ui->label_CloseGroup_22->setText(b_9ER14PS_24.opencircuitgroup_22);
    ui->label_CloseGroup_23->setText(b_9ER14PS_24.opencircuitgroup_23);
    ui->label_CloseGroup_24->setText(b_9ER14PS_24.opencircuitgroup_24);

    ui->label_CloseBat->setText(b_9ER14PS_24.str_closecircuitbattery);
    ui->label_OpenBat->setText(b_9ER14PS_24.str_closecircuitbattery); // строка та же
}

void MainWindow::click_radioButton_Battery_9ER14PS_24_v2()
{
    ui->label_CloseGroup_28->hide();
    ui->label_CloseGroup_27->hide();
    ui->label_CloseGroup_26->hide();
    ui->label_CloseGroup_25->hide();
    ui->label_CloseGroup_24->show();
    ui->label_CloseGroup_23->show();
    ui->label_CloseGroup_22->show();
    ui->label_CloseGroup_21->show();

    ui->label_OpenGroup_28->hide();
    ui->label_OpenGroup_27->hide();
    ui->label_OpenGroup_26->hide();
    ui->label_OpenGroup_25->hide();
    ui->label_OpenGroup_24->show();
    ui->label_OpenGroup_23->show();
    ui->label_OpenGroup_22->show();
    ui->label_OpenGroup_21->show();

    ui->label_CorpusVoltage_1->setText(b_9ER14PS_24_v2.str_voltage_corpus_1);
    ui->label_CorpusVoltage_2->setText(b_9ER14PS_24_v2.str_voltage_corpus_2);
    ui->label_IsolationResistance_1->setText(b_9ER14PS_24_v2.str_isolation_resistance_1);
    ui->label_IsolationResistance_2->setText(b_9ER14PS_24_v2.str_isolation_resistance_2);
    ui->label_IsolationResistance_3->setText(b_9ER14PS_24_v2.str_isolation_resistance_3);
    ui->label_IsolationResistance_4->setText(b_9ER14PS_24_v2.str_isolation_resistance_4);

    ui->label_OpenGroup_1->setText(b_9ER14PS_24_v2.opencircuitgroup_1);
    ui->label_OpenGroup_2->setText(b_9ER14PS_24_v2.opencircuitgroup_2);
    ui->label_OpenGroup_3->setText(b_9ER14PS_24_v2.opencircuitgroup_3);
    ui->label_OpenGroup_4->setText(b_9ER14PS_24_v2.opencircuitgroup_4);
    ui->label_OpenGroup_5->setText(b_9ER14PS_24_v2.opencircuitgroup_5);
    ui->label_OpenGroup_6->setText(b_9ER14PS_24_v2.opencircuitgroup_6);
    ui->label_OpenGroup_7->setText(b_9ER14PS_24_v2.opencircuitgroup_7);
    ui->label_OpenGroup_8->setText(b_9ER14PS_24_v2.opencircuitgroup_8);
    ui->label_OpenGroup_9->setText(b_9ER14PS_24_v2.opencircuitgroup_9);
    ui->label_OpenGroup_10->setText(b_9ER14PS_24_v2.opencircuitgroup_10);
    ui->label_OpenGroup_11->setText(b_9ER14PS_24_v2.opencircuitgroup_11);
    ui->label_OpenGroup_12->setText(b_9ER14PS_24_v2.opencircuitgroup_12);
    ui->label_OpenGroup_13->setText(b_9ER14PS_24_v2.opencircuitgroup_13);
    ui->label_OpenGroup_14->setText(b_9ER14PS_24_v2.opencircuitgroup_14);
    ui->label_OpenGroup_15->setText(b_9ER14PS_24_v2.opencircuitgroup_15);
    ui->label_OpenGroup_16->setText(b_9ER14PS_24_v2.opencircuitgroup_16);
    ui->label_OpenGroup_17->setText(b_9ER14PS_24_v2.opencircuitgroup_17);
    ui->label_OpenGroup_18->setText(b_9ER14PS_24_v2.opencircuitgroup_18);
    ui->label_OpenGroup_19->setText(b_9ER14PS_24_v2.opencircuitgroup_19);
    ui->label_OpenGroup_20->setText(b_9ER14PS_24_v2.opencircuitgroup_20);
    ui->label_OpenGroup_21->setText(b_9ER14PS_24_v2.opencircuitgroup_21);
    ui->label_OpenGroup_22->setText(b_9ER14PS_24_v2.opencircuitgroup_22);
    ui->label_OpenGroup_23->setText(b_9ER14PS_24_v2.opencircuitgroup_23);
    ui->label_OpenGroup_24->setText(b_9ER14PS_24_v2.opencircuitgroup_24);

    ui->label_CloseGroup_1->setText(b_9ER14PS_24_v2.opencircuitgroup_1);
    ui->label_CloseGroup_2->setText(b_9ER14PS_24_v2.opencircuitgroup_2);
    ui->label_CloseGroup_3->setText(b_9ER14PS_24_v2.opencircuitgroup_3);
    ui->label_CloseGroup_4->setText(b_9ER14PS_24_v2.opencircuitgroup_4);
    ui->label_CloseGroup_5->setText(b_9ER14PS_24_v2.opencircuitgroup_5);
    ui->label_CloseGroup_6->setText(b_9ER14PS_24_v2.opencircuitgroup_6);
    ui->label_CloseGroup_7->setText(b_9ER14PS_24_v2.opencircuitgroup_7);
    ui->label_CloseGroup_8->setText(b_9ER14PS_24_v2.opencircuitgroup_8);
    ui->label_CloseGroup_9->setText(b_9ER14PS_24_v2.opencircuitgroup_9);
    ui->label_CloseGroup_10->setText(b_9ER14PS_24_v2.opencircuitgroup_10);
    ui->label_CloseGroup_11->setText(b_9ER14PS_24_v2.opencircuitgroup_11);
    ui->label_CloseGroup_12->setText(b_9ER14PS_24_v2.opencircuitgroup_12);
    ui->label_CloseGroup_13->setText(b_9ER14PS_24_v2.opencircuitgroup_13);
    ui->label_CloseGroup_14->setText(b_9ER14PS_24_v2.opencircuitgroup_14);
    ui->label_CloseGroup_15->setText(b_9ER14PS_24_v2.opencircuitgroup_15);
    ui->label_CloseGroup_16->setText(b_9ER14PS_24_v2.opencircuitgroup_16);
    ui->label_CloseGroup_17->setText(b_9ER14PS_24_v2.opencircuitgroup_17);
    ui->label_CloseGroup_18->setText(b_9ER14PS_24_v2.opencircuitgroup_18);
    ui->label_CloseGroup_19->setText(b_9ER14PS_24_v2.opencircuitgroup_19);
    ui->label_CloseGroup_20->setText(b_9ER14PS_24_v2.opencircuitgroup_20);
    ui->label_CloseGroup_21->setText(b_9ER14PS_24_v2.opencircuitgroup_21);
    ui->label_CloseGroup_22->setText(b_9ER14PS_24_v2.opencircuitgroup_22);
    ui->label_CloseGroup_23->setText(b_9ER14PS_24_v2.opencircuitgroup_23);
    ui->label_CloseGroup_24->setText(b_9ER14PS_24_v2.opencircuitgroup_24);

    ui->label_CloseBat->setText(b_9ER14PS_24_v2.str_closecircuitbattery);
    ui->label_OpenBat->setText(b_9ER14PS_24_v2.str_closecircuitbattery); // строка та же
}

void MainWindow::click_radioButton_Battery_9ER14P_24()
{
    ui->label_CloseGroup_28->hide();
    ui->label_CloseGroup_27->hide();
    ui->label_CloseGroup_26->hide();
    ui->label_CloseGroup_25->hide();
    ui->label_CloseGroup_24->show();
    ui->label_CloseGroup_23->show();
    ui->label_CloseGroup_22->show();
    ui->label_CloseGroup_21->show();

    ui->label_OpenGroup_28->hide();
    ui->label_OpenGroup_27->hide();
    ui->label_OpenGroup_26->hide();
    ui->label_OpenGroup_25->hide();
    ui->label_OpenGroup_24->show();
    ui->label_OpenGroup_23->show();
    ui->label_OpenGroup_22->show();
    ui->label_OpenGroup_21->show();

    ui->label_CorpusVoltage_1->setText(b_9ER14P_24.str_voltage_corpus_1);
    ui->label_CorpusVoltage_2->setText(b_9ER14P_24.str_voltage_corpus_2);
    ui->label_IsolationResistance_1->setText(b_9ER14P_24.str_isolation_resistance_1);
    ui->label_IsolationResistance_2->setText(b_9ER14P_24.str_isolation_resistance_2);
    ui->label_IsolationResistance_3->setText(b_9ER14P_24.str_isolation_resistance_3);
    ui->label_IsolationResistance_4->setText(b_9ER14P_24.str_isolation_resistance_4);

    ui->label_OpenGroup_1->setText(b_9ER14P_24.opencircuitgroup_1);
    ui->label_OpenGroup_2->setText(b_9ER14P_24.opencircuitgroup_2);
    ui->label_OpenGroup_3->setText(b_9ER14P_24.opencircuitgroup_3);
    ui->label_OpenGroup_4->setText(b_9ER14P_24.opencircuitgroup_4);
    ui->label_OpenGroup_5->setText(b_9ER14P_24.opencircuitgroup_5);
    ui->label_OpenGroup_6->setText(b_9ER14P_24.opencircuitgroup_6);
    ui->label_OpenGroup_7->setText(b_9ER14P_24.opencircuitgroup_7);
    ui->label_OpenGroup_8->setText(b_9ER14P_24.opencircuitgroup_8);
    ui->label_OpenGroup_9->setText(b_9ER14P_24.opencircuitgroup_9);
    ui->label_OpenGroup_10->setText(b_9ER14P_24.opencircuitgroup_10);
    ui->label_OpenGroup_11->setText(b_9ER14P_24.opencircuitgroup_11);
    ui->label_OpenGroup_12->setText(b_9ER14P_24.opencircuitgroup_12);
    ui->label_OpenGroup_13->setText(b_9ER14P_24.opencircuitgroup_13);
    ui->label_OpenGroup_14->setText(b_9ER14P_24.opencircuitgroup_14);
    ui->label_OpenGroup_15->setText(b_9ER14P_24.opencircuitgroup_15);
    ui->label_OpenGroup_16->setText(b_9ER14P_24.opencircuitgroup_16);
    ui->label_OpenGroup_17->setText(b_9ER14P_24.opencircuitgroup_17);
    ui->label_OpenGroup_18->setText(b_9ER14P_24.opencircuitgroup_18);
    ui->label_OpenGroup_19->setText(b_9ER14P_24.opencircuitgroup_19);
    ui->label_OpenGroup_20->setText(b_9ER14P_24.opencircuitgroup_20);
    ui->label_OpenGroup_21->setText(b_9ER14P_24.opencircuitgroup_21);
    ui->label_OpenGroup_22->setText(b_9ER14P_24.opencircuitgroup_22);
    ui->label_OpenGroup_23->setText(b_9ER14P_24.opencircuitgroup_23);
    ui->label_OpenGroup_24->setText(b_9ER14P_24.opencircuitgroup_24);

    ui->label_CloseGroup_1->setText(b_9ER14P_24.opencircuitgroup_1);
    ui->label_CloseGroup_2->setText(b_9ER14P_24.opencircuitgroup_2);
    ui->label_CloseGroup_3->setText(b_9ER14P_24.opencircuitgroup_3);
    ui->label_CloseGroup_4->setText(b_9ER14P_24.opencircuitgroup_4);
    ui->label_CloseGroup_5->setText(b_9ER14P_24.opencircuitgroup_5);
    ui->label_CloseGroup_6->setText(b_9ER14P_24.opencircuitgroup_6);
    ui->label_CloseGroup_7->setText(b_9ER14P_24.opencircuitgroup_7);
    ui->label_CloseGroup_8->setText(b_9ER14P_24.opencircuitgroup_8);
    ui->label_CloseGroup_9->setText(b_9ER14P_24.opencircuitgroup_9);
    ui->label_CloseGroup_10->setText(b_9ER14P_24.opencircuitgroup_10);
    ui->label_CloseGroup_11->setText(b_9ER14P_24.opencircuitgroup_11);
    ui->label_CloseGroup_12->setText(b_9ER14P_24.opencircuitgroup_12);
    ui->label_CloseGroup_13->setText(b_9ER14P_24.opencircuitgroup_13);
    ui->label_CloseGroup_14->setText(b_9ER14P_24.opencircuitgroup_14);
    ui->label_CloseGroup_15->setText(b_9ER14P_24.opencircuitgroup_15);
    ui->label_CloseGroup_16->setText(b_9ER14P_24.opencircuitgroup_16);
    ui->label_CloseGroup_17->setText(b_9ER14P_24.opencircuitgroup_17);
    ui->label_CloseGroup_18->setText(b_9ER14P_24.opencircuitgroup_18);
    ui->label_CloseGroup_19->setText(b_9ER14P_24.opencircuitgroup_19);
    ui->label_CloseGroup_20->setText(b_9ER14P_24.opencircuitgroup_20);
    ui->label_CloseGroup_21->setText(b_9ER14P_24.opencircuitgroup_21);
    ui->label_CloseGroup_22->setText(b_9ER14P_24.opencircuitgroup_22);
    ui->label_CloseGroup_23->setText(b_9ER14P_24.opencircuitgroup_23);
    ui->label_CloseGroup_24->setText(b_9ER14P_24.opencircuitgroup_24);

    ui->label_CloseBat->setText(b_9ER14P_24.str_closecircuitbattery);
    ui->label_OpenBat->setText(b_9ER14P_24.str_closecircuitbattery); // строка та же
}

void MainWindow::click_radioButton_Battery_9ER20P_20()
{
    ui->label_CloseGroup_28->hide();
    ui->label_CloseGroup_27->hide();
    ui->label_CloseGroup_26->hide();
    ui->label_CloseGroup_25->hide();
    ui->label_CloseGroup_24->hide();
    ui->label_CloseGroup_23->hide();
    ui->label_CloseGroup_22->hide();
    ui->label_CloseGroup_21->hide();

    ui->label_OpenGroup_28->hide();
    ui->label_OpenGroup_27->hide();
    ui->label_OpenGroup_26->hide();
    ui->label_OpenGroup_25->hide();
    ui->label_OpenGroup_24->hide();
    ui->label_OpenGroup_23->hide();
    ui->label_OpenGroup_22->hide();
    ui->label_OpenGroup_21->hide();

    ui->label_CorpusVoltage_1->setText(b_9ER20P_20.str_voltage_corpus_1);
    ui->label_CorpusVoltage_2->setText(b_9ER20P_20.str_voltage_corpus_2);
    ui->label_IsolationResistance_1->setText(b_9ER20P_20.str_isolation_resistance_1);
    ui->label_IsolationResistance_2->setText(b_9ER20P_20.str_isolation_resistance_2);
    ui->label_IsolationResistance_3->setText(b_9ER20P_20.str_isolation_resistance_3);
    ui->label_IsolationResistance_4->setText(b_9ER20P_20.str_isolation_resistance_4);

    ui->label_OpenGroup_1->setText(b_9ER20P_20.opencircuitgroup_1);
    ui->label_OpenGroup_2->setText(b_9ER20P_20.opencircuitgroup_2);
    ui->label_OpenGroup_3->setText(b_9ER20P_20.opencircuitgroup_3);
    ui->label_OpenGroup_4->setText(b_9ER20P_20.opencircuitgroup_4);
    ui->label_OpenGroup_5->setText(b_9ER20P_20.opencircuitgroup_5);
    ui->label_OpenGroup_6->setText(b_9ER20P_20.opencircuitgroup_6);
    ui->label_OpenGroup_7->setText(b_9ER20P_20.opencircuitgroup_7);
    ui->label_OpenGroup_8->setText(b_9ER20P_20.opencircuitgroup_8);
    ui->label_OpenGroup_9->setText(b_9ER20P_20.opencircuitgroup_9);
    ui->label_OpenGroup_10->setText(b_9ER20P_20.opencircuitgroup_10);
    ui->label_OpenGroup_11->setText(b_9ER20P_20.opencircuitgroup_11);
    ui->label_OpenGroup_12->setText(b_9ER20P_20.opencircuitgroup_12);
    ui->label_OpenGroup_13->setText(b_9ER20P_20.opencircuitgroup_13);
    ui->label_OpenGroup_14->setText(b_9ER20P_20.opencircuitgroup_14);
    ui->label_OpenGroup_15->setText(b_9ER20P_20.opencircuitgroup_15);
    ui->label_OpenGroup_16->setText(b_9ER20P_20.opencircuitgroup_16);
    ui->label_OpenGroup_17->setText(b_9ER20P_20.opencircuitgroup_17);
    ui->label_OpenGroup_18->setText(b_9ER20P_20.opencircuitgroup_18);
    ui->label_OpenGroup_19->setText(b_9ER20P_20.opencircuitgroup_19);
    ui->label_OpenGroup_20->setText(b_9ER20P_20.opencircuitgroup_20);

    ui->label_CloseGroup_1->setText(b_9ER20P_20.opencircuitgroup_1);
    ui->label_CloseGroup_2->setText(b_9ER20P_20.opencircuitgroup_2);
    ui->label_CloseGroup_3->setText(b_9ER20P_20.opencircuitgroup_3);
    ui->label_CloseGroup_4->setText(b_9ER20P_20.opencircuitgroup_4);
    ui->label_CloseGroup_5->setText(b_9ER20P_20.opencircuitgroup_5);
    ui->label_CloseGroup_6->setText(b_9ER20P_20.opencircuitgroup_6);
    ui->label_CloseGroup_7->setText(b_9ER20P_20.opencircuitgroup_7);
    ui->label_CloseGroup_8->setText(b_9ER20P_20.opencircuitgroup_8);
    ui->label_CloseGroup_9->setText(b_9ER20P_20.opencircuitgroup_9);
    ui->label_CloseGroup_10->setText(b_9ER20P_20.opencircuitgroup_10);
    ui->label_CloseGroup_11->setText(b_9ER20P_20.opencircuitgroup_11);
    ui->label_CloseGroup_12->setText(b_9ER20P_20.opencircuitgroup_12);
    ui->label_CloseGroup_13->setText(b_9ER20P_20.opencircuitgroup_13);
    ui->label_CloseGroup_14->setText(b_9ER20P_20.opencircuitgroup_14);
    ui->label_CloseGroup_15->setText(b_9ER20P_20.opencircuitgroup_15);
    ui->label_CloseGroup_16->setText(b_9ER20P_20.opencircuitgroup_16);
    ui->label_CloseGroup_17->setText(b_9ER20P_20.opencircuitgroup_17);
    ui->label_CloseGroup_18->setText(b_9ER20P_20.opencircuitgroup_18);
    ui->label_CloseGroup_19->setText(b_9ER20P_20.opencircuitgroup_19);
    ui->label_CloseGroup_20->setText(b_9ER20P_20.opencircuitgroup_20);

    ui->label_CloseBat->setText(b_9ER20P_20.str_closecircuitbattery);
    ui->label_OpenBat->setText(b_9ER20P_20.str_closecircuitbattery); // строка та же
}

void MainWindow::click_radioButton_Battery_9ER20P_20_v2()
{
    ui->label_CloseGroup_28->hide();
    ui->label_CloseGroup_27->hide();
    ui->label_CloseGroup_26->hide();
    ui->label_CloseGroup_25->hide();
    ui->label_CloseGroup_24->hide();
    ui->label_CloseGroup_23->hide();
    ui->label_CloseGroup_22->hide();
    ui->label_CloseGroup_21->hide();

    ui->label_OpenGroup_28->hide();
    ui->label_OpenGroup_27->hide();
    ui->label_OpenGroup_26->hide();
    ui->label_OpenGroup_25->hide();
    ui->label_OpenGroup_24->hide();
    ui->label_OpenGroup_23->hide();
    ui->label_OpenGroup_22->hide();
    ui->label_OpenGroup_21->hide();

    ui->label_CorpusVoltage_1->setText(b_9ER20P_20_v2.str_voltage_corpus_1);
    ui->label_CorpusVoltage_2->setText(b_9ER20P_20_v2.str_voltage_corpus_2);
    ui->label_IsolationResistance_1->setText(b_9ER20P_20_v2.str_isolation_resistance_1);
    ui->label_IsolationResistance_2->setText(b_9ER20P_20_v2.str_isolation_resistance_2);
    ui->label_IsolationResistance_3->setText(b_9ER20P_20_v2.str_isolation_resistance_3);
    ui->label_IsolationResistance_4->setText(b_9ER20P_20_v2.str_isolation_resistance_4);

    ui->label_OpenGroup_1->setText(b_9ER20P_20_v2.opencircuitgroup_1);
    ui->label_OpenGroup_2->setText(b_9ER20P_20_v2.opencircuitgroup_2);
    ui->label_OpenGroup_3->setText(b_9ER20P_20_v2.opencircuitgroup_3);
    ui->label_OpenGroup_4->setText(b_9ER20P_20_v2.opencircuitgroup_4);
    ui->label_OpenGroup_5->setText(b_9ER20P_20_v2.opencircuitgroup_5);
    ui->label_OpenGroup_6->setText(b_9ER20P_20_v2.opencircuitgroup_6);
    ui->label_OpenGroup_7->setText(b_9ER20P_20_v2.opencircuitgroup_7);
    ui->label_OpenGroup_8->setText(b_9ER20P_20_v2.opencircuitgroup_8);
    ui->label_OpenGroup_9->setText(b_9ER20P_20_v2.opencircuitgroup_9);
    ui->label_OpenGroup_10->setText(b_9ER20P_20_v2.opencircuitgroup_10);
    ui->label_OpenGroup_11->setText(b_9ER20P_20_v2.opencircuitgroup_11);
    ui->label_OpenGroup_12->setText(b_9ER20P_20_v2.opencircuitgroup_12);
    ui->label_OpenGroup_13->setText(b_9ER20P_20_v2.opencircuitgroup_13);
    ui->label_OpenGroup_14->setText(b_9ER20P_20_v2.opencircuitgroup_14);
    ui->label_OpenGroup_15->setText(b_9ER20P_20_v2.opencircuitgroup_15);
    ui->label_OpenGroup_16->setText(b_9ER20P_20_v2.opencircuitgroup_16);
    ui->label_OpenGroup_17->setText(b_9ER20P_20_v2.opencircuitgroup_17);
    ui->label_OpenGroup_18->setText(b_9ER20P_20_v2.opencircuitgroup_18);
    ui->label_OpenGroup_19->setText(b_9ER20P_20_v2.opencircuitgroup_19);
    ui->label_OpenGroup_20->setText(b_9ER20P_20_v2.opencircuitgroup_20);

    ui->label_CloseGroup_1->setText(b_9ER20P_20_v2.opencircuitgroup_1);
    ui->label_CloseGroup_2->setText(b_9ER20P_20_v2.opencircuitgroup_2);
    ui->label_CloseGroup_3->setText(b_9ER20P_20_v2.opencircuitgroup_3);
    ui->label_CloseGroup_4->setText(b_9ER20P_20_v2.opencircuitgroup_4);
    ui->label_CloseGroup_5->setText(b_9ER20P_20_v2.opencircuitgroup_5);
    ui->label_CloseGroup_6->setText(b_9ER20P_20_v2.opencircuitgroup_6);
    ui->label_CloseGroup_7->setText(b_9ER20P_20_v2.opencircuitgroup_7);
    ui->label_CloseGroup_8->setText(b_9ER20P_20_v2.opencircuitgroup_8);
    ui->label_CloseGroup_9->setText(b_9ER20P_20_v2.opencircuitgroup_9);
    ui->label_CloseGroup_10->setText(b_9ER20P_20_v2.opencircuitgroup_10);
    ui->label_CloseGroup_11->setText(b_9ER20P_20_v2.opencircuitgroup_11);
    ui->label_CloseGroup_12->setText(b_9ER20P_20_v2.opencircuitgroup_12);
    ui->label_CloseGroup_13->setText(b_9ER20P_20_v2.opencircuitgroup_13);
    ui->label_CloseGroup_14->setText(b_9ER20P_20_v2.opencircuitgroup_14);
    ui->label_CloseGroup_15->setText(b_9ER20P_20_v2.opencircuitgroup_15);
    ui->label_CloseGroup_16->setText(b_9ER20P_20_v2.opencircuitgroup_16);
    ui->label_CloseGroup_17->setText(b_9ER20P_20_v2.opencircuitgroup_17);
    ui->label_CloseGroup_18->setText(b_9ER20P_20_v2.opencircuitgroup_18);
    ui->label_CloseGroup_19->setText(b_9ER20P_20_v2.opencircuitgroup_19);
    ui->label_CloseGroup_20->setText(b_9ER20P_20_v2.opencircuitgroup_20);

    ui->label_CloseBat->setText(b_9ER20P_20_v2.str_closecircuitbattery);
    ui->label_OpenBat->setText(b_9ER20P_20_v2.str_closecircuitbattery); // строка та же
}

void MainWindow::click_radioButton_Battery_9ER20P_28()
{
    ui->label_CloseGroup_28->show();
    ui->label_CloseGroup_27->show();
    ui->label_CloseGroup_26->show();
    ui->label_CloseGroup_25->show();
    ui->label_CloseGroup_24->show();
    ui->label_CloseGroup_23->show();
    ui->label_CloseGroup_22->show();
    ui->label_CloseGroup_21->show();

    ui->label_OpenGroup_28->show();
    ui->label_OpenGroup_27->show();
    ui->label_OpenGroup_26->show();
    ui->label_OpenGroup_25->show();
    ui->label_OpenGroup_24->show();
    ui->label_OpenGroup_23->show();
    ui->label_OpenGroup_22->show();
    ui->label_OpenGroup_21->show();

    ui->label_CorpusVoltage_1->setText(b_9ER20P_28.str_voltage_corpus_1);
    ui->label_CorpusVoltage_2->setText(b_9ER20P_28.str_voltage_corpus_2);
    ui->label_IsolationResistance_1->setText(b_9ER20P_28.str_isolation_resistance_1);
    ui->label_IsolationResistance_2->setText(b_9ER20P_28.str_isolation_resistance_2);
    ui->label_IsolationResistance_3->setText(b_9ER20P_28.str_isolation_resistance_3);
    ui->label_IsolationResistance_4->setText(b_9ER20P_28.str_isolation_resistance_4);

    ui->label_OpenGroup_1->setText(b_9ER20P_28.opencircuitgroup_1);
    ui->label_OpenGroup_2->setText(b_9ER20P_28.opencircuitgroup_2);
    ui->label_OpenGroup_3->setText(b_9ER20P_28.opencircuitgroup_3);
    ui->label_OpenGroup_4->setText(b_9ER20P_28.opencircuitgroup_4);
    ui->label_OpenGroup_5->setText(b_9ER20P_28.opencircuitgroup_5);
    ui->label_OpenGroup_6->setText(b_9ER20P_28.opencircuitgroup_6);
    ui->label_OpenGroup_7->setText(b_9ER20P_28.opencircuitgroup_7);
    ui->label_OpenGroup_8->setText(b_9ER20P_28.opencircuitgroup_8);
    ui->label_OpenGroup_9->setText(b_9ER20P_28.opencircuitgroup_9);
    ui->label_OpenGroup_10->setText(b_9ER20P_28.opencircuitgroup_10);
    ui->label_OpenGroup_11->setText(b_9ER20P_28.opencircuitgroup_11);
    ui->label_OpenGroup_12->setText(b_9ER20P_28.opencircuitgroup_12);
    ui->label_OpenGroup_13->setText(b_9ER20P_28.opencircuitgroup_13);
    ui->label_OpenGroup_14->setText(b_9ER20P_28.opencircuitgroup_14);
    ui->label_OpenGroup_15->setText(b_9ER20P_28.opencircuitgroup_15);
    ui->label_OpenGroup_16->setText(b_9ER20P_28.opencircuitgroup_16);
    ui->label_OpenGroup_17->setText(b_9ER20P_28.opencircuitgroup_17);
    ui->label_OpenGroup_18->setText(b_9ER20P_28.opencircuitgroup_18);
    ui->label_OpenGroup_19->setText(b_9ER20P_28.opencircuitgroup_19);
    ui->label_OpenGroup_20->setText(b_9ER20P_28.opencircuitgroup_20);
    ui->label_OpenGroup_21->setText(b_9ER20P_28.opencircuitgroup_21);
    ui->label_OpenGroup_22->setText(b_9ER20P_28.opencircuitgroup_22);
    ui->label_OpenGroup_23->setText(b_9ER20P_28.opencircuitgroup_23);
    ui->label_OpenGroup_24->setText(b_9ER20P_28.opencircuitgroup_24);
    ui->label_OpenGroup_25->setText(b_9ER20P_28.opencircuitgroup_25);
    ui->label_OpenGroup_26->setText(b_9ER20P_28.opencircuitgroup_26);
    ui->label_OpenGroup_27->setText(b_9ER20P_28.opencircuitgroup_27);
    ui->label_OpenGroup_28->setText(b_9ER20P_28.opencircuitgroup_28);

    ui->label_CloseGroup_1->setText(b_9ER20P_28.opencircuitgroup_1);
    ui->label_CloseGroup_2->setText(b_9ER20P_28.opencircuitgroup_2);
    ui->label_CloseGroup_3->setText(b_9ER20P_28.opencircuitgroup_3);
    ui->label_CloseGroup_4->setText(b_9ER20P_28.opencircuitgroup_4);
    ui->label_CloseGroup_5->setText(b_9ER20P_28.opencircuitgroup_5);
    ui->label_CloseGroup_6->setText(b_9ER20P_28.opencircuitgroup_6);
    ui->label_CloseGroup_7->setText(b_9ER20P_28.opencircuitgroup_7);
    ui->label_CloseGroup_8->setText(b_9ER20P_28.opencircuitgroup_8);
    ui->label_CloseGroup_9->setText(b_9ER20P_28.opencircuitgroup_9);
    ui->label_CloseGroup_10->setText(b_9ER20P_28.opencircuitgroup_10);
    ui->label_CloseGroup_11->setText(b_9ER20P_28.opencircuitgroup_11);
    ui->label_CloseGroup_12->setText(b_9ER20P_28.opencircuitgroup_12);
    ui->label_CloseGroup_13->setText(b_9ER20P_28.opencircuitgroup_13);
    ui->label_CloseGroup_14->setText(b_9ER20P_28.opencircuitgroup_14);
    ui->label_CloseGroup_15->setText(b_9ER20P_28.opencircuitgroup_15);
    ui->label_CloseGroup_16->setText(b_9ER20P_28.opencircuitgroup_16);
    ui->label_CloseGroup_17->setText(b_9ER20P_28.opencircuitgroup_17);
    ui->label_CloseGroup_18->setText(b_9ER20P_28.opencircuitgroup_18);
    ui->label_CloseGroup_19->setText(b_9ER20P_28.opencircuitgroup_19);
    ui->label_CloseGroup_20->setText(b_9ER20P_28.opencircuitgroup_20);
    ui->label_CloseGroup_21->setText(b_9ER20P_28.opencircuitgroup_21);
    ui->label_CloseGroup_22->setText(b_9ER20P_28.opencircuitgroup_22);
    ui->label_CloseGroup_23->setText(b_9ER20P_28.opencircuitgroup_23);
    ui->label_CloseGroup_24->setText(b_9ER20P_28.opencircuitgroup_24);
    ui->label_CloseGroup_25->setText(b_9ER20P_28.opencircuitgroup_25);
    ui->label_CloseGroup_26->setText(b_9ER20P_28.opencircuitgroup_26);
    ui->label_CloseGroup_27->setText(b_9ER20P_28.opencircuitgroup_27);
    ui->label_CloseGroup_28->setText(b_9ER20P_28.opencircuitgroup_28);

    ui->label_CloseBat->setText(b_9ER20P_28.str_closecircuitbattery);
    ui->label_OpenBat->setText(b_9ER20P_28.str_closecircuitbattery); // строка та же
}

void MainWindow::click_radioButton_Simulator()
{
    ui->label_CloseGroup_28->show();
    ui->label_CloseGroup_27->show();
    ui->label_CloseGroup_26->show();
    ui->label_CloseGroup_25->show();
    ui->label_CloseGroup_24->show();
    ui->label_CloseGroup_23->show();
    ui->label_CloseGroup_22->show();
    ui->label_CloseGroup_21->show();

    ui->label_OpenGroup_28->show();
    ui->label_OpenGroup_27->show();
    ui->label_OpenGroup_26->show();
    ui->label_OpenGroup_25->show();
    ui->label_OpenGroup_24->show();
    ui->label_OpenGroup_23->show();
    ui->label_OpenGroup_22->show();
    ui->label_OpenGroup_21->show();

    ui->label_CorpusVoltage_1->setText(simulator.str_voltage_corpus_1);
    ui->label_CorpusVoltage_2->setText(simulator.str_voltage_corpus_2);

    ui->label_IsolationResistance_1->setText(simulator.str_isolation_resistance_1);
    ui->label_IsolationResistance_2->setText(simulator.str_isolation_resistance_2);
    ui->label_IsolationResistance_3->setText(simulator.str_isolation_resistance_3);
    ui->label_IsolationResistance_4->setText(simulator.str_isolation_resistance_4);

    ui->label_OpenGroup_1->setText(simulator.opencircuitgroup_1);
    ui->label_OpenGroup_2->setText(simulator.opencircuitgroup_2);
    ui->label_OpenGroup_3->setText(simulator.opencircuitgroup_3);
    ui->label_OpenGroup_4->setText(simulator.opencircuitgroup_4);
    ui->label_OpenGroup_5->setText(simulator.opencircuitgroup_5);
    ui->label_OpenGroup_6->setText(simulator.opencircuitgroup_6);
    ui->label_OpenGroup_7->setText(simulator.opencircuitgroup_7);
    ui->label_OpenGroup_8->setText(simulator.opencircuitgroup_8);
    ui->label_OpenGroup_9->setText(simulator.opencircuitgroup_9);
    ui->label_OpenGroup_10->setText(simulator.opencircuitgroup_10);
    ui->label_OpenGroup_11->setText(simulator.opencircuitgroup_11);
    ui->label_OpenGroup_12->setText(simulator.opencircuitgroup_12);
    ui->label_OpenGroup_13->setText(simulator.opencircuitgroup_13);
    ui->label_OpenGroup_14->setText(simulator.opencircuitgroup_14);
    ui->label_OpenGroup_15->setText(simulator.opencircuitgroup_15);
    ui->label_OpenGroup_16->setText(simulator.opencircuitgroup_16);
    ui->label_OpenGroup_17->setText(simulator.opencircuitgroup_17);
    ui->label_OpenGroup_18->setText(simulator.opencircuitgroup_18);
    ui->label_OpenGroup_19->setText(simulator.opencircuitgroup_19);
    ui->label_OpenGroup_20->setText(simulator.opencircuitgroup_20);
    ui->label_OpenGroup_21->setText(simulator.opencircuitgroup_21);
    ui->label_OpenGroup_22->setText(simulator.opencircuitgroup_22);
    ui->label_OpenGroup_23->setText(simulator.opencircuitgroup_23);
    ui->label_OpenGroup_24->setText(simulator.opencircuitgroup_24);
    ui->label_OpenGroup_25->setText(simulator.opencircuitgroup_25);
    ui->label_OpenGroup_26->setText(simulator.opencircuitgroup_26);
    ui->label_OpenGroup_27->setText(simulator.opencircuitgroup_27);
    ui->label_OpenGroup_28->setText(simulator.opencircuitgroup_28);

    ui->label_CloseGroup_1->setText(simulator.opencircuitgroup_1);
    ui->label_CloseGroup_2->setText(simulator.opencircuitgroup_2);
    ui->label_CloseGroup_3->setText(simulator.opencircuitgroup_3);
    ui->label_CloseGroup_4->setText(simulator.opencircuitgroup_4);
    ui->label_CloseGroup_5->setText(simulator.opencircuitgroup_5);
    ui->label_CloseGroup_6->setText(simulator.opencircuitgroup_6);
    ui->label_CloseGroup_7->setText(simulator.opencircuitgroup_7);
    ui->label_CloseGroup_8->setText(simulator.opencircuitgroup_8);
    ui->label_CloseGroup_9->setText(simulator.opencircuitgroup_9);
    ui->label_CloseGroup_10->setText(simulator.opencircuitgroup_10);
    ui->label_CloseGroup_11->setText(simulator.opencircuitgroup_11);
    ui->label_CloseGroup_12->setText(simulator.opencircuitgroup_12);
    ui->label_CloseGroup_13->setText(simulator.opencircuitgroup_13);
    ui->label_CloseGroup_14->setText(simulator.opencircuitgroup_14);
    ui->label_CloseGroup_15->setText(simulator.opencircuitgroup_15);
    ui->label_CloseGroup_16->setText(simulator.opencircuitgroup_16);
    ui->label_CloseGroup_17->setText(simulator.opencircuitgroup_17);
    ui->label_CloseGroup_18->setText(simulator.opencircuitgroup_18);
    ui->label_CloseGroup_19->setText(simulator.opencircuitgroup_19);
    ui->label_CloseGroup_20->setText(simulator.opencircuitgroup_20);
    ui->label_CloseGroup_21->setText(simulator.opencircuitgroup_21);
    ui->label_CloseGroup_22->setText(simulator.opencircuitgroup_22);
    ui->label_CloseGroup_23->setText(simulator.opencircuitgroup_23);
    ui->label_CloseGroup_24->setText(simulator.opencircuitgroup_24);
    ui->label_CloseGroup_25->setText(simulator.opencircuitgroup_25);
    ui->label_CloseGroup_26->setText(simulator.opencircuitgroup_26);
    ui->label_CloseGroup_27->setText(simulator.opencircuitgroup_27);
    ui->label_CloseGroup_28->setText(simulator.opencircuitgroup_28);

    ui->label_CloseBat->setText(simulator.str_closecircuitbattery);
    ui->label_OpenBat->setText(simulator.str_closecircuitbattery); // строка та же
}


//перегруз события закрытия крестиком или Alt-F4
void MainWindow::closeEvent(QCloseEvent *event)
{
    qDebug()<<"closeEvent";
    //event->accept(); by default?
    event->ignore();
    on_action_Exit_triggered();
 }

// нажат пункт меню Выход
void MainWindow::on_action_Exit_triggered()
{
    qDebug()<<"on_action_Exit_triggered";
    // если отчёт не сформирован, то все данные будут утеряны. !!! сообщить в окошке.  завести логическую переменную - отчёт не формировался после какой-нить проверки.

    int ret=QMessageBox::question(this, "Завершение работы программы", "Выйти из программы?", tr("Да"), tr("Нет"));// QMessageBox::Ok, QMessageBox::Cancel);
    if(ret == 0) // да
    {

    //    delete report;// почему-то без этого при закрытии программки вывалилась ошибка закрытия приложения
        // emit destroyed(); // отправить сигнал о закрытии (уничтожении) окна. но тогда надо к каждому окну приклеплять сигнал connect(this, SIGNAL(destroyed()), объект-окно, SLOT(close()));
        // или так:
        qApp->quit(); // qApp - это глобальный (доступный из любого места приложения) указатель на объект текущего приложения
        // Слот quit() - определён в QCoreApplication и реализует выход из приложения с возвратом кода 0 (это код успешного завершения)
        // http://qt-project.org/doc/qt-4.8/qcoreapplication.html#quit
    }
    if(ret == 1) // нет
    {
        qDebug()<<"continue work";
    }

}

// нажат пункт меню Порт
void MainWindow::on_action_Port_triggered()
{
    if(!(this->comPortWidget))
    {
        this->comPortWidget = new ComPortWidget();  // Be sure to destroy you window somewhere
        // сигнал - передача данных по последовательному порту, открытому в mainwiget.cpp
        connect(this->kds, SIGNAL(sendSerialData(QByteArray)), this->comPortWidget, SLOT(procSerialDataTransfer(QByteArray)));
        // по сигналу готовности данных примем их
        connect(this->comPortWidget, SIGNAL(sendSerialReceivedData(QByteArray)), this->kds, SLOT(getSerialDataReceived(QByteArray)));
    }
    if(this->comPortWidget)
    {
        this->comPortWidget->show();
    }
}

// приём данных из порта (для пинга в режиме ожидания в главном окне)
void MainWindow::getSerialDataReceived(quint8 operation_code, QByteArray data)
{
    QByteArray keyword9("ALARM#"), keyword8("IDLE#OK");
//    qDebug() << "mainwindow.cpp getSerialDataReceived(): Rx: " << data.toHex();
    if(ping && (operation_code == 0x01)) // если приняли пинг, то
    {
        if(firstping)
        {
            sendSerialData(0x08, "IDLE#");
            //firstping = false;
            qDebug() << "\n\n MainWindow.cpp getSerialDataReceived(): FIRST IDLE SEND: ";
        }
        //!!!kds->online = true; // установим флаг онлайна
        statusLabel->setText(tr("Связь установлена")); // и напишем про это
    }
    if(data == keyword9)// alarm
    {
        // кажется оно здесь не нужно, проверить!!!: alarm=true;
        //timeout = false;
        sendSerialData(0x08, "IDLE#");
        qDebug() << "\n\n MainWindow.cpp getSerialDataReceived(): ALARM: " << data;
        QMessageBox::critical(this, tr("Батарея неисправна"), tr("Напряжение на корпусе!"));
        return;
    }
    if(data.indexOf(keyword8)>=0) // IDLE#OK стоп режима отработан
    {
/*!!!        kds->box_number=data.right(1).toInt();
        insulResist->box_number=kds->box_number;
        thermometer->box_number=kds->box_number;
        openCircuitGroup->box_number=kds->box_number;
        openCircuitBattery->box_number=kds->box_number;
        closedCircuitGroup->box_number=kds->box_number;
        closedCircuitBattery->box_number=kds->box_number;
        depassivation->box_number=kds->box_number;
        qDebug() << "\n\n MainWindow.cpp getSerialDataReceived(): FIRST IDLE RECEIVED: kds->box_number="<<kds->box_number;*/
        firstping = false;
    }
}
