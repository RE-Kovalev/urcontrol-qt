#ifndef DUCKER_H
#define DUCKER_H

#include <QDialog>

namespace Ui {
class ducker;
}

class ducker : public QDialog
{
    Q_OBJECT

public:
    explicit ducker(QWidget *parent = nullptr);
    ~ducker();

private:
    Ui::ducker *ui;
};

#endif // DUCKER_H
