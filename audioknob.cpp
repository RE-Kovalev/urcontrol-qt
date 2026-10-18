#include "audioknob.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QSignalBlocker>
#include <QWheelEvent>
#include <QtMath>

#include <algorithm>
#include <cmath>

AudioKnob::AudioKnob(QWidget *parent)
    : QDial(parent)
{
    setNotchesVisible(false);
    setWrapping(false);
    setRange(0, 100);
    setValue(0);
    setMinimumSize(48, 48);

    connect(this, &QDial::valueChanged, this, [this](int)
            {
                if (m_mode == Stepped)
                    snapToStep();
            });
}

AudioKnob::Mode AudioKnob::mode() const
{
    return m_mode;
}

void AudioKnob::setMode(Mode mode)
{
    if (m_mode == mode)
        return;

    m_mode = mode;
    snapToStep();
    update();
}

int AudioKnob::stepSize() const
{
    return m_stepSize;
}

void AudioKnob::setStepSize(int step)
{
    m_stepSize = std::max(1, step);
    setSingleStep(m_stepSize);
    snapToStep();
    update();
}

void AudioKnob::snapToStep()
{
    if (m_mode != Stepped)
        return;

    const int offset = value() - minimum();
    const int snapped = minimum() + static_cast<int>(std::lround(static_cast<double>(offset) / m_stepSize)) * m_stepSize;
    const int bounded = std::clamp(snapped, minimum(), maximum());

    if(bounded != value())
    {
        // Avoid recursive valueChanged signals while correcting a raw QDial value.
        QSignalBlocker blocker(this);
        setValue(bounded);
    }
}

void AudioKnob::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const QRectF area = rect().adjusted(7, 7, -7, -7);
    const qreal side = std::min(area.width(), area.height());
    if (side <= 0)
        return;

    const QPointF center = area.center();
    const qreal radius = side * 0.39;
    const qreal start = 225.0;
    const qreal sweep = 270.0;
    const qreal fraction = maximum() > minimum() ? static_cast<qreal>(value() - minimum()) / (maximum() - minimum()) : 0.0;

    painter.setPen(QPen(palette().mid().color(), 3, Qt::SolidLine, Qt::RoundCap));
    painter.setBrush(Qt::NoBrush);
    painter.drawArc( QRectF( center.x() - radius
                           , center.y() - radius
                           , 2 * radius
                           , 2 * radius
                           )
                    , static_cast<int>(start * 16)
                    , static_cast<int>(-sweep * 16)
                    );

    const qreal knobRadius = radius * 0.79;
    painter.setPen(QPen(palette().dark().color(), 1));
    painter.setBrush(palette().button());
    painter.drawEllipse(center, knobRadius, knobRadius);

    const qreal angle = qDegreesToRadians(start - sweep * fraction);
    const QPointF tip(center.x() + std::cos(angle) * knobRadius * 0.75, center.y() - std::sin(angle) * knobRadius * 0.75);
    painter.setPen(QPen(palette().buttonText().color(), 3, Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(center, tip);

    if(m_mode == Stepped && maximum() > minimum())
    {
        const int count = (maximum() - minimum()) / m_stepSize;
        if(count <= 100)
        {
            painter.setPen(QPen(palette().text().color(), 1));
            for(int i = 0; i <= count; ++i)
            {
                const qreal ratio = static_cast<qreal>(i * m_stepSize) / (maximum() - minimum());
                const qreal a = qDegreesToRadians(start - sweep * ratio);
                const QPointF inner( center.x() + std::cos(a) * radius * 1.16
                                   , center.y() - std::sin(a) * radius * 1.16
                                   );
                const QPointF outer( center.x() + std::cos(a) * radius * 1.29
                                   , center.y() - std::sin(a) * radius * 1.29
                                   );
                painter.drawLine(inner, outer);
            }
        }
    }
}

void AudioKnob::mousePressEvent(QMouseEvent *event)
{
    QDial::mousePressEvent(event);
    snapToStep();
}

void AudioKnob::mouseMoveEvent(QMouseEvent *event)
{
    QDial::mouseMoveEvent(event);
    snapToStep();
}

void AudioKnob::mouseReleaseEvent(QMouseEvent *event)
{
    QDial::mouseReleaseEvent(event);
    snapToStep();
}
void AudioKnob::keyPressEvent(QKeyEvent *event)
{
    QDial::keyPressEvent(event);
    snapToStep();
}

void AudioKnob::wheelEvent(QWheelEvent *event)
{
    QDial::wheelEvent(event);
    snapToStep();
}
