#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPointer>
#include <QHBoxLayout>
#include <QTimer>
#include <QVector>
#include <array>

#include <urcontrol.h>

#include "mixerchannel.h"
#include "parameterreader.h"

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

    void on_mixerTabs_currentChanged(int index);

private:
    Ui::MainWindow *ui;
    bool initialization = true;

    using MeterFrame = std::array<urc_meter_level, URC_METER_SLOTS>;

    // This registry is independent of the actual Qt widget type and is easily
    // extendable: additional parameter groups can reuse the same read engine.
    struct ParameterBinding
    {
        QString unit;
        QString name;
        uint16_t id;
        uint8_t channel;
        QPointer<QObject> target;
        std::function<void(int32_t)> applyValue;
    };

    QVector<ParameterBinding> bindings;
    QPointer<ParameterReader> reader;
    bool disconnectRequested = false;
    int readSucceeded = 0;
    int readFailed = 0;

    QTimer meterKeepaliveTimer;
    bool metersStarted = false;

    QVector<QPointer<MixerChannel>> meterChannels[3];

    bool loadModels();
    void restoreLastSelection();
    void loadAutoConnect(const QString& model);
    void refreshPorts();
    void saveLastSelection();
    void connectDevice();
    void disconnectDevice();
    void updateMixerInputs(QHBoxLayout* inputChannels, int mix);
    void bind(const char* paramGroupName, MixerChannel* mixerChannel, int mix = 1, int channel = 0);

    bool bindParameter(MixerChannel* mixerChannel, const QString &name, MixerChannel::Control control, uint8_t channel);
    void registerBinding(const urc_parameter *parameter, uint8_t channel, QObject* target, std::function<void(int32_t)> applyValue);
    void readInitialParameters();
    void onParameterRead(int index, int result, int value);
    void onInitialReadFinished();
    void finishDisconnect();
    void updateHeadphonesMix();

    void startMeters();
    void stopMeters();

    void showMeterLevels(const MeterFrame& levels);
    void showMeterLevel(int index, const urc_meter_level& level);
    static void meterCallback(const urc_message* message, const uint8_t* raw, size_t length, void* userData);
};
#endif // MAINWINDOW_H
