#include "mixerchannel.h"
#include "ui_mixerchannel.h"

#include <QSignalBlocker>

#include "device.h"

MixerChannel::MixerChannel(QWidget *parent)
: QWidget(parent)
, ui(new Ui::MixerChannel)
{
    ui->setupUi(this);

    // Update policy for hidable elements
    QSizePolicy policy = ui->soloCheckBox->sizePolicy();
    policy.setRetainSizeWhenHidden(true);
    ui->soloCheckBox->setSizePolicy(policy);
    policy = ui->PAN->sizePolicy();
    policy.setRetainSizeWhenHidden(true);
    ui->PAN->setSizePolicy(policy);

    connect(ui->fader, &QSlider::valueChanged,
            this, [this](int value)
            {
                if (!Device::get()
                ||  !faderParameter)
                    return;

                const int result = urc_change_parameter(Device::get(), faderParameter->id, value, channel);
                qWarning() << result;
                if (result != URC_OK)
                    emit parameterWriteFailed(result);
            });

    connect(ui->PAN, &QDial::valueChanged,
            this, [this](int value)
            {
                if (!Device::get()
                ||  !PANParameter)
                    return;

                const int result = urc_change_parameter(Device::get(), PANParameter->id, value, channel);
                if (result != URC_OK)
                    emit parameterWriteFailed(result);
            });

    connect(ui->muteCheckBox, &QCheckBox::toggled,
            this, [this](bool muted)
            {
                if (!Device::get()
                ||  !muteParameter)
                    return;

                const int result = urc_change_parameter(Device::get(), muteParameter->id, muted ? 0 : 1, channel);

                if (result != URC_OK)
                    emit parameterWriteFailed(result);
            });

    connect(ui->soloCheckBox, &QCheckBox::toggled,
            this, [this](bool solo)
            {
                if (!Device::get()
                ||  !soloParameter)
                    return;

                const int result = urc_change_parameter(Device::get(), soloParameter->id, solo ? 1 : 0, channel);

                if (result != URC_OK)
                    emit parameterWriteFailed(result);
            });
}

MixerChannel::~MixerChannel()
{
    delete ui;
}

bool MixerChannel::bindFaderParameter(const urc_parameter* _parameter)
{
    if (!_parameter)
        return false;

    faderParameter = _parameter;

    const QSignalBlocker blocker(ui->fader);

    ui->fader->setRange(faderParameter->minimum, faderParameter->maximum);

    if (faderParameter->default_value != URC_NO_DEFAULT)
        ui->fader->setValue(faderParameter->default_value);

    return true;
}

void MixerChannel::setFaderValue(int32_t value)
{
    if (!faderParameter)
        return;

    const QSignalBlocker blocker(ui->fader);

    ui->fader->setValue(value);
}

bool MixerChannel::bindMuteParameter(const urc_parameter *_parameter)
{
    if (!_parameter)
        return false;

    muteParameter = _parameter;

    return true;
}

void MixerChannel::setMute(bool muted)
{
    const QSignalBlocker blocker(ui->muteCheckBox);
    ui->muteCheckBox->setChecked(muted);
}

bool MixerChannel::bindSoloParameter(const urc_parameter *_parameter)
{
    if (!_parameter)
        return false;

    soloParameter = _parameter;

    return true;
}

void MixerChannel::setSolo(bool solo)
{
    const QSignalBlocker blocker(ui->soloCheckBox);
    ui->soloCheckBox->setChecked(solo);
}

bool MixerChannel::bindPANParameter(const urc_parameter* _parameter)
{
    if (!_parameter)
        return false;

    PANParameter = _parameter;

    const QSignalBlocker blocker(ui->PAN);

    ui->PAN->setRange(PANParameter->minimum, PANParameter->maximum);

    if (PANParameter->default_value != URC_NO_DEFAULT)
        ui->PAN->setValue(PANParameter->default_value);

    return true;
}

void MixerChannel::setSoloVisible(bool visibility)
{
    ui->soloCheckBox->setVisible(visibility);
}

void MixerChannel::setPANVisible(bool visibility)
{
    ui->PAN->setVisible(visibility);
}

void MixerChannel::setPANValue(int32_t value)
{
    if (!PANParameter)
        return;

    const QSignalBlocker blocker(ui->PAN);
    ui->PAN->setValue(value);
}
