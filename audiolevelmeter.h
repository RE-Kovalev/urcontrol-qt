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
    void setLevels(float left, float right, float leftPeak, float rightPeak);
    float level() const noexcept;
    float levelDb() const noexcept;

    void setMono(bool mono);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    float m_leftLevel = 0.0f;
    float m_rightLevel = 0.0f;
    float m_leftPeak = 0.0f;
    float m_rightPeak = 0.0f;
    bool m_mono = false;
    static constexpr float MinDb = -60.0f;
    static constexpr float MaxDb = 0.0f;
signals:
};

#endif // AUDIOLEVELMETER_H
