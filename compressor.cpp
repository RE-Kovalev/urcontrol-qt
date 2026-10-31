#include "compressor.h"
#include "ui_compressor.h"

Compressor::Compressor(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Compressor)
{
    ui->setupUi(this);
}

Compressor::~Compressor()
{
    delete ui;
}
