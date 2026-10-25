#include "streamingeffects.h"
#include "ui_streamingeffects.h"

#include <QCheckBox>
#include <QComboBox>
#include <QSignalBlocker>

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

                const int type = ui->variants->itemData(index).toInt();
                updateEnabledControl();
                if (type == 0 && available)
                    emit effectTypeChanged(ui->variants->itemData(index).toInt());

                emit effectTypeChanged(type);
            }
        );

    setEffectAvailable(false);
}

StreamingEffects::~StreamingEffects()
{
    delete ui;
}

void StreamingEffects::setVariants(const QStringList& variants)
{
    const QSignalBlocker blocker(ui->variants);
    ui->variants->clear();
    for (int type = 0; type < variants.size(); type++)
        ui->variants->addItem(variants.at(type), type);

    confirmedType = 0;
    if (ui->variants->count() != 0)
        ui->variants->setCurrentIndex(0);
    updateEnabledControl();
}

void StreamingEffects::setEffectType(int type)
{
    const int index = ui->variants->findData(type);
    if (index < 0)
        return;

    confirmedType = type;
    const QSignalBlocker blocker(ui->variants);
    ui->variants->setCurrentIndex(index);
    updateEnabledControl();
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
}

void StreamingEffects::updateEnabledControl()
{
    const bool canEnable = available && ui->variants->currentData().toInt() != 0;

    if (!canEnable)
    {
        const QSignalBlocker blocker(ui->enabled);
        ui->enabled->setChecked(false);
    }

    ui->enabled->setEnabled(canEnable);
}
