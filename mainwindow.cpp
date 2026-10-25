#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QSettings>
#include <QFileInfo>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QMetaObject>
#include <QDebug>
#include <algorithm>
#include <QCheckBox>
#include <QSignalBlocker>

#include "midiport.h"
#include "device.h"

enum MeterGroup
{
    MeterDAW,
    MeterMusic,
    MeterVoice,
    MeterRevX,
    MeterMain,
    MeterGroupCount
};

struct MeterRoute
{
    int left;
    int right;
};

// -1 означает, что индекс ещё не определён.
// Строки соответствуют Mix 1, Mix 2, Mix 3.
// Столбцы: DAW, Music, Voice, REV-X, Main.

constexpr MeterRoute kMeterRouting[2][MeterGroupCount] = {
    {
        { 6,  7},  // Mix 1: DAW
        {54, 55},  // Mix 1: Music
        {56, 57},  // Mix 1: Voice
        {30, 31},  // Mix 1: REV-X
        {10, 11}   // Mix 1: Main
    },
    {
        { 6,  7},  // Mix 3: DAW
        {54, 55},  // Mix 3: Music
        {56, 57},  // Mix 3: Voice
        {30, 31},  // Mix 3: REV-X
        {52, 53}   // Mix 3: Main
    }
};

constexpr const char* kConfigPath = "./devices.ini";
constexpr const char* kLastsPath = "./lasts";
constexpr int kMaxInputs = 6;
constexpr int kInitialReadTimeoutMs = 700;
constexpr int kMix1KnobRole = Qt::UserRole + 1;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    meterKeepaliveTimer.setInterval(2000);
    connect(&meterKeepaliveTimer, &QTimer::timeout,
            this, [this]()
            {
                if (!metersStarted)
                    return;

                urc_device* device = Device::get();

                if (!device)
                    return;

                const int result = urc_send_keepalive(device);

                if (result != URC_OK)
                {
                    qWarning() << "Meter keepalive error:"
                               << urc_strerror(result);

                    stopMeters();
                }
            });

    controlPollTimer.setInterval(500);
    connect(&controlPollTimer, &QTimer::timeout, this, &MainWindow::pollSoloMute);

    statusBar()->showMessage(tr("Disconnected"));

    bool result = loadModels();
    restoreLastSelection();
    refreshPorts();

    bind("DAWMix", ui->DAW1, 1);
    bind("MusicMix", ui->Music1, 1, 0);
    bind("MusicMix", ui->Voice1, 1, 1); // Voice on channel 2 (1 in lib - zero based) for MusicMMixI
    bind("ReverbMix", ui->REV_X1, 1);
    bind("MainMix", ui->Main1, 1);
    bind("DAWMix", ui->DAW3, 3);
    bind("MusicMix", ui->Music3, 3, 0);
    bind("MusicMix", ui->Voice3, 3, 1); // Voice on channel 2 (1 in lib - zero based) for MusicMMixI
    bind("ReverbMix", ui->REV_X3, 3);
    bind("MainMix", ui->Main3, 3);

    bindReverbOutput();

    ui->DAW1->setFaderDisabled(hasMix1Knob()); // Disable fader for mix1

    QSizePolicy policy = ui->REV_XSE1->sizePolicy();
    policy.setRetainSizeWhenHidden(true);
    ui->REV_XSE1->setSizePolicy(policy);
    ui->REV_XSE1->setVisible(false); // Disable Streaming effets for REV-X

    policy = ui->REV_XSE3->sizePolicy();
    policy.setRetainSizeWhenHidden(true);
    ui->REV_XSE3->setSizePolicy(policy);
    ui->REV_XSE3->setVisible(false); // Disable Streaming effets for REV-X

    initialization = false;
    if (!result)
        ui->connectButton->setDisabled(true);
    else
    {
        updateMixerInputs(ui->inputChannels1, 1);
        updateMixerInputs(ui->inputChannels3, 3);
        updateMixerRevs(ui->inputRev1, 1);
        updateMixerRevs(ui->inputRev3, 3);
        if (ui->autoconnect->isChecked())
            connectDevice();
    }

}

MainWindow::~MainWindow()
{
    controlPollTimer.stop();

    if (reader)
    {
        reader->requestInterruption();
        reader->wait();
    }

    if (controlPollReader)
    {
        controlPollReader->requestInterruption();
        controlPollReader->wait();
    }

    stopMeters();
    Device::close();

    delete ui;
}

bool MainWindow::loadModels()
{
    QSettings settings(kConfigPath, QSettings::IniFormat);
    if (!QFileInfo::exists(kConfigPath))
    {
        QMessageBox::warning(this, tr("Devices configuration"), tr("File ./devices.ini don't exists"));
        return false;
    }
    if (!settings.childGroups().size())
    {
        QMessageBox::warning(this, tr("Devices configuration"), tr("Devices is not found in ./devices.ini"));
        return false;
    }
    for (size_t i = 0; i < settings.childGroups().size(); i++)
    {
        settings.beginGroup(settings.childGroups().at(i));
        bool valid = false;
        const int inputs = settings.value("Inputs").toInt(&valid);
        const bool mix1Knob = settings.value("Mix1Knob", false).toBool();

        settings.endGroup();

        if (valid && inputs > 0 && inputs <= kMaxInputs)
        {
            const QString model = settings.childGroups().at(i);

            ui->modelComboBox->addItem(model, inputs);

            const int index = ui->modelComboBox->count() - 1;

            ui->modelComboBox->setItemData(index, mix1Knob, kMix1KnobRole);
        }

    }

    return true;
}

void MainWindow::restoreLastSelection()
{
    QSettings lasts(kLastsPath, QSettings::IniFormat);
    const QString model = lasts.value("Device").toString();
    const int index = ui->modelComboBox->findText(model);
    if (index >= 0)
        ui->modelComboBox->setCurrentIndex(index);

    const bool autoconnect = lasts.value("AutoConnect").toBool();
    ui->autoconnect->setChecked(autoconnect);

    const int lastMix = lasts.value("Mix", 1).toInt();
    const int mixIndex = qBound(0, lastMix - 1, ui->mixerTabs->count() - 1);

    ui->mixerTabs->setCurrentIndex(mixIndex);
}

void MainWindow::refreshPorts()
{
    const QString selected = ui->midiPortComboBox->currentData().toString();
    QSettings lasts(kLastsPath, QSettings::IniFormat);
    const QString last = lasts.value("Port").toString();
    const QSignalBlocker blocker(ui->midiPortComboBox);
    ui->midiPortComboBox->clear();
    QVector<MidiPort> ports = enumerateMidiPorts();
    for (size_t i = 0; i < ports.size(); i++)
        ui->midiPortComboBox->addItem(ports.at(i).label, ports.at(i).id);

    int index = ui->midiPortComboBox->findData(selected);
    if (index < 0)
        index = ui->midiPortComboBox->findData(last);

    if (index >= 0)
        ui->midiPortComboBox->setCurrentIndex(index);

    saveLastSelection();
}

void MainWindow::saveLastSelection()
{
    if (!initialization)
    {
        QSettings lasts(kLastsPath, QSettings::IniFormat);
        lasts.setValue("Device", ui->modelComboBox->currentText());
        lasts.setValue("Port", ui->midiPortComboBox->currentData().toString());
        lasts.setValue("AutoConnect", ui->autoconnect->isChecked());
        lasts.setValue("Mix", ui->mixerTabs->currentIndex() + 1);
        lasts.sync();
    }
}

void MainWindow::on_updateButton_clicked()
{
    refreshPorts();
}


void MainWindow::on_modelComboBox_currentTextChanged(const QString &arg1)
{
    updateMixerInputs(ui->inputChannels1, 1);
    updateMixerInputs(ui->inputChannels3, 3);
    updateMixerRevs(ui->inputRev1, 1);
    updateMixerRevs(ui->inputRev3, 3);

    ui->DAW1->setFaderDisabled(hasMix1Knob());

    saveLastSelection();
}


void MainWindow::on_midiPortComboBox_currentIndexChanged(int index)
{
    saveLastSelection();
}


void MainWindow::on_autoconnect_stateChanged(int arg1)
{
    saveLastSelection();
}

void MainWindow::connectDevice()
{
    if (Device::get()
    || ui->modelComboBox->currentIndex() < 0
    || ui->midiPortComboBox->currentIndex() < 0)
        return;

    const QByteArray port = ui->midiPortComboBox->currentData().toString().toUtf8();
    const int result = Device::open(port.constData());
    if (result != URC_OK)
    {
        Device::close();

        QMessageBox::warning(this, tr("Connection error"), QString::fromUtf8(urc_strerror(result)));
        statusBar()->showMessage(tr("Connection error"));
        return;
    }

    ui->modelComboBox->setEnabled(false);
    ui->midiPortComboBox->setEnabled(false);
    ui->updateButton->setEnabled(false);
    ui->mixerTabs->setEnabled(false);
    ui->connectButton->setText(tr("Disconnect"));
    statusBar()->showMessage(tr("Connected: ") + QString::fromUtf8(port));
    readInitialParameters();
}

void MainWindow::disconnectDevice()
{
    controlPollTimer.stop();
    disconnectRequested = true;

    if (reader)
        reader->requestInterruption();

    if (controlPollReader)
        controlPollReader->requestInterruption();

    if (reader
    || controlPollReader)
    {
        ui->connectButton->setEnabled(false);
        statusBar()->showMessage(tr("Stopping parameter read..."));
        return;
    }

    finishDisconnect();
}

void MainWindow::finishDisconnect()
{
    controlPollTimer.stop();

    if (reader || controlPollReader)
        return;

    stopMeters();
    if (urc_device* device = Device::get())
        urc_set_callback(device, nullptr, nullptr);
    Device::close();
    disconnectRequested = false;
    ui->modelComboBox->setEnabled(true);
    ui->midiPortComboBox->setEnabled(true);
    ui->updateButton->setEnabled(true);
    ui->mixerTabs->setEnabled(false);
    ui->connectButton->setEnabled(true);
    ui->connectButton->setText(tr("Connect"));
    statusBar()->showMessage(tr("Disconnected"));
}

void MainWindow::on_connectButton_released()
{
    if (Device::get())
        disconnectDevice();
    else
        connectDevice();
}

void MainWindow::updateMixerRevs(QHBoxLayout* inputChannels, int mix)
{
    if (initialization
    || !inputChannels)
        return;

    if (mix < 1 || mix > 3)
        return;

    const int inputsCount = ui->modelComboBox->currentData().toInt();

    auto& knobs = reverbKnobs[mix - 1];

    knobs.clear();

    while (QLayoutItem* item = inputChannels->takeAt(0))
    {
        delete item->widget();
        delete item;
    }

    const urc_parameter* parameter = urc_find_parameter("mixer", "InputReverbSend");

    // Only one binding per physical input is required,
    // even though the knob appears on two tabs.
    if (mix == 1)
        bindings.erase(
            std::remove_if(
                bindings.begin(),
                bindings.end(),
                [](const ParameterBinding& binding)
                {
                    return binding.unit == "mixer" && binding.name == "InputReverbSend";
                }
                ),
            bindings.end()
        );

    for (int i = 0; i < inputsCount; ++i)
    {
        QWidget* inputWidget = new QWidget();

        QHBoxLayout* inputGroup = new QHBoxLayout(inputWidget);

        inputGroup->setContentsMargins(0, 0, 0, 0);
        inputGroup->setSpacing(0);

        AudioKnob* knob = new AudioKnob(inputWidget);

        knobs.append(knob);

        inputGroup->addWidget(knob, 0, Qt::AlignHCenter);

        inputChannels->addWidget(inputWidget);

        if (!parameter)
        {
            knob->setEnabled(false);
            continue;
        }

        knob->setRange(parameter->minimum, parameter->maximum);

        // GUI -> MIDI
        connect(
            knob,
            &QDial::valueChanged,
            this,
            [this, parameter, i](int value)
            {
                urc_device* device = Device::get();

                if (!device
                ||  disconnectRequested
                ||  reader)
                    return;

                const int result = urc_change_parameter(
                    device,
                    parameter->id,
                    value,
                    static_cast<uint8_t>(i)
                    );

                if (result != URC_OK)
                {
                    qWarning() << "InputReverbSend error:"
                               << "input" << i + 1
                               << urc_strerror(result);
                    return;
                }

                // Keep both tabs synchronized.
                setInputReverbSendValue(i, value);
            }
        );

        // MIDI -> GUI
        // Register only once for each physical input.
        if (mix == 1)
            registerBinding(
                parameter,
                static_cast<uint8_t>(i),
                this,
                [this, i](int32_t value)
                {
                    setInputReverbSendValue(i, value);
                }
            );
    }

    updateReverbKnobsEnabled();
}

void MainWindow::updateMixerInputs(QHBoxLayout* inputChannels, int mix)
{
    if (!initialization)
    {
        const int inputsCount = ui->modelComboBox->currentData().toInt();

        if (mix < 1 || mix > 3)
            return;

        auto& channels = meterChannels[mix - 1];
        channels.clear();

        // Remove olds
        while(QLayoutItem* item = inputChannels->takeAt(0))
        {
            if(QWidget* widget = item->widget())
            {
                delete widget;
            }

            delete item;
        }

        bindings.erase(
            std::remove_if(
                bindings.begin(),
                bindings.end(),
                [](const ParameterBinding& binding)
                {
                    return binding.target.isNull();
                }
                ),
            bindings.end()
        );

        // Create new
        for (size_t i = 0; i < inputsCount; i++)
        {
            QWidget* inputWidget = new QWidget();
            QVBoxLayout* inputGroup = new QVBoxLayout(inputWidget);

            MixerChannel* input = new MixerChannel(inputWidget);
            bind("InputMix", input, mix, i); // i channel (Zero based)
            input->setMono(true);
            channels.append(input);
            inputGroup->addWidget(input, 1);

            QLabel* inputLabel = new QLabel(tr("Input %1").arg(i + 1), inputWidget);
            inputLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
            inputGroup->addWidget(inputLabel, 0);

            inputGroup->setContentsMargins(0, 0, 0, 0);
            inputGroup->setSpacing(0);

            inputChannels->addWidget(inputWidget);

            if (mix == 1)
                input->setFaderDisabled(hasMix1Knob()); // Disable fader for physical mix 1 knob
        }
    }
}

void MainWindow::registerBinding(const urc_parameter *parameter, uint8_t channel,
                                 QObject *target, std::function<void(int32_t)> applyValue)
{
    if (!parameter
    ||  !target
    ||  !applyValue)
        return;

    bindings.push_back({QString::fromLatin1(parameter->unit), QString::fromLatin1(parameter->name), parameter->id, channel, QPointer<QObject>(target), std::move(applyValue)});
}

bool MainWindow::bindParameter(MixerChannel* mixerChannel, const QString& name, MixerChannel::Control control, uint8_t channel)
{
    const QByteArray encodedName = name.toUtf8();
    const urc_parameter* parameter = urc_find_parameter("mixer", encodedName.constData());
    if (!parameter)
        return false;

    bool bound = false;
    switch (control)
    {
    case MixerChannel::Control::Fader:
        bound = mixerChannel->bindFaderParameter(parameter);
        break;
    case MixerChannel::Control::Mute:
        bound = mixerChannel->bindMuteParameter(parameter);
        break;
    case MixerChannel::Control::Solo:
        bound = mixerChannel->bindSoloParameter(parameter);
        break;
    case MixerChannel::Control::Pan:
        bound = mixerChannel->bindPANParameter(parameter);
        break;
    }
    if (bound)
    {
        // Only this binding needs to know the widget type and its setter.
        // The read engine itself works with protocol addresses + callbacks.
        const QPointer<MixerChannel> guarded(mixerChannel);
        registerBinding(parameter, channel, mixerChannel,
                        [guarded, control](int32_t value)
                        {
                            if (guarded)
                                guarded->applyParameterValue(control, value);
                        });
    }
    return bound;
}

void MainWindow::bind(const char* paramGroupName, MixerChannel* mixerChannel, int mix, int channel)
{

    const QString prefix = QString::fromLatin1(paramGroupName) + QString::number(mix);
    const uint8_t protocolChannel = static_cast<uint8_t>(channel);

    mixerChannel->bind_channel(channel);

    const bool fader = bindParameter(mixerChannel, prefix + "Volume", MixerChannel::Control::Fader, protocolChannel);
    const bool mute = bindParameter(mixerChannel, prefix + "Mute", MixerChannel::Control::Mute, protocolChannel);
    const bool solo = bindParameter(mixerChannel, prefix + "Solo", MixerChannel::Control::Solo, protocolChannel);
    const bool pan = bindParameter(mixerChannel, prefix + "Pan", MixerChannel::Control::Pan, protocolChannel);
    mixerChannel->setSoloVisible(solo);
    mixerChannel->setPANVisible(pan);
    mixerChannel->setDisabled(!fader || !mute);
}

void MainWindow::readInitialParameters()
{
    if (!Device::get()
    ||  reader)
        return;

    QVector<ParameterReadRequest> requests;
    requests.reserve(bindings.size());
    for (size_t i = 0; i < bindings.size(); i++)
        requests.push_back({bindings.at(i).id, bindings.at(i).channel});

    readSucceeded = 0;
    readFailed = 0;
    disconnectRequested = false;

    if (requests.isEmpty())
    {
        ui->mixerTabs->setEnabled(true);
        statusBar()->showMessage(tr("Connected: no parameters to read"));
        return;
    }

    // Hardware MIX knob: UR22C
    if (hasMix1Knob())
        requests.push_back({352, 0});

    reader = new ParameterReader(requests, kInitialReadTimeoutMs, this);
    connect(reader, &ParameterReader::parameterRead, this, &MainWindow::onParameterRead);
    connect(reader, &QThread::finished, this, &MainWindow::onInitialReadFinished);
    connect(reader, &QThread::finished, reader, &QObject::deleteLater);

    reader->start();
}

void MainWindow::onParameterRead(int index, int result, int value)
{
    // All values arrive in the GUI thread via queued Qt signals.
    if (index < 0
    ||  disconnectRequested)
        return;

    if (hasMix1Knob()  && index == bindings.size())
    {
        if (result == URC_OK)
        {
            readSucceeded++;
            applyIncomingParameter(352, 0, value);

            qDebug() << "Initial hardware MIX:" << value;
        }
        else
        {
            readFailed++;
            qWarning() << "Unable to read hardware MIX:"
                       << urc_strerror(result);

            if (result == URC_ERR_CLOSED)
                disconnectRequested = true;
        }

        return;
    }

    if (index >= bindings.size())
        return;

    const ParameterBinding &binding = bindings.at(index);
    if (result == URC_OK)
    {
        readSucceeded++;
        if (binding.target)
            binding.applyValue(value);
    }
    else
    {
        readFailed++;
        if (result == URC_ERR_CLOSED)
            disconnectRequested = true;

        qWarning().noquote() << "Unable to read"
                             << binding.unit + "/" + binding.name
                             << "channel" << static_cast<int>(binding.channel)
                             << "error" << result << urc_strerror(result);
    }

}

void MainWindow::onInitialReadFinished()
{
    // The thread has terminated: the device can now be closed safely.
    reader = nullptr;

    if (disconnectRequested)
    {
        finishDisconnect();
        return;
    }

    ui->mixerTabs->setEnabled(true);

    controlPollTimer.start();
    pollSoloMute();

    if (urc_device* device = Device::get())
        urc_set_callback(device, &MainWindow::midiCallback, this);

    startMeters();
}

void MainWindow::on_mixerTabs_currentChanged(int index)
{
    if (!initialization
    &&  index >= 0)
    {
        saveLastSelection();
        updateHeadphonesMix();
    }
}

void MainWindow::updateHeadphonesMix()
{
    if (!Device::get()
    ||  reader)
        return;

    const int mixIndex = ui->mixerTabs->currentIndex();

    if (mixIndex < 0
    ||  mixIndex >= 3)
        return;

    static const int values[] = {0, 1, 4};

    const urc_parameter* parameter = urc_find_parameter("mixer", "Headphones2Input");

    if (!parameter)
        return;

    const int result = urc_change_parameter(Device::get(), parameter->id, values[mixIndex], 0);

    if (result != URC_OK)
    {
        qWarning() << "Headphones2Input error:"
                   << result
                   << urc_strerror(result);
    }
}

void MainWindow::midiCallback(const urc_message* message, const uint8_t* raw, size_t length, void* userData)
{
    if (!message
    ||  !userData)
        return;

    MainWindow* window = static_cast<MainWindow*>(userData);

    // Входящие изменения параметров
    if (message->type == URC_MSG_CHANGE
    ||  message->type == URC_MSG_REPLY)
    {
        const uint16_t id = message->parameter;
        const uint8_t channel = message->channel;
        const int32_t value = message->value;

        qInfo() << "MIDI parameter:"
                << "type" << message->type
                << "id" << id
                << "channel" << channel
                << "value" << value;

        QMetaObject::invokeMethod(window, [window, id, channel, value]()
            {
                if (!Device::get()
                ||  window->disconnectRequested)
                    return;

                window->applyIncomingParameter(id, channel, value);
            },
            Qt::QueuedConnection
        );

        return;
    }

    if (message->type == URC_MSG_UNKNOWN)
    {
        if (raw && length > 0
        &&  length <= 64)
            qInfo().noquote()
            << "Unknown MIDI:"
            << QByteArray(
                   reinterpret_cast<const char*>(raw),
                   static_cast<qsizetype>(length)
                   ).toHex(' ');

        return;
    }

    if (message->type != URC_MSG_METER
    ||  !raw)
        return;

    MeterFrame levels{};

    for (size_t i = 0; i < levels.size(); ++i)
        if (urc_meter_decode(raw, length, i, &levels[i]) != URC_OK)
            return;

    QMetaObject::invokeMethod(
        window,
        [window, levels]()
        {
            if (!window->metersStarted)
                return;

            window->showMeterLevels(levels);
        },
        Qt::QueuedConnection
    );
}

void MainWindow::showMeterLevel(int index, const urc_meter_level& level)
{
    for (auto& channels : meterChannels)
    {
        if (index < 0 || index >= channels.size())
            continue;

        if (channels[index])
            channels[index]->setMeterLevels(level);
    }
}

void MainWindow::startMeters()
{
    if (metersStarted || !Device::get())
        return;

    if (ui->modelComboBox->currentText() != QStringLiteral("UR22C"))
        return;

    urc_device* device = Device::get();

    const int result = urc_meters_start(device);

    if (result != URC_OK)
    {
        qWarning() << "Unable to start meters:"
                   << urc_strerror(result);
        return;
    }

    metersStarted = true;
    meterKeepaliveTimer.start();
}

void MainWindow::stopMeters()
{
    meterKeepaliveTimer.stop();

    const bool wasStarted = metersStarted;
    metersStarted = false;

    urc_device* device = Device::get();

    if (device)
    {
        if (wasStarted)
        {
            const int result = urc_meters_stop(device);

            if (result != URC_OK)
                qWarning() << "Unable to stop meters:"
                           << urc_strerror(result);
        }
    }

    for (auto& channels : meterChannels)
        for (const auto& channel : channels)
            if (channel)
                channel->resetMeter();

    MixerChannel* channels[] = {
        ui->DAW1, ui->DAW3,
        ui->Music1, ui->Music3,
        ui->Voice1, ui->Voice3,
        ui->REV_X1, ui->REV_X3,
        ui->Main1, ui->Main3
    };

    for (MixerChannel* channel : channels)
    {
        if (channel)
            channel->resetMeter();
    }
}

void MainWindow::showMeterLevels(const MeterFrame& levels)
{
    showMeterLevel(0, levels[0]);
    showMeterLevel(1, levels[1]);

    MixerChannel* channels[3][MeterGroupCount] = {
        {
            ui->DAW1,
            ui->Music1,
            ui->Voice1,
            ui->REV_X1,
            ui->Main1
        },
        {
            ui->DAW3,
            ui->Music3,
            ui->Voice3,
            ui->REV_X3,
            ui->Main3
        }
    };

    for (int mix = 0; mix < 2; mix++)
    {
        for (int group = 0; group < MeterGroupCount; group++)
        {
            MixerChannel* channel = channels[mix][group];

            if (!channel)
                continue;

            const MeterRoute& route = kMeterRouting[mix][group];

            if (route.left < 0
            ||  route.left >= URC_METER_SLOTS)
                continue;

            urc_meter_level level = levels[route.left];

            if (route.right >= 0
            &&  route.right < URC_METER_SLOTS)
            {
                const urc_meter_level& right = levels[route.right];

                level.current = std::max(level.current, right.current);

                level.peak = std::max(level.peak, right.peak);
            }

            channel->setMeterLevels(level);
        }
    }
}

void MainWindow::applyIncomingParameter(uint16_t id, uint8_t channel, int32_t value)
{
    // Physical mix knob on device
    if (id == 352
    &&  channel == 0)
    {
        // Ignore hardware MIX for devices without Mix1Knob
        if (!hasMix1Knob())
            return;

        const int mix = std::clamp<int>(value, -63, 63);

        // 103 = 0 dB, 0 = -inf dB.
        constexpr int maxVolume = 103;

        int inputVolume = maxVolume;
        int dawVolume = maxVolume;

        if (mix < 0)
            dawVolume = std::lround(maxVolume * (mix + 63) / 63.0);
        else if (mix > 0)
            inputVolume = std::lround(maxVolume * (63 - mix) / 63.0);

        for (const ParameterBinding& binding : bindings)
        {
            if (!binding.target || binding.unit != "mixer")
                continue;

            if (binding.id == 12)
                // InputMix1Volume
                binding.applyValue(inputVolume);
            else
            if (binding.id == 21 && binding.channel == 0)
                // DAWMix1Volume.
                binding.applyValue(dawVolume);
        }

        return;
    }

    for (const ParameterBinding& binding : bindings)
    {
        if (binding.id != id
        ||  binding.channel != channel
        ||  !binding.target)
            continue;

        binding.applyValue(value);
    }
}

void MainWindow::pollSoloMute()
{
    if (!Device::get()
    ||  disconnectRequested
    ||  reader
    ||  controlPollReader)
        return;

    QVector<ParameterReadRequest> requests;
    QVector<int> bindingIndices;

    for (int i = 0; i < bindings.size(); ++i)
    {
        const ParameterBinding& binding = bindings.at(i);

        if (!binding.target
        ||  binding.unit != "mixer")
            continue;

        if (!binding.name.endsWith("Solo")
        &&  !binding.name.endsWith("Mute"))
            continue;

        if (binding.name.contains("Mix2"))
            continue;

        requests.push_back({binding.id, binding.channel});

        bindingIndices.push_back(i);
    }

    if (requests.isEmpty())
        return;

    auto* poller = new ParameterReader(requests, 150, this);

    controlPollReader = poller;

    connect(
        poller,
        &ParameterReader::parameterRead,
        this,
        [this, bindingIndices](int index, int result, int value)
        {
            if (disconnectRequested
            ||  index < 0
            ||  index >= bindingIndices.size())
                return;

            const ParameterBinding& binding = bindings.at(bindingIndices.at(index));

            if (result != URC_OK)
            {
                qWarning() << "Solo/Mute poll failed:"
                           << binding.name
                           << "channel" << binding.channel
                           << "result" << result;
                return;
            }

            if (!binding.target)
                return;

            binding.applyValue(value);
        }
    );

    connect(
        poller,
        &QThread::finished,
        this,
        [this, poller]()
        {
            if (controlPollReader == poller)
                controlPollReader.clear();

            if (disconnectRequested && !reader)
                finishDisconnect();
        }
    );

    connect(poller, &QThread::finished, poller, &QObject::deleteLater);

    poller->start();
}

bool MainWindow::hasMix1Knob() const
{
    return ui->modelComboBox->currentData(kMix1KnobRole).toBool();
}

void MainWindow::setReverbOutputValue(int32_t value)
{
    reverbOutputValue = value;

    const QSignalBlocker block1(ui->REV_X1Send);
    const QSignalBlocker block3(ui->REV_X3Send);

    ui->REV_X1Send->setChecked(value == 0);
    ui->REV_X3Send->setChecked(value == 4);

    updateReverbKnobsEnabled();
}

void MainWindow::bindReverbOutput()
{
    const urc_parameter* parameter = urc_find_parameter("mixer", "ReverbOutput");

    if (!parameter)
    {
        ui->REV_X1Send->setEnabled(false);
        ui->REV_X3Send->setEnabled(false);
        return;
    }

    registerBinding(
        parameter,
        0,
        this,
        [this](int32_t value)
        {
            setReverbOutputValue(value);
        }
    );

    const auto selectOutput =
        [this, parameter](int output, bool checked)
    {
        if (!checked)
        {
            setReverbOutputValue(reverbOutputValue);
            return;
        }

        urc_device* device = Device::get();

        if (!device
        ||  disconnectRequested
        ||  reader)
        {
            setReverbOutputValue(reverbOutputValue);
            return;
        }

        const int result = urc_change_parameter(device, parameter->id, output, 0);

        if (result != URC_OK)
        {
            qWarning() << "ReverbOutput error:"
                       << urc_strerror(result);

            setReverbOutputValue(reverbOutputValue);
            return;
        }

        setReverbOutputValue(output);
    };

    connect(
        ui->REV_X1Send,
        &QCheckBox::clicked,
        this,
        [selectOutput](bool checked)
        {
            selectOutput(0, checked);
        }
    );

    connect(
        ui->REV_X3Send,
        &QCheckBox::clicked,
        this,
        [selectOutput](bool checked)
        {
            selectOutput(4, checked);
        }
    );
}

void MainWindow::setInputReverbSendValue(int input, int32_t value)
{
    for (auto& knobs : reverbKnobs)
    {
        if (input < 0
        ||  input >= knobs.size())
            continue;

        AudioKnob* knob = knobs[input];

        if (!knob)
            continue;

        const QSignalBlocker blocker(knob);
        knob->setValue(value);
    }
}

void MainWindow::updateReverbKnobsEnabled()
{
    const bool mix1Enabled = reverbOutputValue == 0;
    const bool mix3Enabled = reverbOutputValue == 4;

    for (const auto& knob : reverbKnobs[0])
        if (knob)
            knob->setEnabled(mix1Enabled);

    for (const auto& knob : reverbKnobs[1])
        if (knob)
            knob->setEnabled(mix3Enabled);

    for (const auto& knob : reverbKnobs[2])
        if (knob)
            knob->setEnabled(mix3Enabled);
}
