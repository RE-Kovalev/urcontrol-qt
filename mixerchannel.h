#ifndef MIXERCHANNEL_H
#define MIXERCHANNEL_H

#include <QWidget>

#include <urcontrol.h>

namespace Ui {
class MixerChannel;
}

class MixerChannel : public QWidget
{
    Q_OBJECT

public:
    explicit MixerChannel(QWidget *parent = nullptr);
    ~MixerChannel();

    void bind_channel(uint8_t _channel) { channel = _channel; }

    bool bindFaderParameter(const urc_parameter* parameter);
    bool bindMuteParameter(const urc_parameter* parameter);
    bool bindSoloParameter(const urc_parameter* parameter);
    bool bindPANParameter(const urc_parameter* parameter);

    void setFaderValue(int32_t value);
    void setMute(bool muted);
    void setSolo(bool solo);
    void setPam(int32_t value);

    void setSoloVisible(bool visibility);
    void setPANVisible(bool visibility);
signals:
    void parameterWriteFailed(int error);

private:
    Ui::MixerChannel *ui;

    const urc_parameter* faderParameter = nullptr;
    const urc_parameter* muteParameter = nullptr;
    const urc_parameter* soloParameter = nullptr;
    const urc_parameter* PANParameter = nullptr;
    uint8_t channel = 0;
};

#endif // MIXERCHANNEL_H
