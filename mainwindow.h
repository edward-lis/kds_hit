#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "settings.h"

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
    ///
    /// \brief settings конфигурация переменных параметров из ini-файла
    ///
    Settings settings;

private slots:
    void pressbutton();
    // слоты вызываются при клике на радиокнопки выбора устройстваы
    void click_radioButton_Simulator();
    void click_radioButton_Battery_9ER20P_20();
    void click_radioButton_Battery_9ER20P_20_v2();
    void click_radioButton_Battery_9ER14PS_24();
    void click_radioButton_Battery_9ER14PS_24_v2();
    void click_radioButton_Battery_9ER20P_28();
    void click_radioButton_Battery_9ER14P_24();
};

#endif // MAINWINDOW_H
