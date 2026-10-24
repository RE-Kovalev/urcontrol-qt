#include "audiolevelmeter.h"

#include <QPainter>
#include <QPaintEvent>
#include <QFontMetrics>
#include <QLinearGradient>

#include <algorithm>
#include <cmath>

AudioLevelMeter::AudioLevelMeter(QWidget* parent)
: QWidget(parent)
{
    setMinimumSize(minimumSizeHint());
}

void AudioLevelMeter::setLevel(float amplitude)
{
    setLevels(amplitude, amplitude, amplitude, amplitude);
}

void AudioLevelMeter::setLevels(float left, float right, float leftPeak, float rightPeak)
{
    const auto normalize = [](float value)
    {
        if (!std::isfinite(value))
            return 0.0f;

        return std::clamp(value, 0.0f, 1.0f);
    };

    left = normalize(left);
    right = normalize(right);
    leftPeak = normalize(leftPeak);
    rightPeak = normalize(rightPeak);

    if (m_leftLevel == left
    &&  m_rightLevel == right
    &&  m_leftPeak == leftPeak
    &&  m_rightPeak == rightPeak)
        return;

    m_leftLevel = left;
    m_rightLevel = right;
    m_leftPeak = leftPeak;
    m_rightPeak = rightPeak;

    update();
}

void AudioLevelMeter::setMono(bool mono)
{
    if (m_mono == mono)
        return;

    m_mono = mono;
    update();
}

float AudioLevelMeter::level() const noexcept
{
    return std::max(m_leftLevel, m_rightLevel);
}

float AudioLevelMeter::levelDb() const noexcept
{
    const float amplitude = level();

    if (amplitude <= 0.0f)
        return -INFINITY;

    return 20.0f * std::log10(amplitude);
}

QSize AudioLevelMeter::sizeHint() const
{
    return QSize(80, 240);
}

QSize AudioLevelMeter::minimumSizeHint() const
{
    return QSize(74, 100);
}

void AudioLevelMeter::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const qreal top = 20.0;
    const qreal bottom = height() - 8.0;
    const qreal meterHeight = std::max<qreal>(1.0, bottom - top);

    const auto yForDb = [&](float db) -> qreal
    {
        const float fraction = std::clamp((db - MinDb) / (MaxDb - MinDb), 0.0f, 1.0f);

        return bottom - fraction * meterHeight;
    };

    const auto drawMeter = [&](const QRectF& track, float amplitude, float peakAmplitude)
    {
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(38, 42, 48));
        painter.drawRoundedRect(track, 3, 3);

        if (amplitude > 0.0f)
        {

            const float db = 20.0f * std::log10(amplitude);

            if (db >= MinDb)
            {
                const qreal fillTop = yForDb(db);

                const QRectF fill(track.left(), fillTop, track.width(), bottom - fillTop);

                QLinearGradient gradient(track.bottomLeft(), track.topLeft());

                gradient.setColorAt(0.00, QColor(48, 188, 91));
                gradient.setColorAt(0.49, QColor(48, 188, 91));
                gradient.setColorAt(0.51, QColor(240, 194, 55));
                gradient.setColorAt(0.95, QColor(240, 194, 55));
                gradient.setColorAt(0.96, QColor(225, 65, 65));
                gradient.setColorAt(0.96, QColor(225, 0, 0));

                painter.save();
                painter.setClipRect(fill);
                painter.setBrush(gradient);
                painter.drawRoundedRect(track, 3, 3);
                painter.restore();
            }
        }
        if (peakAmplitude > 0.0f)
        {
            const float peakDb =
                20.0f * std::log10(peakAmplitude);

            if (peakDb >= MinDb)
            {
                const qreal y = yForDb(peakDb);

                painter.save();
                painter.setPen(QPen(QColor(245, 245, 245), 1.5));

                painter.drawLine(QPointF(track.left() + 0.5, y), QPointF(track.right() - 0.5, y));

                painter.restore();
            }
        }
    };

    painter.setPen(QColor(180, 186, 193));

    if (m_mono)
    {
        const QRectF track(12.0, top, 11.0, meterHeight);

        drawMeter(track, std::max(m_leftLevel, m_rightLevel), std::max(m_leftPeak, m_rightPeak));

        painter.setPen(QColor(180, 186, 193));
        painter.drawText(QRectF(6.0, 2.0, 25.0, 16.0), Qt::AlignCenter, QStringLiteral("M"));
    }
    else
    {
        const QRectF leftTrack(6.0, top, 11.0, meterHeight);
        const QRectF rightTrack(20.0, top, 11.0, meterHeight);

        drawMeter(leftTrack, m_leftLevel, m_leftPeak);
        drawMeter(rightTrack, m_rightLevel, m_rightPeak);

        painter.setPen(QColor(180, 186, 193));

        painter.drawText(QRectF(6.0, 2.0, 11.0, 16.0), Qt::AlignCenter, QStringLiteral("L"));

        painter.drawText(QRectF(20.0, 2.0, 11.0, 16.0), Qt::AlignCenter, QStringLiteral("R"));
    }

    painter.setPen(QColor(180, 186, 193));

    const QFontMetrics fm(font());
    const int ticks[] = {0, -3, -6, -12, -20, -30, -40, -60};

    for (const int tick : ticks)
    {
        const qreal y = yForDb(static_cast<float>(tick));

        painter.drawLine(QPointF(34, y), QPointF(39, y));

        painter.drawText(QPointF(42, y + fm.ascent() / 2.0 - 1), QString::number(tick));
    }
}