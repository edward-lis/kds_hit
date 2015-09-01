/* таки писать на двух языках или нет? */
// точки измерения параметров имитатора батареи - уточнить в ини-файле
// при старте заблокировать выбор батареи
// сколько измерений изоляции УСХТИЛБ ? что значит поочерёдно?

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "battery.h"

///
/// \brief параметры конкретных типов батарей
///
Battery simulator, b_9ER20P_20, b_9ER20P_20_v2, b_9ER14PS_24, b_9ER14PS_24_v2, b_9ER20P_28, b_9ER14P_24;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    settings(NULL)
{
    // загрузить конфигурационные установки и параметры батарей из ini-файла.
    // файл находится в том же каталоге, что и исполняемый.
    settings.loadSettings();
    ui->setupUi(this);

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

