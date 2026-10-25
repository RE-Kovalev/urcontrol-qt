#ifndef STREAMINGEFFECTS_H
#define STREAMINGEFFECTS_H

#include <QStringList>
#include <QWidget>

namespace Ui {
class StreamingEffects;
}

class StreamingEffects : public QWidget
{
    Q_OBJECT

public:
    explicit StreamingEffects(QWidget *parent = nullptr);
    ~StreamingEffects() override;

    void setVariants(const QStringList& variants);
    void setEffectType(int type);
    void setEffectEnabled(bool enabled);
    void setEffectAvailable(bool available);

    int confirmedEffectType() const { return confirmedType; }
    bool confirmedEffectEnabled() const { return confirmedEnabled; }

signals:
    void effectTypeChanged(int type);
    void effectEnabledChanged(bool enabled);

private:
    void updateEnabledControl();

    Ui::StreamingEffects *ui;
    int confirmedType = 0;
    bool confirmedEnabled = false;
    bool available = false;
};

#endif // STREAMINGEFFECTS_H
