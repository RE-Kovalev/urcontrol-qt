#ifndef AUDIOLEVELMETER_H
#define AUDIOLEVELMETER_H

#include <QWidget>

class AudioLevelMeter: public QWidget
{
    Q_OBJECT
public:
    explicit AudioLevelMeter(QWidget* parent = nullptr);

    // Linear peak amplitude: 0.0 (silence) ... 1.0 (0 dBFS).
    void setLevel(float amplitude);
    void setLevels(float amplitude, float peakAmplitude);
    float level() const noexcept;
    float levelDb() const noexcept;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    float m_level = 0.0f;
    float m_peakLevel = 0.0f;
    static constexpr float MinDb = -60.0f;
    static constexpr float MaxDb = 0.0f;
signals:
};

#endif // AUDIOLEVELMETER_H
