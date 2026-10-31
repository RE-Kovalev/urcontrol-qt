#include "gate.h"
#include "ui_gate.h"

Gate::Gate(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Gate)
{
    ui->setupUi(this);
}

Gate::~Gate()
{
    delete ui;
}
