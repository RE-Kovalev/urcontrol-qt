#include "streamingeffects.h"
#include "ui_streamingeffects.h"

StreamingEffects::StreamingEffects(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::StreamingEffects)
{
    ui->setupUi(this);
}

StreamingEffects::~StreamingEffects()
{
    delete ui;
}
