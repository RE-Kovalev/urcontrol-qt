#ifndef MBCOMPRESSOR_H
#define MBCOMPRESSOR_H

#include "effecteditor.h"
#include "effectprotocol.h"
#include <array>

class QLabel;
class QSlider;
class AudioLevelMeter;

namespace Ui {
class mbcompressor;
}

class mbcompressor : public EffectEditor
{
    Q_OBJECT

public:
    explicit mbcompressor(uint8_t channel, const QString& channelName, QWidget *parent = nullptr);
    ~mbcompressor() override;

    void applyIncomingParameter(uint16_t id, int32_t value) override;
    void setMeterLevels(const urc_meter_level& level) override;
    void setStereoMeterLevels(const urc_meter_level& left, const urc_meter_level& right) override;
    void resetMeter() override;

private:
    struct SliderBinding
    {
        QSlider* slider;
        QLabel* label;
        const EffectProtocol::Parameter* parameter;
        bool initialized = false;
        int32_t confirmed = 0;
    };

    Ui::mbcompressor *ui;
    std::array<SliderBinding, 5> sliders;

    void bindSlider(int index);
    void sendSlider(int index, int value);
    void applySlider(int index, int32_t value);
    void requestInitialValues();
};

#endif // MBCOMPRESSOR_H
