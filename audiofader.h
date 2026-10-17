#ifndef AUDIOFADER_H
#define AUDIOFADER_H

#include <QSlider>

class AudioFader : public QSlider
{
public:
    AudioFader(QWidget* parent = nullptr);
protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override;
};

#endif // AUDIOFADER_H
