#include "mainwindow.h"
#include <QApplication>

MainWindow *w=NULL; // вынес объявление переменной на глобальный уровень, чтобы можно было бахнуть какой-нить месседжбокс, например, из левого класса

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    w = new MainWindow;
    w->show();

    return a.exec();
}
