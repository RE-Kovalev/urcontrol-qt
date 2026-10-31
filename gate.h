#ifndef GATE_H
#define GATE_H

#include <QDialog>

namespace Ui {
class Gate;
}

class Gate : public QDialog
{
    Q_OBJECT

public:
    explicit Gate(QWidget *parent = nullptr);
    ~Gate();

private:
    Ui::Gate *ui;
};

#endif // GATE_H
