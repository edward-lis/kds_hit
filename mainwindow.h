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
    int iBatteryIndex;
    int iStep;
    int iAllSteps;
    int iStepVoltageOnTheHousing;
    int iStepInsulationResistance;
    int iStepOpenCircuitVoltageGroup;
    int iStepClosedCircuitVoltageGroup;
    int iStepDepassivation;
    int iStepClosedCircuitVoltageBattery;
    int iStepInsulationResistanceMeasuringBoardUUTBB;
    int iStepOpenCircuitVoltagePowerSupply;
    int iStepClosedCircuitVoltagePowerSupply;
    int iParamsNumberChecked;
    QVector<int> imDepassivation;
    QString str;
    QString paramMsg;
    QSerialPort *com;
    //bool bStop;
    bool bPause;
    bool bCheckCompleteVoltageOnTheHousing;
    bool bCheckCompleteInsulationResistance;
    bool bCheckCompleteOpenCircuitVoltageGroup;
    bool bCheckCompleteClosedCircuitVoltageGroup;
    bool bCheckCompleteClosedCircuitVoltageBattery;
    bool bCheckCompleteInsulationResistanceMeasuringBoardUUTBB;
    bool bCheckCompleteOpenCircuitVoltagePowerSupply;
    bool bCheckCompleteClosedCircuitVoltagePowerSupply;
    void getCOMPorts();

    float param;
public slots:
    void openCOMPort();
    void closeCOMPort();
    void writeCOMPortData();
    void readCOMPortData();
    void checkAutoModeDiagnostic();
    void resetCheck();
    void isUUTBB();
    void setPause();
    //void clickModeDiagnostic();
    //void paramCheck();
    void handleSelectionChangedBattery(int index);
    void Log(QString message, QString color);
    //void setEnabled(bool flag);
    void delay(int millisecondsToWait);
    void progressBarSet(int iVal);
    //void progressBarSetMaximum();
    //void checkVoltageOnTheHousing(int iBatteryCurrentIndex, int iCurrentStep);
    void checkVoltageOnTheHousing();
    void checkInsulationResistance();
    void checkOpenCircuitVoltageGroup();
    void checkClosedCircuitVoltageGroup();
    void checkClosedCircuitVoltageBattery();
    void checkDepassivation();
    void checkInsulationResistanceMeasuringBoardUUTBB();
    void checkOpenCircuitVoltagePowerSupply();
    void checkClosedCircuitVoltagePowerSupply();

};

#endif // MAINWINDOW_H
