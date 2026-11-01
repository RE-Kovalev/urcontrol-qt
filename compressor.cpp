#include "compressor.h"
#include "ui_compressor.h"
#include "device.h"

#include <QCheckBox>
#include <QHideEvent>
#include <QLabel>
#include <QRadioButton>
#include <QShowEvent>
#include <QSignalBlocker>
#include <QSlider>
#include <cmath>

Compressor::Compressor(uint8_t channel, const QString& channelName, QWidget* parent)
    : EffectEditor(channel, parent)
    , ui(new Ui::Compressor)
{
    ui->setupUi(this);
    setWindowTitle(tr("Compressor - %1").arg(channelName));
    ui->levels->setMono(true);

    sliders = {
        { {ui->threshold, ui->thresholdValue, &EffectProtocol::compThreshold}
        , {ui->ratio, ui->ratioValue, &EffectProtocol::compRatio}
        , {ui->gain, ui->gainValue, &EffectProtocol::compGain}
        , {ui->attack, ui->attackValue, &EffectProtocol::compAttack}
        , {ui->release, ui->releaseValue, &EffectProtocol::compRelease}
        }
    };

    for (int index = 0; index < static_cast<int>(sliders.size()); index++)
        bindSlider(index);

    ui->gainAuto->setEnabled(EffectProtocol::compGainAuto.isKnown() && Device::get());
    if (EffectProtocol::compGainAuto.isKnown())
        ui->gain->setEnabled(false); // Wait for the device auto-makeup state.
    
    {
        const QSignalBlocker blocker(ui->gainAuto);
        ui->gainAuto->setChecked(false);
    }
    connect(ui->gainAuto, &QCheckBox::toggled, this, &Compressor::sendGainAuto);

    const auto& knee = EffectProtocol::compKnee;
    const bool canSetKnee = knee.hasRange() && knee.maximum - knee.minimum == 2 && Device::get();
    
    ui->kneeSoft->setEnabled(canSetKnee);
    ui->kneeMedium->setEnabled(canSetKnee);
    ui->kneeHard->setEnabled(canSetKnee);

    connect(ui->kneeSoft, &QRadioButton::toggled, this,
            [this](bool checked)
            {
                if (checked)
                    sendKnee(EffectProtocol::compKnee.minimum);
            });
    connect(ui->kneeMedium, &QRadioButton::toggled, this,
            [this](bool checked)
            {
                if (checked)
                    sendKnee(EffectProtocol::compKnee.minimum + 1);
            });
    connect(ui->kneeHard, &QRadioButton::toggled, this,
            [this](bool checked)
            {
                if (checked)
                    sendKnee(EffectProtocol::compKnee.minimum + 2);
            });

    gainPollTimer.setInterval(500);
    connect(&gainPollTimer, &QTimer::timeout, this, &Compressor::queryGain);
    requestInitialValues();
}

Compressor::~Compressor()
{
    gainPollTimer.stop();
    delete ui;
}

void Compressor::bindSlider(int index)
{
    SliderBinding& binding = sliders.at(index);
    binding.label->setText(QStringLiteral("?"));
    binding.slider->setEnabled(binding.parameter->hasRange() && Device::get());
    if (binding.parameter->hasRange())
    {
        const QSignalBlocker blocker(binding.slider);
        binding.slider->setRange(binding.parameter->minimum, binding.parameter->maximum);
    }

    connect(binding.slider, &QSlider::valueChanged, this, [this, index](int value)
            {
                sendSlider(index, value);
            });
}

void Compressor::sendSlider(int index, int value)
{
    SliderBinding& binding = sliders.at(index);
    urc_device* device = Device::get();
    if (!device 
    ||  !binding.parameter->hasRange()
    ||  (binding.slider == ui->gain && ui->gainAuto->isChecked()))
        return;

    const int result = urc_change_parameter(device, static_cast<uint16_t>(binding.parameter->id), value, protocolChannel());
    if (result == URC_OK)
        applySlider(index, value);
    else 
    if (binding.initialized)
        applySlider(index, binding.confirmed);
}

void Compressor::applySlider(int index, int32_t value)
{
    SliderBinding& binding = sliders.at(index);
    binding.initialized = true;
    binding.confirmed = value;
    binding.label->setText(EffectProtocol::formatValue(*binding.parameter, value));
    if (binding.parameter->hasRange())
    {
        const QSignalBlocker blocker(binding.slider);
        binding.slider->setValue(value);
    }
}

void Compressor::applyGainAuto(bool enabled)
{
    gainAutoInitialized = true;
    gainAutoConfirmed = enabled;
    {
        const QSignalBlocker blocker(ui->gainAuto);
        ui->gainAuto->setChecked(enabled);
    }

    ui->gain->setEnabled(EffectProtocol::compGain.hasRange() && Device::get() && !enabled);
    updateGainPolling();
    if (enabled)
        queryGain();
}

void Compressor::sendGainAuto(bool enabled)
{
    urc_device* device = Device::get();
    const auto& parameter = EffectProtocol::compGainAuto;
    if (!device 
    ||  !parameter.isKnown())
    {
        if (gainAutoInitialized)
            applyGainAuto(gainAutoConfirmed);
        return;
    }

    const int result = urc_change_parameter(device, static_cast<uint16_t>(parameter.id), enabled ? 1 : 0, protocolChannel());
    if (result == URC_OK)
        applyGainAuto(enabled);
    else 
    if (gainAutoInitialized)
        applyGainAuto(gainAutoConfirmed);
}

void Compressor::applyKnee(int32_t value)
{
    kneeInitialized = true;
    kneeConfirmed = value;
    const auto& parameter = EffectProtocol::compKnee;
    if (!parameter.hasRange() 
    ||  parameter.maximum - parameter.minimum != 2)
        return;

    const QSignalBlocker soft(ui->kneeSoft);
    const QSignalBlocker medium(ui->kneeMedium);
    const QSignalBlocker hard(ui->kneeHard);
    if (value == parameter.minimum)
        ui->kneeSoft->setChecked(true);
    else 
    if (value == parameter.minimum + 1)
        ui->kneeMedium->setChecked(true);
    else 
    if (value == parameter.minimum + 2)
        ui->kneeHard->setChecked(true);
}

void Compressor::sendKnee(int32_t value)
{
    urc_device* device = Device::get();
    const auto& parameter = EffectProtocol::compKnee;
    if (!device 
    ||  !parameter.hasRange() 
    ||  parameter.maximum - parameter.minimum != 2)
        return;

    const int result = urc_change_parameter(device, static_cast<uint16_t>(parameter.id), value, protocolChannel());
    if (result == URC_OK)
        applyKnee(value);
    else 
    if (kneeInitialized)
        applyKnee(kneeConfirmed);
}

void Compressor::applyIncomingParameter(uint16_t id, int32_t value)
{
    for (int index = 0; index < static_cast<int>(sliders.size()); index++)
    {
        if (sliders.at(index).parameter->isKnown()
        &&  sliders.at(index).parameter->id == id)
            applySlider(index, value);
    }

    if (EffectProtocol::compGainAuto.isKnown()
    &&  EffectProtocol::compGainAuto.id == id)
        applyGainAuto(value == 1);

    if (EffectProtocol::compKnee.isKnown()
    &&  EffectProtocol::compKnee.id == id)
        applyKnee(value);
}

void Compressor::requestInitialValues()
{
    urc_device* device = Device::get();
    if (!device)
        return;

    for (const SliderBinding& binding : sliders)
        if (binding.parameter->isKnown())
            urc_query_parameter(device, static_cast<uint16_t>(binding.parameter->id),protocolChannel());

    if (EffectProtocol::compGainAuto.isKnown())
        urc_query_parameter(device, static_cast<uint16_t>(EffectProtocol::compGainAuto.id),protocolChannel());
    if (EffectProtocol::compKnee.isKnown())
        urc_query_parameter(device, static_cast<uint16_t>(EffectProtocol::compKnee.id),protocolChannel());
}

void Compressor::queryGain()
{
    urc_device* device = Device::get();
    if (device 
    &&  EffectProtocol::compGain.isKnown())
        urc_query_parameter(device, static_cast<uint16_t>(EffectProtocol::compGain.id), protocolChannel());
}

void Compressor::updateGainPolling()
{
    if (isVisible() 
    &&  gainAutoConfirmed 
    &&  EffectProtocol::compGain.isKnown() 
    &&  Device::get())
        gainPollTimer.start();
    else
        gainPollTimer.stop();
}

void Compressor::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);
    updateGainPolling();
}

void Compressor::hideEvent(QHideEvent* event)
{
    gainPollTimer.stop();
    QDialog::hideEvent(event);
}

void Compressor::setMeterLevels(const urc_meter_level& level)
{
    const float current = std::pow(10.0f, static_cast<float>(level.current) / 200.0f);
    const float peak = std::pow(10.0f, static_cast<float>(level.peak) / 200.0f);
    ui->levels->setLevels(current, current, peak, peak);
}

void Compressor::resetMeter()
{
    ui->levels->setLevel(0.0f);
}
