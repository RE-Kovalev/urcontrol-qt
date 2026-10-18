#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QSettings>
#include <QFileInfo>
#include <QMessageBox>
#include <QVBoxLayout>

#include "midiport.h"
#include "device.h"


constexpr const char* kConfigPath = "./devices.ini";
constexpr const char* kLastsPath = "./lasts";
constexpr int kMaxInputs = 6;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    statusBar()->showMessage(tr("Disconnected"));

    bool result = loadModels();
    restoreLastSelection();
    refreshPorts();

    bind("DAWMix", ui->DAW);
    bind("MusicMix", ui->Music, 1, 0);
    bind("MusicMix", ui->Voice, 1, 1); // Voice on channel 2 (1 in lib - zero based) for MusicMMixI
    bind("ReverbMix", ui->REV_X);
    bind("MainMix", ui->Main);

    initialization = false;
    if (!result)
        ui->connectButton->setDisabled(true);
    else
    {
        updateMixerInputs();
        if (ui->autoconnect->isChecked())
            connectDevice();
    }

}

MainWindow::~MainWindow()
{
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
        settings.endGroup();
        if (valid && inputs > 0 && inputs <= kMaxInputs)
            ui->modelComboBox->addItem(settings.childGroups().at(i), inputs);

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
        lasts.sync();
    }
}

void MainWindow::on_updateButton_clicked()
{
    refreshPorts();
}


void MainWindow::on_modelComboBox_currentTextChanged(const QString &arg1)
{
    updateMixerInputs();
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
    ui->mixerArea->setEnabled(true);
    ui->connectButton->setText(tr("Disconnect"));
    statusBar()->showMessage(tr("Connected: ") + QString::fromUtf8(port));
}

void MainWindow::disconnectDevice()
{
    Device::close();
    ui->modelComboBox->setEnabled(true);
    ui->midiPortComboBox->setEnabled(true);
    ui->updateButton->setEnabled(true);
    ui->mixerArea->setEnabled(false);
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

void MainWindow::updateMixerInputs()
{
    if (!initialization)
    {
        const int inputsCount = ui->modelComboBox->currentData().toInt();

        // Remove olds
        while(QLayoutItem* item = ui->inputChannels->takeAt(0))
        {
            if(QWidget* widget = item->widget())
            {
                delete widget;
            }

            delete item;
        }
        // Create new
        for (size_t i = 0; i < inputsCount; i++)
        {
            QWidget* inputWidget = new QWidget();
            QVBoxLayout* inputGroup = new QVBoxLayout(inputWidget);

            MixerChannel* input = new MixerChannel(inputWidget);
            bind("InputMix", input, 1, i); // i channel (Zero based)
            inputGroup->addWidget(input, 1);

            QLabel* inputLabel = new QLabel(tr("Input %1").arg(i + 1), inputWidget);
            inputLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
            inputGroup->addWidget(inputLabel, 0);

            inputGroup->setContentsMargins(0, 0, 0, 0);
            inputGroup->setSpacing(0);

            ui->inputChannels->addWidget(inputWidget);
        }
    }
}

void MainWindow::bind(const char* paramGroupName, MixerChannel* mixerChannel, int mix, int channel)
{
    QString faderParameterName = QString(paramGroupName) + QString("%1Volume").arg(mix);
    const urc_parameter *faderParameter = urc_find_parameter( "mixer"
                                                             , faderParameterName.toUtf8().constData()
                                                             );
    QString muteParameterName = QString(paramGroupName) + QString("%1Mute").arg(mix);
    const urc_parameter *muteParameter = urc_find_parameter( "mixer"
                                                            , muteParameterName.toUtf8().constData()
                                                            );
    QString soloParameterName = QString(paramGroupName) + QString("%1Solo").arg(mix);
    const urc_parameter *soloParameter = urc_find_parameter( "mixer"
                                                            , soloParameterName.toUtf8().constData()
                                                            );
    QString PANParameterName = QString(paramGroupName) + QString("%1Pan").arg(mix);
    const urc_parameter *PANParameter = urc_find_parameter( "mixer"
                                                           , PANParameterName.toUtf8().constData()
                                                           );

    mixerChannel->bind_channel(channel);

    if (!mixerChannel->bindSoloParameter(soloParameter))
        mixerChannel->setSoloVisible(false);
    if (!mixerChannel->bindPANParameter(PANParameter))
        mixerChannel->setPANVisible(false);

    if (!mixerChannel->bindFaderParameter(faderParameter)
    ||  !mixerChannel->bindMuteParameter(muteParameter))
        mixerChannel->setDisabled(true);
}
