#ifndef AUDIOKNOB_H
#define AUDIOKNOB_H

#include <QDial>

class AudioKnob : public QDial
{
    Q_OBJECT
public:
    enum Mode
    {
        Smooth,
        Stepped
    };
    Q_ENUM(Mode)

    AudioKnob(QWidget *parent = nullptr);

    Mode mode() const;
    void setMode(Mode mode);

    int stepSize() const;
    void setStepSize(int step);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    void snapToStep();
    Mode m_mode = Smooth;
    int m_stepSize = 1;
};

#endif // AUDIOKNOB_H
