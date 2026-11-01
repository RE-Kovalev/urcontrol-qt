#include "mbcompressor.h"
#include "ui_mbcompressor.h"
#include "audiolevelmeter.h"
#include "device.h"

#include <QLabel>
#include <QSignalBlocker>
#include <QSlider>
#include <QVBoxLayout>
#include <cmath>

namespace
{
    // The LOW/MID/HIGH display scale is documented by Yamaha as
    // -inf, -60 dB to +18 dB. Raw codes 0..79 are an unverified
    // inference and should be checked against the device.
    constexpr EffectProtocol::Parameter low{330, 0, 79};
    constexpr EffectProtocol::Parameter mid{331, 0, 79};
    constexpr EffectProtocol::Parameter high{332, 0, 79};
    constexpr EffectProtocol::Parameter lm{342, 5, 96};
    constexpr EffectProtocol::Parameter mh{343, 18, 108};

    QString frequencyValue(int32_t value, int minimum, int maximum,
                           double minimumHz, double maximumHz)
    {
        if (value == minimum)
            return QStringLiteral("%1 Hz").arg(QString::number(minimumHz, 'f', 1));
        if (value == maximum)
            return QStringLiteral("%1 kHz").arg(QString::number(maximumHz / 1000.0, 'f', 2));
        if (value < minimum 
        ||  value > maximum)
            return QStringLiteral("raw %1").arg(value);

        const double position = static_cast<double>(value - minimum) / (maximum - minimum);
        const double frequency = minimumHz * std::pow(maximumHz / minimumHz, position);
        if (frequency >= 1000.0)
            return QStringLiteral("≈%1 kHz").arg(QString::number(frequency / 1000.0, 'f', 2));
        return QStringLiteral("≈%1 Hz").arg(QString::number(frequency, 'f', 1));
    }

    float amplitude(int16_t value)
    {
        return std::pow(10.0f, static_cast<float>(value) / 200.0f);
    }
}

mbcompressor::mbcompressor(uint8_t channel, const QString& channelName, QWidget *parent)
: EffectEditor(channel, parent)
, ui(new Ui::mbcompressor)
{
    ui->setupUi(this);
    setWindowTitle(tr("M.B.Comp - %1").arg(channelName));

    sliders = {
        { {ui->low, ui->lowValue, &low}
        , {ui->mid, ui->midValue, &mid}
        , {ui->high, ui->highValue, &high}
        , {ui->lm, ui->lmValue, &lm}
        , {ui->mh, ui->mhValue, &mh}
        }
    };

    for (int index = 0; index < static_cast<int>(sliders.size()); index++)
        bindSlider(index);

    requestInitialValues();
}

mbcompressor::~mbcompressor()
{
    delete ui;
}

void mbcompressor::bindSlider(int index)
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

void mbcompressor::sendSlider(int index, int value)
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

void mbcompressor::applySlider(int index, int32_t value)
{
    SliderBinding& binding = sliders.at(index);
    binding.confirmed = value;
    binding.initialized = true;
    if (index == 3)
        binding.label->setText(frequencyValue(value, 5, 96, 21.2, 4000.0));
    else
    if (index == 4)
        binding.label->setText(frequencyValue(value, 18, 108, 42.5, 8000.0));
    else
    if (value == 0)
        binding.label->setText(QStringLiteral("-inf dB"));
    else
    if (value >= 1 && value <= 79)
        binding.label->setText(QStringLiteral("%1 dB").arg(value - 61));
    else
        binding.label->setText(QStringLiteral("raw %1").arg(value));

    if (binding.parameter->hasRange())
    {
        const QSignalBlocker blocker(binding.slider);
        binding.slider->setValue(value);
    }
}

void mbcompressor::applyIncomingParameter(uint16_t id, int32_t value)
{
    for (int index = 0; index < static_cast<int>(sliders.size()); index++)
        if (sliders.at(index).parameter->id == id)
            applySlider(index, value);
}

void mbcompressor::requestInitialValues()
{
    urc_device* device = Device::get();
    if (!device)
        return;

    for (const SliderBinding& binding : sliders)
        urc_query_parameter(device, static_cast<uint16_t>(binding.parameter->id), protocolChannel());
}

void mbcompressor::setMeterLevels(const urc_meter_level& level)
{
    setStereoMeterLevels(level, level);
}

void mbcompressor::setStereoMeterLevels(const urc_meter_level& left, const urc_meter_level& right)
{
    ui->levels->setLevels(amplitude(left.current), amplitude(right.current), amplitude(left.peak), amplitude(right.peak));
}

void mbcompressor::resetMeter()
{
    ui->levels->setLevels(0.0f, 0.0f, 0.0f, 0.0f);
}
