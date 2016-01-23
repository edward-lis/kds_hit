#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QItemDelegate>
#include <QStandardItemModel>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QTime>
#include <QMessageBox>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    int iStartCheck;
    int iBatteryCurrentIndex;
    int iDiagnosticModeCurrentIndex;
    int iProgressBarAllSteps;
    int iParamsNumberChecked;
    QString color;
    QString paramMsg;
    QSerialPort *com;
    void fillPortsInfo();
    float paramVoltageOnTheHousing1;
    float paramVoltageOnTheHousing2;
    float paramInsulationResistance1;
    float paramInsulationResistance2;
    float paramInsulationResistance3;
    float paramInsulationResistance4;
    //float paramClosedCircuitVoltage;
    //float paramClosedCircuitVoltage;
    float paramClosedCircuitVoltage;
public slots:
    void ResetCheck();
    void paramCheck();
    void handleSelectionChangedBattery(int index);
    void handleSelectionChangedDiagnosticMode(int index);
    void CheckBatteryVoltageOnTheHousing(int index);
    void CheckBatteryInsulationResistance(int index);
    void CheckBatteryOpenCircuitVoltageGroup(int index);
    void CheckBatteryClosedCircuitVoltageGroup(int index);
    void CheckBatteryClosedCircuitVoltage(int index);
    void CheckBatteryInsulationResistanceMeasuringBoardUUTBB(int index);
    void CheckBatteryOpenCircuitVoltagePowerSupply(int index);
    void CheckBatteryClosedCircuitVoltagePowerSupply(int index);
    void CheckBattery();
    void Log(QString message, QString color);
    void setEnabled(bool flag);
    void delay(int millisecondsToWait);
    void progressBarSet(int iVal);
    void progressBarSetMaximum();
    void openCOMPort();
    void closeCOMPort();
    void writeData();
    void readData();
};

#endif // MAINWINDOW_H
