#ifndef MIXERCHANNEL_H
#define MIXERCHANNEL_H

#include <QWidget>

namespace Ui {
class MixerChannel;
}

class MixerChannel : public QWidget
{
    Q_OBJECT

public:
    explicit MixerChannel(QWidget *parent = nullptr);
    ~MixerChannel();

    void setLevel(float level);
    void setVolume(int volume);
    void setMute(bool muted);

private:
    Ui::MixerChannel *ui;
};

#endif // MIXERCHANNEL_H
