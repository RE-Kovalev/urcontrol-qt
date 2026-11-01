#include "streamingeffects.h"
#include "ui_streamingeffects.h"
#include "effecteditor.h"

#include <QCheckBox>
#include <QComboBox>
#include <QSignalBlocker>
#include <QPushButton>
#include <utility>

StreamingEffects::StreamingEffects(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::StreamingEffects)
{
    ui->setupUi(this);

    connect(ui->enabled, &QCheckBox::toggled, this, &StreamingEffects::effectEnabledChanged);

    connect(ui->variants, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index)
            {
                if (index < 0)
                    return;
                if (variantIndex != index)
                {
                    closeEditor();
                    variantIndex = index;
                }

                updateEditorButton();
                updateEnabledControl();
                emit effectTypeChanged(ui->variants->itemData(index).toInt());
            }
        );
    connect(ui->editButton, &QPushButton::released,
            this, [this]()
            {
                if (edited)
                {
                    closeEditor();
                    return;
                }

                if (!available 
                ||  !editorFactory)
                    return;

                const int type = ui->variants->currentData().toInt();
                EffectEditor* editor = editorFactory(type, protocolChannel, channelTitle, this);
                if (!editor)
                    return;

                edited = editor;
                edited->setAttribute(Qt::WA_DeleteOnClose);
                edited->setModal(false);
                edited->show();
            }
        );
    setEffectAvailable(false);
}

StreamingEffects::~StreamingEffects()
{
    closeEditor();
    delete ui;
}

void StreamingEffects::closeEditor()
{
    if (edited)
    {
        edited->close();
        edited = nullptr;
    }
}

void StreamingEffects::setChannel(uint8_t channel, const QString& channelName)
{
    if (protocolChannel != channel 
    ||  channelTitle != channelName)
        closeEditor();
    protocolChannel = channel;
    channelTitle = channelName;
}

void StreamingEffects::setEditorFactory(EditorFactory factory)
{
    closeEditor();
    editorFactory = std::move(factory);
    updateEditorButton();
}

void StreamingEffects::updateEditorButton()
{
    ui->editButton->setEnabled(available && editorFactory
                               && ui->variants->currentData().toInt() != 0);
}

void StreamingEffects::setVariants(const QStringList& variants)
{
    closeEditor();
    const QSignalBlocker blocker(ui->variants);
    ui->variants->clear();
    for (int type = 0; type < variants.size(); type++)
        ui->variants->addItem(variants.at(type), type);

    confirmedType = 0;
    variantIndex = ui->variants->count() != 0 ? 0 : -1;
    if (variantIndex >= 0)
        ui->variants->setCurrentIndex(variantIndex);
    updateEnabledControl();
    updateEditorButton();
}
void StreamingEffects::setEffectType(int type)
{
    const int index = ui->variants->findData(type);
    if (index < 0)
        return;

    confirmedType = type;
    if (variantIndex != index)
    {
        closeEditor();
        variantIndex = index;
    }
    const QSignalBlocker blocker(ui->variants);
    ui->variants->setCurrentIndex(index);
    updateEnabledControl();
    updateEditorButton();
}

void StreamingEffects::setEffectEnabled(bool enabled)
{
    confirmedEnabled = enabled;
    const QSignalBlocker blocker(ui->enabled);
    ui->enabled->setChecked(enabled);
}
void StreamingEffects::setEffectAvailable(bool value)
{
    available = value;
    ui->variants->setEnabled(available);
    updateEnabledControl();
    updateEditorButton();
}

void StreamingEffects::updateEnabledControl()
{
    const bool canEnable = available && ui->variants->currentData().toInt() != 0;

    if (!canEnable)
    {
        const QSignalBlocker blocker(ui->enabled);
        ui->enabled->setChecked(false);
    }

    ui->editButton->setEnabled(canEnable);
    ui->enabled->setEnabled(canEnable);
}

void StreamingEffects::applyIncomingParameter(uint16_t id, uint8_t channel, int32_t value)
{
    if (edited 
    &&  channel == protocolChannel)
        edited->applyIncomingParameter(id, value);
}

void StreamingEffects::setMeterLevels(const urc_meter_level& level)
{
    if (edited)
        edited->setMeterLevels(level);
}

void StreamingEffects::resetMeter()
{
    if (edited)
        edited->resetMeter();
}
