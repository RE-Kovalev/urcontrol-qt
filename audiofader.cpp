#include "audiofader.h"

#include <QMouseEvent>

AudioFader::AudioFader(QWidget *parent)
: QSlider(Qt::Vertical, parent)
{
    setRange(0, 127); // -60.0 ... +6.0 dB
    setValue(0);
    setSingleStep(1);
    setPageStep(10);

    setStyleSheet(R"(
        QSlider::groove:vertical {
            background: #15191e;
            width: 7px;
            border: 1px solid #080a0c;
            border-radius: 3px;
            margin: 8px 0;
        }

        QSlider::handle:vertical {
            background: qlineargradient(
                x1:0, y1:0, x2:1, y2:0,
                stop:0 #555b62,
                stop:0.12 #b3bac1,
                stop:0.28 #e0e4e7,
                stop:0.5 #a9afb5,
                stop:0.72 #e0e4e7,
                stop:0.88 #969da4,
                stop:1 #4c535a
            );
            border: 1px solid #353a40;
            border-radius: 3px;
            height: 28px;
            margin: 0 -17px;
        }

        QSlider::handle:vertical:hover {
            border: 1px solid #8dc4ed;
        }

        QSlider::handle:vertical:pressed {
            background: #a9b3bd;
        }
    )");
}

void AudioFader::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        setValue(103); // 0.0 dB
        event->accept();
        return;
    }

    QSlider::mouseDoubleClickEvent(event);
}
