#include "device.h"

Device Device::instance;

int Device::open(const char* alsa_port)
{
    close(); // Close old connection

    return urc_open(&instance.device, alsa_port);
}

void Device::close()
{
    if (instance.device != nullptr)
    {
        urc_close(instance.device);
        instance.device = nullptr;
    }
}