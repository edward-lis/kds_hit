/* таки писать на двух языках или нет? */
// точки измерения параметров имитатора батареи - уточнить в ини-файле
// при старте заблокировать выбор батареи. добавить в кдс ф-ию инициализации батареи.
// сколько измерений изоляции УСХТИЛБ ? что значит поочерёдно?


// Описание:
// В ини-файле параметры конкретных батарей.
// В зависимости от типа батареи - разный интерфейс в части кол-ва цепей и точек измерения параметров
// Связь по последовательному порту происходит через объект Kds. Который, в свою очередь, использует объект comportwidget.
// При нахождении в режиме главного окна периодически посылается пинг. При переключении в какой-нибудь режим диагностики пинг в главном окне отключается.
// После первого пинга после отсутствия связи коробочка возвращает свой номер, который надо себе запомнить.

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

    // для отладки уберём
    ui->groupBox_OpenBat->setVisible(false);
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

    ui->label_CorpusVoltage_1->setText(b_9ER14PS_24.voltage_corpus_1);
    ui->label_CorpusVoltage_2->setText(b_9ER14PS_24.voltage_corpus_2);
    ui->label_IsolationResistance_1->setText(b_9ER14PS_24.isolation_resistance_1);
    ui->label_IsolationResistance_2->setText(b_9ER14PS_24.isolation_resistance_2);
    ui->label_IsolationResistance_3->setText(b_9ER14PS_24.isolation_resistance_3);
    ui->label_IsolationResistance_4->setText(b_9ER14PS_24.isolation_resistance_4);
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

    ui->label_CorpusVoltage_1->setText(b_9ER14PS_24_v2.voltage_corpus_1);
    ui->label_CorpusVoltage_2->setText(b_9ER14PS_24_v2.voltage_corpus_2);
    ui->label_IsolationResistance_1->setText(b_9ER14PS_24_v2.isolation_resistance_1);
    ui->label_IsolationResistance_2->setText(b_9ER14PS_24_v2.isolation_resistance_2);
    ui->label_IsolationResistance_3->setText(b_9ER14PS_24_v2.isolation_resistance_3);
    ui->label_IsolationResistance_4->setText(b_9ER14PS_24_v2.isolation_resistance_4);
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

    ui->label_CorpusVoltage_1->setText(b_9ER14P_24.voltage_corpus_1);
    ui->label_CorpusVoltage_2->setText(b_9ER14P_24.voltage_corpus_2);
    ui->label_IsolationResistance_1->setText(b_9ER14P_24.isolation_resistance_1);
    ui->label_IsolationResistance_2->setText(b_9ER14P_24.isolation_resistance_2);
    ui->label_IsolationResistance_3->setText(b_9ER14P_24.isolation_resistance_3);
    ui->label_IsolationResistance_4->setText(b_9ER14P_24.isolation_resistance_4);
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

    ui->label_CorpusVoltage_1->setText(b_9ER20P_20.voltage_corpus_1);
    ui->label_CorpusVoltage_2->setText(b_9ER20P_20.voltage_corpus_2);
    ui->label_IsolationResistance_1->setText(b_9ER20P_20.isolation_resistance_1);
    ui->label_IsolationResistance_2->setText(b_9ER20P_20.isolation_resistance_2);
    ui->label_IsolationResistance_3->setText(b_9ER20P_20.isolation_resistance_3);
    ui->label_IsolationResistance_4->setText(b_9ER20P_20.isolation_resistance_4);
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

    ui->label_CorpusVoltage_1->setText(b_9ER20P_20_v2.voltage_corpus_1);
    ui->label_CorpusVoltage_2->setText(b_9ER20P_20_v2.voltage_corpus_2);
    ui->label_IsolationResistance_1->setText(b_9ER20P_20_v2.isolation_resistance_1);
    ui->label_IsolationResistance_2->setText(b_9ER20P_20_v2.isolation_resistance_2);
    ui->label_IsolationResistance_3->setText(b_9ER20P_20_v2.isolation_resistance_3);
    ui->label_IsolationResistance_4->setText(b_9ER20P_20_v2.isolation_resistance_4);
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

    ui->label_CorpusVoltage_1->setText(b_9ER20P_28.voltage_corpus_1);
    ui->label_CorpusVoltage_2->setText(b_9ER20P_28.voltage_corpus_2);
    ui->label_IsolationResistance_1->setText(b_9ER20P_28.isolation_resistance_1);
    ui->label_IsolationResistance_2->setText(b_9ER20P_28.isolation_resistance_2);
    ui->label_IsolationResistance_3->setText(b_9ER20P_28.isolation_resistance_3);
    ui->label_IsolationResistance_4->setText(b_9ER20P_28.isolation_resistance_4);
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

    ui->label_CorpusVoltage_1->setText(simulator.voltage_corpus_1);
    ui->label_CorpusVoltage_2->setText(simulator.voltage_corpus_2);

    ui->label_IsolationResistance_1->setText(simulator.isolation_resistance_1);
    ui->label_IsolationResistance_2->setText(simulator.isolation_resistance_2);
    ui->label_IsolationResistance_3->setText(simulator.isolation_resistance_3);
    ui->label_IsolationResistance_4->setText(simulator.isolation_resistance_4);
}


//перегруз события закрытия крестиком или Alt-F4
void MainWindow::closeEvent(QCloseEvent *event)
{
    event->accept();
    on_action_Exit_triggered();
 }

// нажат пункт меню Выход
void MainWindow::on_action_Exit_triggered()
{
    //    delete report;// почему-то без этого при закрытии программки вывалилась ошибка закрытия приложения
        // emit destroyed(); // отправить сигнал о закрытии (уничтожении) окна. но тогда надо к каждому окну приклеплять сигнал connect(this, SIGNAL(destroyed()), объект-окно, SLOT(close()));
        // или так:
        qApp->quit(); // qApp - это глобальный (доступный из любого места приложения) указатель на объект текущего приложения
        // Слот quit() - определён в QCoreApplication и реализует выход из приложения с возвратом кода 0 (это код успешного завершения)
        // http://qt-project.org/doc/qt-4.8/qcoreapplication.html#quit
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
