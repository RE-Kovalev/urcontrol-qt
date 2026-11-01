#include "ducker.h"
#include "ui_ducker.h"

ducker::ducker(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ducker)
{
    ui->setupUi(this);
}

ducker::~ducker()
{
    delete ui;
}
