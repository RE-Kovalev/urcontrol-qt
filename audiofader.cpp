#include "audiofader.h"

#include <QMouseEvent>

AudioFader::AudioFader(QWidget *parent)
: QSlider(Qt::Vertical, parent)
{
    setRange(-600, 60); // -60.0 ... +6.0 dB
    setValue(0);
    setSingleStep(1);
    setPageStep(10);
}

void AudioFader::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        setValue(0);
        event->accept();
        return;
    }

    QSlider::mouseDoubleClickEvent(event);
}
