#ifndef DEVICECLOSER_H
#define DEVICECLOSER_H

#include <urcontrol.h>

struct DeviceCloser
{
    void operator()(urc_device* device) const
    {
        if (device != nullptr)
        {
            urc_close(device);
        }
    }
};

#endif // DEVICECLOSER_H
