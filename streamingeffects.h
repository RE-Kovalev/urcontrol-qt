#ifndef STREAMINGEFFECTS_H
#define STREAMINGEFFECTS_H

#include <QWidget>

namespace Ui {
class StreamingEffects;
}

class StreamingEffects : public QWidget
{
    Q_OBJECT

public:
    explicit StreamingEffects(QWidget *parent = nullptr);
    ~StreamingEffects();

private:
    Ui::StreamingEffects *ui;
};

#endif // STREAMINGEFFECTS_H
