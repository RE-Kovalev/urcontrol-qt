#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <urcontrol.h>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void on_updateButton_clicked();

    void on_modelComboBox_currentTextChanged(const QString &arg1);

    void on_midiPortComboBox_currentIndexChanged(int index);

    void on_autoconnect_stateChanged(int arg1);

    void on_connectButton_released();

private:
    Ui::MainWindow *ui;
    bool initialization = true;

    bool loadModels();
    void restoreLastSelection();
    void loadAutoConnect(const QString& model);
    void refreshPorts();
    void saveLastSelection();
    void connectDevice();
    void disconnectDevice();
    void updateMixerInputs();
    void bindDAW();
    void bindMUSIC();
    void bindVoice();
    void bindREV_X();
    void bindMain();

};
#endif // MAINWINDOW_H
