#ifndef DEVICE_H
#define DEVICE_H

#include <urcontrol.h>

class Device
{
public:
    static urc_device* get() { return instance.device; }
    static int open(const char* alsa_port);
    static void close();
private:
    static Device instance;
    ~Device() { close(); }
    urc_device* device = nullptr;
};

#endif // DEVICE_H
