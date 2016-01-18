#include "uutbbinsulationresistance20.h"
#include "ui_uutbbinsulationresistance20.h"

UutbbInsulationResistance20::UutbbInsulationResistance20(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::UutbbInsulationResistance20)
{
    ui->setupUi(this);
}

UutbbInsulationResistance20::~UutbbInsulationResistance20()
{
    delete ui;
}
