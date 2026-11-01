#include "mbcompressor.h"
#include "ui_mbcompressor.h"

mbcompressor::mbcompressor(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::mbcompressor)
{
    ui->setupUi(this);
}

mbcompressor::~mbcompressor()
{
    delete ui;
}
