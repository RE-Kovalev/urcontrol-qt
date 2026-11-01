#ifndef DUCKER_H
#define DUCKER_H

#include "effecteditor.h"
#include "effectprotocol.h"

#include <array>
#include <QDialog>

class QLabel;
class QSlider;
class QCheckBox;

namespace Ui {
class ducker;
}

class ducker : public EffectEditor
{
    Q_OBJECT

public:
    explicit ducker(uint8_t channel, const QString& channelName, QWidget *parent = nullptr);
    ~ducker() override;

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

    struct SourceBinding
    {
        QCheckBox* checkbox;
        const EffectProtocol::Parameter* parameter;
        bool initialized = false;
        int32_t confirmed = 0;
    };

    Ui::ducker *ui;
    std::array<SliderBinding, 4> sliders;
    std::array<SourceBinding, 3> sources;

    void bindSlider(int index);
    void bindSource(int index);
    void sendSlider(int index, int value);
    void sendSource(int index, bool checked);
    void applySlider(int index, int32_t value);
    void applySource(int index, int32_t value);
    void requestInitialValues();
};

#endif // DUCKER_H
