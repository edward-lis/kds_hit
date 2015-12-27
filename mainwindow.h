#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>

#include "settings.h"
#include "comportwidget.h"
#include "kds.h"

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

    // Текст в строке статуса
    QLabel *statusLabel;

    // Окно выбора последовательного порта, и чтения/записи в порт
    ComPortWidget *comPortWidget;
    // Объект - текущая проверка батареи
    Kds *kds;

    // Инициализация объекта КДС
    void initKds();
    // Настройка объекта КДС
    void setupKds();

    bool ping;          // признак режима пинга в главном окне (окно посылает/принимает пинговые посылки)
    bool firstping;     // признак первого пинга. после первого пинга после отсутствия связи коробочка возвращает свой номер, который надо себе запомнить

protected:
    //перегруз для закрытия
    virtual void closeEvent(QCloseEvent *e);

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
    void on_action_Exit_triggered();                   // нажат пункт меню Выход
    void on_action_Port_triggered();

    void getSerialDataReceived(quint8 operation_code, QByteArray data); // ф-я, которая принимает данные из последовательного порта  (для пинга в режиме ожидания в главном окне)


signals:
    void sendSerialData(quint8 operation_code, const QByteArray &data); // сигнал передачи данных в последовательный порт.

};

#endif // MAINWINDOW_H
