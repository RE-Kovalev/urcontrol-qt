#ifndef GATE_H
#define GATE_H

#include "effecteditor.h"
#include "effectprotocol.h"

#include <array>

class QSlider;
class QLabel;

namespace Ui { class Gate; }

class Gate : public EffectEditor
{
    Q_OBJECT
public:
    explicit Gate(uint8_t channel, const QString& channelName, QWidget* parent = nullptr);
    ~Gate() override;

    void applyIncomingParameter(uint16_t id, int32_t value) override;
    void setMeterLevels(const urc_meter_level& level) override;
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

    Ui::Gate *ui;
    std::array<SliderBinding, 5> sliders;

    void bindSlider(int index);
    void sendSlider(int index, int value);
    void applySlider(int index, int32_t value);
    void requestInitialValues();
};

#endif // GATE_H
