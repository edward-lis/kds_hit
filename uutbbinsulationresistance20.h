#ifndef UUTBBINSULATIONRESISTANCE20_H
#define UUTBBINSULATIONRESISTANCE20_H

#include <QDialog>

namespace Ui {
class UutbbInsulationResistance20;
}

class UutbbInsulationResistance20 : public QDialog
{
    Q_OBJECT

public:
    explicit UutbbInsulationResistance20(QWidget *parent = 0);
    ~UutbbInsulationResistance20();

private:
    Ui::UutbbInsulationResistance20 *ui;
};

#endif // UUTBBINSULATIONRESISTANCE20_H
