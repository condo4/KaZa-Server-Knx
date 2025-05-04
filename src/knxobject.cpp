#include "knxobject.h"

#include <QEventLoop>
#include <QTimer>

#define KNX_READ            (0x00)
#define KNX_RESPONSE        (0x01)
#define KNX_WRITE           (0x02)

static inline QString getUnit(uint16_t type)
{
    uint8_t major = type >> 8 & 0xFF;
    uint8_t minor = type & 0xFF;

    switch(major)
    {
    case 5: // 1byte_unsigned
        {
            switch(minor)
            {
                case 1: return "%"; // percent
                case 3: return "°"; // angle
                case 4: return "%"; // percentU8
            }
        }
    break;

    case 6: // 1byte_signed
    {
        switch(minor)
        {
            case 1: return "%"; // percentV8
        }
    }
    break;

    case 7: // 2byte_unsigned
    {
        switch(minor)
        {
            case 2: return "ms"; // time_period_msec
            case 3: return "ms"; // time_period_10msec
            case 4: return "ms"; // time_period_100msec
            case 5: return "s"; // time_period_sec
            case 6: return "min"; // time_period_min
            case 7: return "h"; // time_period_hrs
            case 11: return "mm"; // time_period_msec
            case 12: return "mA"; // current
            case 13: return "lx"; // brightness
        }
    }
    break;

    case 8: // 2byte_signed
    {
        switch(minor)
        {
            case 2: return "ms"; // delta_time_ms
            case 3: return "ms"; // delta_time_10ms
            case 4: return "ms"; // delta_time_100ms
            case 5: return "s"; // delta_time_sec
            case 6: return "min"; // delta_time_min
            case 7: return "h"; // delta_time_hrs
            case 10: return "%"; // percentV16
            case 11: return "°"; // rotation_angle
            case 12: return "m"; // length_m
        }
    }
    break;

    case 9: // 2byte_float
    {
        switch(minor)
        {
            case 1: return "°C"; // temperaturev
            case 2: return "K";  // temperature_difference_2byte
            case 3: return "K/h"; // temperature_a
            case 4: return "Lux"; // illuminance
            case 5: return "m/s"; // wind_speed_ms
            case 6: return "Pa"; // pressure_2byte
            case 7: return "%"; // humidity
            case 8: return "ppm"; // ppm
            case 9: return "m³/h"; // air_flow
            case 10: return "s"; // time_1
            case 11: return "ms"; // time_2
            case 20: return "mV"; // voltage
            case 21: return "mA"; // current
            case 22: return "W/m²"; // power_density
            case 23: return "K/%"; // kelvin_per_percent
            case 24: return "kW"; // power_2byte
            case 25: return "l/h"; // volume_flow
            case 26: return "l/m²"; // rain_amount
            case 27: return "°F"; // temperature_f
            case 28: return "km/h"; // wind_speed_kmh
            case 29: return "g/m³"; // absolute_humidity
            case 30: return "μg/m³"; // concentration_ugm3
        }
    }
    break;

    case 12: // 4byte_unsigned
    {
        switch(minor)
        {
            case 100: return "s"; // long_time_period_sec
            case 101: return "min"; // long_time_period_min
            case 102: return "h"; // long_time_period_hrs
        }
    }
    break;

    case 13: // 4byte_signed
    {
        switch(minor)
        {
            case 2: return "m³/h"; // flow_rate_m3h
            case 10: return "Wh"; // active_energy
            case 11: return "VAh"; // apparant_energy
            case 12: return "VARh"; // reactive_energy
            case 13: return "kWh"; // active_energy_kwh
            case 14: return "kVAh"; // apparant_energy_kvah
            case 15: return "kVARh"; // reactive_energy_kvarh
            case 16: return "MWh"; // active_energy_mwh
            case 100: return "s"; // long_delta_timesec
        }
    }
    break;

    case 14: // 4byte_float
    {
        switch(minor)
        {
        case 0: return "m/s²"; // acceleration
        case 1: return "rad/s²"; // acceleration_angular
        case 2: return "J/mol"; // activation_energy
        case 3: return "s⁻¹"; // activity
        case 4: return "mol"; // mol
        case 6: return "rad"; // angle_rad
        case 7: return "°"; // angle_deg
        case 8: return "J s"; // angular_momentum
        case 9: return "rad/s"; // angular_velocity
        case 10: return "m²"; // area
        case 11: return "F"; // capacitance
        case 12: return "C/m²"; // charge_density_surface
        case 13: return "C/m³"; // charge_density_volume
        case 14: return "m²/N"; // compressibility
        case 15: return "S"; // conductance
        case 16: return "S/m"; // electrical_conductivity
        case 17: return "kg/m³"; // density
        case 18: return "C"; // electric_charge
        case 19: return "A"; // electric_current
        case 20: return "A/m²"; // electric_current_density
        case 21: return "C m"; // electric_dipole_moment
        case 22: return "C/m²"; // electric_displacement
        case 23: return "V/m"; // electric_field_strength
        case 24: return "c"; // electric_flux
        case 25: return "C/m²"; // electric_flux_density
        case 26: return "C/m²"; // electric_polarization
        case 27: return "V"; // electric_potential
        case 28: return "V"; // electric_potential_difference
        case 29: return "A m²"; //  electromagnetic_moment
        case 30: return "V"; // electromotive_force
        case 31: return "J"; // energy
        case 32: return "N"; // force
        case 33: return "Hz"; // frequency
        case 34: return "rad/s"; // angular_frequency
        case 35: return "J/K"; // heatcapacity
        case 36: return "W"; // heatflowrate
        case 37: return "J"; // heat_quantity
        case 38: return "Ω"; // impedance
        case 39: return "m"; // length
        case 40: return "lm s"; // light_quantity
        case 41: return "cd/m²"; // luminance
        case 42: return "lm"; // luminous_flux
        case 43: return "cd"; // luminous_intensity
        case 44: return "A/m"; // magnetic_field_strength
        case 45: return "Wb"; // magnetic_flux
        case 46: return "T"; // magnetic_flux_density
        case 47: return "A m²"; // magnetic_moment
        case 48: return "T"; // magnetic_polarization
        case 49: return "A/m"; // magnetization
        case 50: return "A"; // magnetomotive_force
        case 51: return "kg"; // mass
        case 52: return "kg/s"; // mass_flux
        case 53: return "N/s"; // momentum
        case 54: return "rad"; // phaseanglerad
        case 55: return "°"; // phaseangledeg
        case 56: return "W"; // power
        case 58: return "Pa"; // pressure
        case 59: return "Ω"; // reactance
        case 60: return "Ω"; // resistance
        case 61: return "Ωm"; // resistivity
        case 62: return "H"; // self_inductance
        case 63: return "sr"; // solid_angle
        case 64: return "W/m²"; // sound_intensity
        case 65: return "m/s"; // speed
        case 66: return "Pa"; // stress
        case 67: return "N/m"; // surface_tension
        case 68: return "°C"; // common_temperature
        case 69: return "K"; // absolute_temperature
        case 70: return "K"; // temperature_difference
        case 71: return "J/K"; // thermal_capacity
        case 72: return "W/mK"; // thermal_conductivity
        case 73: return "V/K"; // thermoelectric_power
        case 74: return "s"; // time_seconds
        case 75: return "Nm"; // torque
        case 76: return "m³"; // volume
        case 77: return "m³/s"; // volume_flux
        case 78: return "N"; // weight
        case 79: return "J"; // work
        case 80: return "VA"; // apparent_power
        }
    }
    break;
    }
    return "";
}

KnxObject::KnxObject(const QString &name, quint16 gad, quint16 dpt, QObject *parent)
    : KaZaObject{name, parent}
    , m_gad(gad)
    , m_dpt(dpt)
{
    setUnit(getUnit(m_dpt));
}

quint16 KnxObject::gad() const {
    return m_gad;
}

quint16 KnxObject::dpt() const {
    return m_dpt;
}

QVariant KnxObject::value() const {
    if(!m_value.isValid())
    {
#ifdef DEBUG_KNX
        qDebug() << "KnxObject ask read for " << name();
#endif
        emit askRead(gad());
    }
    return m_value;
}

void KnxObject::setValue(QVariant newValue) {
    qDebug() << name() << " < " << newValue;
    if(m_value != newValue)
    {
        m_value = newValue;
        emit valueChanged();
    }
}

void KnxObject::changeValue(QVariant newValue, bool confirm) {
    /* Send KNX WRITE FRAME */
    emit askWrite(gad(), dpt(), newValue);
    while(confirm)
    {
        QTimer timeout;
        QEventLoop wait;
        QObject::connect(&timeout, &QTimer::timeout, &wait, &QEventLoop::quit);
        QObject::connect(this, &KnxObject::valueChanged, &wait, &QEventLoop::quit);
        emit askWrite(gad(), dpt(), QVariant());
        timeout.start(500);
        wait.exec();
        if(timeout.remainingTime() == 0)
        {
            emit askWrite(gad(), dpt(), QVariant());
        }
        else
        {
            if(m_value != newValue)
            {
                emit askWrite(gad(), dpt(), newValue);
                emit askWrite(gad(), dpt(), QVariant());
            }
            else {
                break;
            }
        }
    }
}


void KnxObject::reciveFrame(unsigned char *buffer, int len) {
    unsigned char cmd = static_cast<unsigned char>(((buffer[0] & 0x03) << 2) | ((buffer[1] & 0xC0) >> 6));

    if((cmd == KNX_WRITE) | (cmd == KNX_RESPONSE))
    {
        uint8_t mdpt = (m_dpt >> 8);

        if(len < 2)
        {
            qWarning() << "INVALID TELEGRAM " << frameToStr(buffer, len) << "FOR DPT " << dptToStr(m_dpt);
            return;
        }

        switch(mdpt)
        {
            case 1:
            {
                m_value.setValue<bool>(buffer[1] & 0x1);
                emit valueChanged();
                break;
            }
            case 5:
            {
                if(len < 3)
                {
                    qWarning() << "INVALID TELEGRAM " << frameToStr(buffer, len) << "FOR DPT " << dptToStr(m_dpt);
                    return;
                }
                unsigned char d0 = (unsigned char)(buffer[2]);
                unsigned short v = d0; // Short instead of char to avoid toString conversion error

                switch(m_dpt & 0xFF)
                {
                    case 1: // DPT_Scaling [0...100]
                        v = ((d0 * 100) / 255);
                        break;
                    case 2: // DPT_Angle [0...360]
                        v = ((d0 * 360) / 255);
                        break;
                    default:
                        break;
                }

                m_value.setValue(v);
                emit valueChanged();
                break;
            }
            case 7:
            {
                if(len < 4)
                {
                    qWarning() << "INVALID TELEGRAM " << frameToStr(buffer, len) << "FOR DPT " << dptToStr(m_dpt);
                    return;
                }
                unsigned char d0 = (unsigned char)(buffer[2]);
                unsigned char d1 = (unsigned char)(buffer[3]);
                unsigned short v = d0 << 8 | d1;
                m_value.setValue(v);
                emit valueChanged();
                break;
            }
            case 9:
            {
                if(len < 4)
                {
                    qWarning() << "INVALID TELEGRAM " << frameToStr(buffer, len) << "FOR DPT " << dptToStr(m_dpt);
                    return;
                }
                unsigned char d0 = (unsigned char)(buffer[2]);
                unsigned char d1 = (unsigned char)(buffer[3]);
                unsigned char sign = (d0 & 0x80) >>  7;
                unsigned short exp = (d0 & 0x78) >> 3;
                int mant = ((d0 & 0x07) << 8) | d1;
                if(sign != 0)
                    mant = -(~(mant - 1) & 0x07ff);
                m_value.setValue<float>((1 << exp) * 0.01 * ((int)mant));
                emit valueChanged();
                break;
            }
            case 13:
            {
                if(len < 6)
                {
                    qWarning() << "INVALID TELEGRAM " << frameToStr(buffer, len) << "FOR DPT " << dptToStr(m_dpt);
                    return;
                }
                unsigned char d0 = (unsigned char)(buffer[2]);
                unsigned char d1 = (unsigned char)(buffer[3]);
                unsigned char d2 = (unsigned char)(buffer[4]);
                unsigned char d3 = (unsigned char)(buffer[5]);
                signed int v = d0 << 24 | d1 << 16 | d2 << 8 | d3;
                m_value.setValue(v);
                emit valueChanged();
                break;
            }
            case 14:
            {
                if(len < 6)
                {
                    qWarning() << "INVALID TELEGRAM " << frameToStr(buffer, len) << "FOR DPT " << dptToStr(m_dpt);
                    return;
                }
                float v;
                unsigned int rdata = (((unsigned char)(buffer[2])<< 24) |
                                      ((unsigned char)(buffer[3])<< 16) |
                                      ((unsigned char)(buffer[4])<< 8) |
                                      (unsigned char)(buffer[5]));
                memcpy(&v, &rdata, 4);
                m_value.setValue(v);
                emit valueChanged();
                break;
            }
            case 20:
            {
                if(len < 3)
                {
                    qWarning() << "INVALID TELEGRAM " << frameToStr(buffer, len) << "FOR DPT " << dptToStr(m_dpt);
                    return;
                }
                unsigned char d0 = (unsigned char)(buffer[2]);
                m_value.setValue(d0);
                emit valueChanged();
                break;
            }
            default:
            {
                static bool first = true;
                if(first)
                {
                    qWarning() << "Not managed type " << dptToStr(m_dpt);
                    first = false;
                }
            }
        }
#ifdef DEBUG_KNX_FRAME
        qDebug().noquote() << "RECIVE " << ((cmd == KNX_WRITE)?("WRITE"):("RESPONSE")) << " FRAME FOR " << gadToStr(m_gad) << " (" << name() << ") set value to " << m_value;
#endif
    }
    else {
        if(m_localData)
        {
            qDebug() << "TODO: Recieve READ FRAME on Local Data managed object";
        }
    }
}

QVariant KnxObject::rawid() const
{
    return m_gad;
}
