#ifndef PARAMETERAPPLIER_H
#define PARAMETERAPPLIER_H

#include <cstdint>

class ParameterApplier
{
public:
    enum class Control
    {
        Fader,
        Mute,
        Solo,
        Pan
    };

    virtual void setFaderValue(int32_t value) = 0;
    virtual void setMute(bool muted) = 0;
    virtual void setSolo(bool solo) = 0;
    virtual void setPANValue(int32_t value) = 0;
    void applyParameterValue(Control control, int32_t value);
};

#endif // PARAMETERAPPLIER_H
