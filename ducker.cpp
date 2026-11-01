#include "ducker.h"
#include "ui_ducker.h"
#include "device.h"

#include <QCheckBox>
#include <QLabel>
#include <QSignalBlocker>
#include <QSlider>
#include <cmath>

namespace
{
    constexpr EffectProtocol::Parameter input1Source{316, 0, 1};
    constexpr EffectProtocol::Parameter input2Source{317, 0, 1};
    constexpr EffectProtocol::Parameter voiceSource{318, 0, 1};
    constexpr EffectProtocol::Parameter threshold{319, 1, 73, EffectProtocol::Display::Threshold};
    constexpr EffectProtocol::Parameter range{320, 0, 73, EffectProtocol::Display::GateRange};
    constexpr EffectProtocol::Parameter attack{321, 57, 283, EffectProtocol::Display::Attack};
    constexpr EffectProtocol::Parameter decay{323, 24, 300, EffectProtocol::Display::Decay};
}

ducker::ducker(uint8_t channel, const QString& channelName, QWidget *parent)
    : EffectEditor(channel, parent)
    , ui(new Ui::ducker)
{
    ui->setupUi(this);
    setWindowTitle(tr("Ducker - %1").arg(channelName));
    ui->levels->setMono(true);

    sources = {
        { {ui->input1, &input1Source}
        , {ui->input2, &input2Source}
        , {ui->voice, &voiceSource}
        }
    };

    sliders = {
        { {ui->threshold, ui->thresholdValue, &threshold}
        , {ui->range, ui->rangeValue, &range}
        , {ui->attack, ui->attackValue, &attack}
        , {ui->decay, ui->decayValue, &decay}
        }
    };

    for (int index = 0; index < static_cast<int>(sources.size()); index++)
        bindSource(index);

    for (int index = 0; index < static_cast<int>(sliders.size()); index++)
        bindSlider(index);

    requestInitialValues();
}

ducker::~ducker()
{
    delete ui;
}

void ducker::bindSource(int index)
{
    SourceBinding& binding = sources.at(index);
    binding.checkbox->setEnabled(Device::get() != nullptr);
    binding.checkbox->setChecked(false);

    connect(binding.checkbox, &QCheckBox::toggled, this,
            [this, index](bool checked)
            {
                sendSource(index, checked);
            });
}

void ducker::bindSlider(int index)
{
    SliderBinding& binding = sliders.at(index);
    binding.label->setText(QStringLiteral("?"));
    binding.slider->setEnabled(binding.parameter->hasRange() && Device::get());
    if (binding.parameter->hasRange())
    {
        const QSignalBlocker blocker(binding.slider);
        binding.slider->setRange(binding.parameter->minimum, binding.parameter->maximum);
    }

    connect(binding.slider, &QSlider::valueChanged, this,
            [this, index](int value)
            {
                sendSlider(index, value);
            });
}

void ducker::sendSource(int index, bool checked)
{
    SourceBinding& binding = sources.at(index);
    urc_device* device = Device::get();
    if (!device)
        return;

    const int result = urc_change_parameter(device, static_cast<uint16_t>(binding.parameter->id), checked ? 1 : 0, protocolChannel());
    if (result == URC_OK)
        applySource(index, checked ? 1 : 0);
    else 
    if (binding.initialized)
        applySource(index, binding.confirmed);
}

void ducker::sendSlider(int index, int value)
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

void ducker::applySource(int index, int32_t value)
{
    SourceBinding& binding = sources.at(index);
    binding.confirmed = value;
    binding.initialized = true;
    const QSignalBlocker blocker(binding.checkbox);
    binding.checkbox->setChecked(value != 0);
}

void ducker::applySlider(int index, int32_t value)
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

void ducker::applyIncomingParameter(uint16_t id, int32_t value)
{
    for (int index = 0; index < static_cast<int>(sources.size()); index++)
        if (sources.at(index).parameter->id == id)
            applySource(index, value);

    for (int index = 0; index < static_cast<int>(sliders.size()); index++)
        if (sliders.at(index).parameter->id == id)
            applySlider(index, value);
}

void ducker::requestInitialValues()
{
    urc_device* device = Device::get();
    if (!device)
        return;

    for (const SourceBinding& binding : sources)
        urc_query_parameter(device, static_cast<uint16_t>(binding.parameter->id), protocolChannel());

    for (const SliderBinding& binding : sliders)
        urc_query_parameter(device, static_cast<uint16_t>(binding.parameter->id), protocolChannel());
}

void ducker::setMeterLevels(const urc_meter_level& level)
{
    const float current = std::pow(10.0f, static_cast<float>(level.current) / 200.0f);
    const float peak = std::pow(10.0f, static_cast<float>(level.peak) / 200.0f);
    ui->levels->setLevels(current, current, peak, peak);
}

void ducker::resetMeter()
{
    ui->levels->setLevel(0.0f);
}
