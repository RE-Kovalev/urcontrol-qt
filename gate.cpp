#include "gate.h"
#include "ui_gate.h"
#include "device.h"

#include <QLabel>
#include <QSignalBlocker>
#include <QSlider>
#include <cmath>

Gate::Gate(uint8_t channel, const QString& channelName, QWidget* parent)
    : EffectEditor(channel, parent)
    , ui(new Ui::Gate)
{
    ui->setupUi(this);
    setWindowTitle(tr("Gate - %1").arg(channelName));
    ui->levels->setMono(true);

    sliders = {
        { {ui->threshold, ui->thresholdValue, &EffectProtocol::gateThreshold}
        , {ui->range, ui->rangeValue, &EffectProtocol::gateRange}
        , {ui->attack, ui->attackValue, &EffectProtocol::gateAttack}
        , {ui->hold, ui->holdValue, &EffectProtocol::gateHold}
        , {ui->decay, ui->decayValue, &EffectProtocol::gateDecay}
        }
    };

    for (int index = 0; index < static_cast<int>(sliders.size()); index++)
        bindSlider(index);

    requestInitialValues();
}

Gate::~Gate()
{
    delete ui;
}

void Gate::bindSlider(int index)
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

void Gate::sendSlider(int index, int value)
{
    SliderBinding& binding = sliders.at(index);
    urc_device* device = Device::get();
    if (!device 
    ||  !binding.parameter->hasRange())
        return;

    const int result = urc_change_parameter(device, static_cast<uint16_t>(binding.parameter->id), value, protocolChannel());
    if (result == URC_OK)
        applySlider(index, value);
    else 
    if (binding.initialized)
        applySlider(index, binding.confirmed);
}

void Gate::applySlider(int index, int32_t value)
{
    SliderBinding& binding = sliders.at(index);
    binding.confirmed = value;
    binding.initialized = true;
    binding.label->setText(EffectProtocol::formatValue(*binding.parameter, value));
    if (binding.parameter->hasRange())
    {
        const QSignalBlocker blocker(binding.slider);
        binding.slider->setValue(value);
    }
}

void Gate::applyIncomingParameter(uint16_t id, int32_t value)
{
    for (int index = 0; index < static_cast<int>(sliders.size()); index++)
    {
        if (sliders.at(index).parameter->isKnown()
        &&  sliders.at(index).parameter->id == id)
            applySlider(index, value);
    }
}

void Gate::requestInitialValues()
{
    urc_device* device = Device::get();
    if (!device)
        return;

    for (const SliderBinding& binding : sliders)
    {
        if (binding.parameter->isKnown())
            urc_query_parameter(device, static_cast<uint16_t>(binding.parameter->id), protocolChannel());
    }
}

void Gate::setMeterLevels(const urc_meter_level& level)
{
    const float current = std::pow(10.0f, static_cast<float>(level.current) / 200.0f);
    const float peak = std::pow(10.0f, static_cast<float>(level.peak) / 200.0f);
    ui->levels->setLevels(current, current, peak, peak);
}

void Gate::resetMeter()
{
    ui->levels->setLevel(0.0f);
}
