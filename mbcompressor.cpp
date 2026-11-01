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
    constexpr EffectProtocol::Parameter low{330, 0, 56};
    constexpr EffectProtocol::Parameter mid{335, 0, 56};
    constexpr EffectProtocol::Parameter high{340, 0, 56};
    constexpr EffectProtocol::Parameter lm{342, 5, 96};
    constexpr EffectProtocol::Parameter mh{343, 17, 108};

    struct FrequencyPoint
    {
        int code;
        double hertz;
    };

    // Measured UR22C crossover frequencies. M-H shares the scale from
    // 42.5 Hz to 4 kHz, then continues to 8 kHz.
    constexpr FrequencyPoint frequencyPoints[] = {
        {5, 21.2}, {7, 23.6}, {8, 25.0}, {9, 26.5}, {10, 28.0},
        {11, 30.0}, {12, 31.5}, {13, 33.5}, {15, 37.5}, {16, 40.0},
        {17, 42.5}, {20, 50.0}, {21, 53.0}, {22, 56.0}, {23, 60.0},
        {24, 63.0}, {25, 67.0}, {27, 75.0}, {28, 80.0}, {32, 100.0},
        {33, 106.0}, {35, 118.0}, {36, 125.0}, {37, 132.0}, {38, 140.0},
        {39, 150.0}, {44, 200.0}, {45, 212.0}, {47, 236.0}, {48, 250.0},
        {49, 265.0}, {50, 280.0}, {51, 300.0}, {52, 315.0}, {53, 335.0},
        {55, 375.0}, {56, 400.0}, {60, 500.0}, {61, 530.0}, {62, 560.0},
        {63, 600.0}, {64, 630.0}, {65, 670.0}, {67, 750.0}, {68, 800.0},
        {72, 1000.0}, {74, 1060.0}, {75, 1180.0}, {76, 1250.0},
        {77, 1320.0}, {78, 1400.0}, {79, 1500.0}, {84, 2000.0},
        {85, 2120.0}, {87, 2360.0}, {88, 2500.0}, {89, 2650.0},
        {90, 2800.0}, {91, 3000.0}, {92, 3150.0}, {93, 3350.0},
        {95, 3750.0}, {96, 4000.0}, {100, 5000.0}, {101, 5300.0},
        {102, 5600.0}, {103, 6000.0}, {104, 6300.0}, {105, 6700.0},
        {107, 7500.0}, {108, 8000.0}
    };

    QString displayFrequency(double hertz, bool approximate)
    {
        const QString prefix = approximate ? QStringLiteral("≈") : QString();
        if (hertz >= 1000.0)
            return QStringLiteral("%1%2 kHz").arg(prefix, QString::number(hertz / 1000.0, 'f', 2));
        const int precision = hertz == std::floor(hertz) ? 0 : 1;
        return QStringLiteral("%1%2 Hz").arg(prefix, QString::number(hertz, 'f', precision));
    }

    QString frequencyValue(int32_t value, int minimum, int maximum,
                           double minimumHz, double maximumHz)
    {
        if (value < minimum
        ||  value > maximum)
            return QStringLiteral("raw %1").arg(value);

        if (value == minimum)
            return displayFrequency(minimumHz, false);
        if (value == maximum)
            return displayFrequency(maximumHz, false);

        for (const FrequencyPoint& point : frequencyPoints)
            if (value == point.code)
                return displayFrequency(point.hertz, false);

        for (std::size_t index = 1; index < sizeof(frequencyPoints) / sizeof(frequencyPoints[0]); index++)
        {
            const FrequencyPoint& left = frequencyPoints[index - 1];
            const FrequencyPoint& right = frequencyPoints[index];
            if (value > left.code
            &&  value < right.code)
            {
                const double t = static_cast<double>(value - left.code) / (right.code - left.code);
                const double hertz = std::exp(std::log(left.hertz) + t * (std::log(right.hertz) - std::log(left.hertz)));
                return displayFrequency(hertz, true);
            }
        }
        return QStringLiteral("raw %1").arg(value);
    }

    QString gainValue(int32_t value)
    {
        if (value == 0)
            return QStringLiteral("-inf dB");
        if (value >= 1 && value <= 13)
            return QStringLiteral("%1 dB").arg(-60 + (value - 1) * 3);
        if (value == 14)
            return QStringLiteral("-23 dB");
        if (value == 56)
            return QStringLiteral("18 dB");
        if (value > 14 && value < 56)
        {
            // The two measured endpoints differ by 41 dB over 42 codes.
            // Exact intermediate steps are not yet known.
            const double decibels = -23.0 + (value - 14) * (41.0 / 42.0);
            return QStringLiteral("≈%1 dB").arg(QString::number(decibels, 'f', 1));
        }
        return QStringLiteral("raw %1").arg(value);
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
        binding.label->setText(frequencyValue(value, 17, 108, 42.5, 8000.0));
    else
        binding.label->setText(gainValue(value));

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
