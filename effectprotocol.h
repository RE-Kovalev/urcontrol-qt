#ifndef EFFECTPROTOCOL_H
#define EFFECTPROTOCOL_H

#include <QString>
#include <cmath>
#include <cstddef>
#include <cstdint>

// Streaming InputFX3 Gate/Comp parameters are NOT the same as chstrip/Comp*.
// Addresses and endpoint values below were measured on UR22C. Channel is
// selected by the editor (Input 1 = 0, Input 2 = 1 in urcontrol-c).
namespace EffectProtocol
{
    enum class Display
    {
        Raw,
        Threshold,
        GateRange,
        Gain,
        Attack,
        Hold,
        Decay,
        Ratio
    };

    struct Parameter
    {
        int id;
        int minimum;
        int maximum;
        Display display = Display::Raw;

        constexpr bool isKnown() const noexcept
        {
            return id >= 0;
        }

        constexpr bool hasRange() const noexcept
        {
            return isKnown() && minimum < maximum;
        }
    };

    inline constexpr Parameter gateThreshold{300, 1, 73, Display::Threshold};
    inline constexpr Parameter gateRange{301, 0, 73, Display::GateRange};
    inline constexpr Parameter gateAttack{302, 57, 283, Display::Attack};
    inline constexpr Parameter gateHold{303, 0, 213, Display::Hold};
    inline constexpr Parameter gateDecay{304, 24, 300, Display::Decay};

    inline constexpr Parameter compThreshold{308, 19, 73, Display::Threshold};
    inline constexpr Parameter compRatio{309, 0, 120, Display::Ratio};
    inline constexpr Parameter compGain{313, 0, 180, Display::Gain};
    inline constexpr Parameter compGainAuto{314, 0, 1};
    inline constexpr Parameter compAttack{310, 57, 283, Display::Attack};
    inline constexpr Parameter compRelease{311, 24, 300, Display::Decay};
    inline constexpr Parameter compKnee{312, 0, 2};

    struct TimePoint
    {
        int code;
        double milliseconds;
        int precision;
    };

    template<std::size_t N>
    inline QString formatMilliseconds(int32_t value, const TimePoint (&points)[N], int precision)
    {
        for (const TimePoint& point : points)
            if (value == point.code)
                return QStringLiteral("%1 ms").arg(QString::number(point.milliseconds, 'f', point.precision));

        // Only measured points are exact. Other positions are interpolated on a
        // logarithmic scale and explicitly marked approximate until verified.
        for (std::size_t index = 1; index < N; ++index)
        {
            const TimePoint& left = points[index - 1];
            const TimePoint& right = points[index];
            if (value > left.code 
            &&  value < right.code)
            {
                const double t = static_cast<double>(value - left.code) / static_cast<double>(right.code - left.code);
                const double ms = std::exp(std::log(left.milliseconds) + t * (std::log(right.milliseconds) - std::log(left.milliseconds)));
                return QStringLiteral("≈%1 ms").arg(QString::number(ms, 'f', precision));
            }
        }
        return QStringLiteral("raw %1").arg(value);
    }

    inline QString formatValue(const Parameter& parameter, int32_t value)
    {
        constexpr TimePoint attackPoints[] = { {57, 0.092, 3}, {168, 2.552, 3}, {226, 14.50, 2}, {283, 80.00, 2}};
        constexpr TimePoint holdPoints[] = {{0, 0.02, 2}, {77, 5.23, 2}, {164, 233.0, 1}, {213, 1960.0, 1}};
        constexpr TimePoint decayPoints[] = {{24, 9.3, 1}, {106, 37.5, 1}, {214, 233.2, 1}, {300, 999.0, 1}};
        const int precision = value <= 60 ? 2 : (value <= 86 ? 1 : 0);
        int ratioHundredths;
        switch (parameter.display)
        {
        case Display::Threshold:
            // Gate 1 = -72 dB, Compressor 19 = -54 dB, 73 = 0 dB.
            return QStringLiteral("%1 dB").arg(value - 73);

        case Display::GateRange:
            return value == 0 ? QStringLiteral("-inf dB") : QStringLiteral("%1 dB").arg(value - 73);

        case Display::Gain:
            return QStringLiteral("%1 dB").arg(QString::number(value / 10.0, 'f', 1));

        case Display::Attack:
            return formatMilliseconds(value, attackPoints, value < 170 ? 3 : 2);
        
        case Display::Hold:
            return formatMilliseconds(value, holdPoints, value < 100 ? 2 : 1);

        case Display::Decay:
            return formatMilliseconds(value, decayPoints, 1);

        case Display::Ratio:
            if (value == 120)
                return QStringLiteral("INF:1");
            if (value == 119)
                return QStringLiteral("500:1");
            if (value == 118)
                return QStringLiteral("300:1");
            if (value < 0 || value > 116)
                return QStringLiteral("raw %1").arg(value);

            // Work in hundredths to avoid rounding at each step.
            if (value <= 60)       
                ratioHundredths = 100 + value * 5;
            else 
            if (value <= 70) 
                ratioHundredths = 400 + (value - 60) * 10;
            else 
            if (value <= 80) 
                ratioHundredths = 500 + (value - 70) * 20;
            else 
            if (value <= 86) 
                ratioHundredths = 700 + (value - 80) * 50;
            else 
            if (value <= 96) 
                ratioHundredths = 1000 + (value - 86) * 100;
            else 
            if (value <= 106) 
                ratioHundredths = 2000 + (value - 96) * 200;
            else 
            if (value <= 112) 
                ratioHundredths = 4000 + (value - 106) * 500;
            else 
            if (value <= 115) 
                ratioHundredths = 7000 + (value - 112) * 1000;
            else                   
                ratioHundredths = 20000; 

            return QStringLiteral("%1:1").arg(QString::number(ratioHundredths / 100.0, 'f', precision));

        case Display::Raw:
            return QString::number(value);
        }
        return QString::number(value);
    }
}

#endif // EFFECTPROTOCOL_H
