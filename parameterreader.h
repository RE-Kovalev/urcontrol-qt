#ifndef PARAMETERREADER_H
#define PARAMETERREADER_H

#include <QThread>
#include <QVector>
#include <cstdint>

#include <urcontrol.h>

// Read requests contain protocol addresses only, never pointers to GUI widgets.
struct ParameterReadRequest
{
    uint16_t id;
    uint8_t channel;
};

class ParameterReader : public QThread
{
    Q_OBJECT

public:
    ParameterReader(const QVector<ParameterReadRequest>& request, int timeoutMs, QObject* parent = nullptr);

signals:
    // Index refers to the immutable binding snapshot used to start the thread.
    void parameterRead(int index, int result, int value);

protected:
    void run() override;

private:
    QVector<ParameterReadRequest> request;
    int timeoutMs;
};

#endif // PARAMETERREADER_H
