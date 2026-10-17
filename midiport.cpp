#include <alsa/asoundlib.h>
#include <urcontrol.h>

#include "midiport.h"

QVector<MidiPort> enumerateMidiPorts()
{
    QVector<MidiPort> ports;
    int card = -1;
    if (snd_card_next(&card) < 0)
        return ports;

    while (card >= 0)
    {
        snd_ctl_t* ctl = nullptr;
        const QByteArray ctlName = QString("hw:%1").arg(card).toUtf8();
        if (snd_ctl_open(&ctl, ctlName.constData(), 0) >= 0)
        {
            snd_ctl_card_info_t* cardInfo = nullptr;
            snd_ctl_card_info_alloca(&cardInfo);
            const QString cardLabel = snd_ctl_card_info(ctl, cardInfo) >= 0
                                          ? QString::fromLocal8Bit(snd_ctl_card_info_get_name(cardInfo))
                                          : QString("Card %1").arg(card);

            int device = -1;
            while (snd_ctl_rawmidi_next_device(ctl, &device) >= 0 && device >= 0)
            {
                snd_rawmidi_info_t* info = nullptr;
                snd_rawmidi_info_alloca(&info);
                snd_rawmidi_info_set_device(info, static_cast<unsigned int>(device));
                snd_rawmidi_info_set_subdevice(info, 0);
                snd_rawmidi_info_set_stream(info, SND_RAWMIDI_STREAM_INPUT);
                if (snd_ctl_rawmidi_info(ctl, info) < 0)
                    continue;

                const unsigned int count = snd_rawmidi_info_get_subdevices_count(info);
                for (unsigned int sub = 0; sub < count; ++sub)
                {
                    snd_rawmidi_info_set_subdevice(info, sub);
                    snd_rawmidi_info_set_stream(info, SND_RAWMIDI_STREAM_INPUT);
                    if (snd_ctl_rawmidi_info(ctl, info) < 0)
                        continue;

                    const QString subLabel = QString::fromLocal8Bit(snd_rawmidi_info_get_subdevice_name(info));
                    snd_rawmidi_info_set_stream(info, SND_RAWMIDI_STREAM_OUTPUT);
                    if (snd_ctl_rawmidi_info(ctl, info) < 0)
                        continue; // urc_open needs both input and output

                    const QString id = QString("hw:%1,%2,%3").arg(card).arg(device).arg(sub);
                    ports.push_back({id, QString("%1 — %2 %3").arg(id, cardLabel, subLabel)});
                }
            }
            snd_ctl_close(ctl);
        }
        if (snd_card_next(&card) < 0)
            break;

    }
    return ports;
}
