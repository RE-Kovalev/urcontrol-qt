#ifndef STREAMINGEFFECTS_H
#define STREAMINGEFFECTS_H
#include <QStringList>
#include <QWidget>
#include <QDialog>
#include <QPointer>
#include <QHash>
#include "effecteditor.h"
#include <functional>
#include <cstdint>
#include <urcontrol.h>


namespace Ui {
class StreamingEffects;
}

class StreamingEffects : public QWidget
{
    Q_OBJECT

public:
    using EditorFactory = std::function<EffectEditor*(int type, uint8_t channel, const QString& channelName, QWidget* parent)>;

    explicit StreamingEffects(QWidget *parent = nullptr);
    ~StreamingEffects() override;

    void setVariants(const QStringList& variants);
    void setEffectType(int type);
    void setEffectEnabled(bool enabled);
    void setEffectAvailable(bool available);
    int confirmedEffectType() const { return confirmedType; }
    bool confirmedEffectEnabled() const { return confirmedEnabled; }

    void setChannel(uint8_t channel, const QString& channelName);
    void setEditorFactory(EditorFactory factory);
    void setEditRequiresEnabled(bool required) { editRequiresEnabled = required; updateEditorButton(); }
    void setMeterSlot(int type, int slot) { meterSlots.insert(type, slot); }
    int effectMeterSlot() const { return meterSlots.value(confirmedType, -1); }
    void applyIncomingParameter(uint16_t id, uint8_t channel, int32_t value);
    void setMeterLevels(const urc_meter_level& level);
    void setStereoMeterLevels(const urc_meter_level& left, const urc_meter_level& right);
    void resetMeter();
    void closeEditor();

signals:
    void effectTypeChanged(int type);
    void effectEnabledChanged(bool enabled);

private:
    void updateEnabledControl();
    void updateEditorButton();

    Ui::StreamingEffects *ui;
    int confirmedType = 0;
    bool confirmedEnabled = false;
    bool available = false;
    int variantIndex = -1;
    uint8_t protocolChannel = 0;
    QString channelTitle;
    QHash<int, int> meterSlots;
    EditorFactory editorFactory;
    bool editRequiresEnabled = false;
    QPointer<EffectEditor> edited;
};
#endif // STREAMINGEFFECTS_H
