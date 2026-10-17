#include "mixerchannel.h"
#include "ui_mixerchannel.h"

MixerChannel::MixerChannel(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MixerChannel)
{
    ui->setupUi(this);
}

MixerChannel::~MixerChannel()
{
    delete ui;
}

void MixerChannel::setLevel(float level)
{
    ui->levelMeter->setLevel(level);
}

void MixerChannel::setVolume(int volume)
{
    ui->fader->setValue(volume);
}

void MixerChannel::setMute(bool muted)
{
    ui->muteCheckBox->setChecked(muted);
}
