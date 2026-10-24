#include "audiolevelmeter.h"

#include <QPen>
#include <QPainter>
#include <QPaintEvent>
#include <QFontMetrics>
#include <QtGlobal>

#include <algorithm>
#include <cmath>

AudioLevelMeter::AudioLevelMeter(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(minimumSizeHint());
}

void AudioLevelMeter::setLevel(float amplitude)
{
    setLevels(amplitude, amplitude);
}

void AudioLevelMeter::setLevels(float amplitude, float peakAmplitude)
{
    if (!std::isfinite(amplitude))
        amplitude = 0.0f;

    if (!std::isfinite(peakAmplitude))
        peakAmplitude = 0.0f;

    amplitude = std::clamp(amplitude, 0.0f, 1.0f);
    peakAmplitude = std::clamp(peakAmplitude, 0.0f, 1.0f);

    if (m_level == amplitude && m_peakLevel == peakAmplitude)
        return;

    m_level = amplitude;
    m_peakLevel = peakAmplitude;

    update();
}

float AudioLevelMeter::level() const noexcept
{
    return m_level;
}

float AudioLevelMeter::levelDb() const noexcept
{
    if(m_level <= 0.0f)
        return -INFINITY;

    return 20.0f * std::log10(m_level);
}

QSize AudioLevelMeter::sizeHint() const
{
    return QSize(72, 240);
}

QSize AudioLevelMeter::minimumSizeHint() const
{
    return QSize(58, 100);
}

void AudioLevelMeter::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const int top = 8;
    const int bottom = height() - 8;
    const int meterHeight = std::max(1, bottom - top);
    const QRectF track(8.0, top, 14.0, meterHeight);

    const auto yForDb = [&](float db) -> qreal
    {
        const float fraction = std::clamp((db - MinDb) / (MaxDb - MinDb), 0.0f, 1.0f);
        return bottom - fraction * meterHeight;
    };

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(38, 42, 48));
    painter.drawRoundedRect(track, 3, 3);

    const float db = levelDb();
    if(db > MinDb)
    {
        const qreal fillTop = yForDb(db);
        const QRectF fill(track.left(), fillTop, track.width(), bottom - fillTop);

        QLinearGradient gradient(track.bottomLeft(), track.topLeft());
        gradient.setColorAt(0.00, QColor(48, 188, 91));
        gradient.setColorAt(0.49, QColor(48, 188, 91));
        gradient.setColorAt(0.51, QColor(240, 194, 55));
        gradient.setColorAt(0.95, QColor(240, 194, 55));
        gradient.setColorAt(0.96, QColor(225, 65, 65));

        painter.save();
        painter.setClipRect(fill);
        painter.setBrush(gradient);
        painter.drawRoundedRect(track, 3, 3);
        painter.restore();
    }

    painter.setPen(QColor(180, 186, 193));

    if (m_peakLevel > 0.0f)
    {
        const float peakDb = 20.0f * std::log10(m_peakLevel);

        if (peakDb >= MinDb)
        {
            const qreal y = yForDb(peakDb);

            painter.setPen(QPen(QColor(245, 245, 245), 2.0));
            painter.drawLine( QPointF(track.left(), y)
                            , QPointF(track.right(), y)
                            );
        }
    }

    const QFontMetrics fm(font());
    const int ticks[] = {0, -3, -6, -12, -20, -30, -40, -60};
    for(const int tick : ticks)
    {
        const qreal y = yForDb(static_cast<float>(tick));
        painter.drawLine(QPointF(25, y), QPointF(30, y));
        const QString label = QString::number(tick);
        painter.drawText(QPointF(34, y + fm.ascent() / 2.0 - 1), label);
    }
}
