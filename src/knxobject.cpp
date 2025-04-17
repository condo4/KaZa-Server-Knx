#include "knxobject.h"

#include <QEventLoop>
#include <QTimer>

#define KNX_READ            (0x00)
#define KNX_RESPONSE        (0x01)
#define KNX_WRITE           (0x02)

static inline QString getUnit(uint16_t type)
{
    if((type >> 8 & 0xFF) == 5)
    {
        switch(type & 0xFF)
        {
        case 1: return "%";
        case 3: return "%";
        case 4: return "%";
        }
    }
    if((type >> 8 & 0xFF) == 9)
    {
        switch(type & 0xFF)
        {
        case 1: return "°C";
        case 2: return "K";
        case 3: return "K/h";
        case 4: return "Lux";
        case 5: return "m/s";
        case 6: return "Pa";
        case 7: return "%";
        case 8: return "ppm";
        case 10: return "s";
        case 11: return "ms";
        case 20: return "mV";
        case 21: return "mA";
        case 22: return "W/m²";
        case 23: return "K/%";
        case 24: return "kW";
        case 25: return "l/h";
        case 26: return "l/m²";
        case 27: return "°F";
        case 28: return "km/h";
        }
    }
    if((type >> 8 & 0xFF) == 13)
    {
        switch(type & 0xFF)
        {
        case 10: return "Wh";
        case 11: return "VAh";
        case 12: return "VARh";
        case 13: return "kWh";
        case 14: return "kVAh";
        case 15: return "kVARh";
        case 100: return "s";
        }
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
