#ifndef EFFECTEDITOR_H
#define EFFECTEDITOR_H

#include <QDialog>
#include <QString>
#include <cstdint>
#include <urcontrol.h>

// A protocol-channel-specific editor. All effect families can share the
// MainWindow MIDI and meter dispatch without knowing each other's controls.
class EffectEditor : public QDialog
{
public:
    explicit EffectEditor(uint8_t channel, QWidget* parent = nullptr)
        : QDialog(parent)
        , channel_(channel)
    {

    }

    uint8_t protocolChannel() const noexcept { return channel_; }

    virtual void applyIncomingParameter(uint16_t id, int32_t value) = 0;
    virtual void setMeterLevels(const urc_meter_level& value) = 0;
    virtual void setStereoMeterLevels(const urc_meter_level& left, const urc_meter_level& right)
    {
        setMeterLevels(left);
    }
    virtual void resetMeter() = 0;

private:
    uint8_t channel_;
};

#endif // EFFECTEDITOR_H
