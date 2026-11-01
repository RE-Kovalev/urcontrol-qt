#ifndef COMPRESSOR_H
#define COMPRESSOR_H

#include "effecteditor.h"
#include "effectprotocol.h"

#include <QTimer>
#include <array>

class QSlider;
class QLabel;
class QShowEvent;
class QHideEvent;

namespace Ui { class Compressor; }

class Compressor : public EffectEditor
{
    Q_OBJECT
public:
    explicit Compressor(uint8_t channel, const QString& channelName, QWidget* parent = nullptr);
    ~Compressor() override;

    void applyIncomingParameter(uint16_t id, int32_t value) override;
    void setMeterLevels(const urc_meter_level& level) override;
    void resetMeter() override;

protected:
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private:
    struct SliderBinding
    {
        QSlider* slider;
        QLabel* label;
        const EffectProtocol::Parameter* parameter;
        bool initialized = false;
        int32_t confirmed = 0;
    };

    Ui::Compressor *ui;
    std::array<SliderBinding, 5> sliders;
    QTimer gainPollTimer;
    bool gainAutoInitialized = false;
    bool gainAutoConfirmed = false;
    bool kneeInitialized = false;
    int32_t kneeConfirmed = 0;

    void bindSlider(int index);
    void sendSlider(int index, int value);
    void applySlider(int index, int32_t value);
    void applyGainAuto(bool enabled);
    void sendGainAuto(bool enabled);
    void applyKnee(int32_t value);
    void sendKnee(int32_t value);
    void requestInitialValues();
    void queryGain();
    void updateGainPolling();
};

#endif // COMPRESSOR_H
