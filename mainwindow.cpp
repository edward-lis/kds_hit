#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    settings(NULL)
{
    // загрузить конфигурационные установки из ini-файла. файл находится в том же каталоге, что и исполняемый.
    settings.loadSettings();
    ui->setupUi(this);

}

MainWindow::~MainWindow()
{
    delete ui;
}
