#ifndef MIDIPORT_H
#define MIDIPORT_H

#include <QString>
#include <QVector>

struct MidiPort
{
    QString id;
    QString label;
};

QVector<MidiPort> enumerateMidiPorts();

#endif // MIDIPORT_H
