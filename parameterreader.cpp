#include "parameterreader.h"

#include "device.h"

ParameterReader::ParameterReader(const QVector<ParameterReadRequest>& request, int timeoutMs, QObject *parent)
: QThread(parent)
, request(request)
, timeoutMs(timeoutMs)
{

}

void ParameterReader::run()
{
    for (int index = 0; index < request.size(); index++)
    {
        if (isInterruptionRequested())
            break;

        const ParameterReadRequest &req = request.at(index);
        int32_t value = 0;
        const int result = urc_get_parameter(Device::get(), req.id, req.channel, timeoutMs, &value);

        if (isInterruptionRequested())
            break;

        emit parameterRead(index, result, value);

        if (result == URC_ERR_CLOSED)
            break;
    }
}
