#include "parameterapplier.h"

void ParameterApplier::applyParameterValue(Control control, int32_t value)
{
    switch (control)
    {
    case Control::Fader:
        setFaderValue(value);
        break;
    case Control::Mute:
        // Mixer protocol: 0 = muted; 1 = unmuted.
        setMute(!value);
        break;
    case Control::Solo:
        setSolo(value);
        break;
    case Control::Pan:
        setPANValue(value);
        break;
    }
}
