#ifndef MBCOMPRESSOR_H
#define MBCOMPRESSOR_H

#include <QDialog>

namespace Ui {
class mbcompressor;
}

class mbcompressor : public QDialog
{
    Q_OBJECT

public:
    explicit mbcompressor(QWidget *parent = nullptr);
    ~mbcompressor();

private:
    Ui::mbcompressor *ui;
};

#endif // MBCOMPRESSOR_H
