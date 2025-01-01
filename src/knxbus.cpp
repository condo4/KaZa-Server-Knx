#include "knxbus.h"
#include <eibclient.h>
#include <QSocketNotifier>
#include <QDomDocument>
#include <QDomElement>
#include <minizip/unzip.h>
#include "knxobject.h"

inline void decode_dpt1(const unsigned char *data, bool *value) {
    *value = data[0] & 0x1;
}

inline void encode_dpt1(unsigned char *data, bool value) {
    data[0] &= 0xFE;
    if(value) data[0] |= 0x1;
}

inline void decode_dpt2(const unsigned char *data, bool *control, bool *value) {
    *value = data[0] & 0x1;
    *control = data[0] & 0x2;
}

inline void encode_dpt2(unsigned char *data, bool control, bool value) {
    data[0] &= 0xFC;
    if(value) data[0] |= 0x1;
    if(control) data[0] |= 0x2;
}

inline void decode_dpt3(const unsigned char *data, bool *control, unsigned char *stepcode) {
    *stepcode = data[0] & 0x7;
    *control = data[0] & 0x8;
}

inline void encode_dpt3(unsigned char *data, bool control, unsigned char stepcode) {
    data[0] &= 0xF0;
    if(control) data[0] |= 0x8;
    data[0] |= (0x7 & stepcode);
}

inline void decode_dpt4(const unsigned char *data, char *value) {
    *value = data[1];
}

inline void encode_dpt4(unsigned char *data, char value) {
    data[1] = value;
}

inline void decode_dpt5(const unsigned char *data, unsigned char *value) {
    *value = data[1];
}

inline void encode_dpt5(unsigned char *data, unsigned char value) {
    data[1] = value;
}

inline void decode_dpt6(const unsigned char *data, signed char *value) {
    *value = data[1];
}

inline void encode_dpt6(unsigned char *data, signed char value) {
    data[1] = value;
}

inline void decode_dpt7(const unsigned char *data, unsigned short *value) {
    *value = data[1] << 8 | data[2];
}

inline void encode_dpt7(unsigned char *data, unsigned short value) {
    data[1] = (value >> 8) & 0xFF;
    data[2] = value & 0xFF;
}

inline void decode_dpt8(const unsigned char *data, signed short *value) {
    *value = data[1] << 8 | data[2];
}

inline void encode_dpt8(unsigned char *data, signed short value) {
    data[1] = (value >> 8) & 0xFF;
    data[2] = value & 0xFF;
}

inline void decode_dpt9(const unsigned char *data, float *value) {
    unsigned char sign = (data[1] & 0x80) >>  7;
    unsigned short exp = (data[1] & 0x78) >> 3;
    int mant = ((data[1] & 0x07) << 8) | data[2];
    if(sign != 0)
        mant = -(~(mant - 1) & 0x07ff);

    *value  = (1 << exp) * 0.01 * ((int)mant);
}

inline void encode_dpt9(unsigned char *data, float value) {
    unsigned int sign = (value < 0);
    unsigned int exp  = 0;
    int mant = value * 100.0;

    while(mant > 2047 || mant <= -2048 )
    {
        mant = mant >> 1;
        ++exp;
    }
    mant &= 0x07ff;

    data[1] = (sign << 7) | (exp << 3) | ((mant & 0x07ff) >> 8);
    data[2] = mant & 0xFF;
}

inline void decode_dpt10(const unsigned char *data, unsigned char *day, unsigned char *hour, unsigned char *minute, unsigned char *second) {
    *day  = ((data[1]) >>5) & 0x7;
    *hour = (data[1] & 0x1F);
    *minute = (data[2] & 0x3F);
    *second = (data[3] & 0x3F);
}

inline void encode_dpt10(unsigned char *data, unsigned char day, unsigned char hour, unsigned char minute, unsigned char second) {
    data[1] = ((day & 0x7) << 5) | (hour & 0x1F);
    data[2] = minute & 0x3F;
    data[3] = second & 0x3F;
}


inline void decode_dpt11(const unsigned char *data, unsigned char *day, unsigned char *month, unsigned char *year) {
    *day = (data[1] & 0x3F);
    *month = (data[2] & 0x3F);
    *year = (data[3] & 0x7F);
}

inline void encode_dpt11(unsigned char *data, unsigned char day, unsigned char month, unsigned char year) {
    data[1] = day & 0x3F;
    data[2] = month & 0x3F;
    data[3] = year & 0x7F;
}


inline void decode_dpt12(const unsigned char *data, unsigned int *value) {
    *value = data[1] << 24 | data[2] << 16 | data[3] << 8 | data[4];
}

inline void encode_dpt12(unsigned char *data, unsigned int value) {
    data[1] = (value >> 24) & 0xFF;
    data[2] = (value >> 16) & 0xFF;
    data[3] = (value >>  8) & 0xFF;
    data[4] = (value >>  0) & 0xFF;
}

inline void decode_dpt13(const unsigned char *data, signed int *value) {
    *value = data[1] << 24 | data[2] << 16 | data[3] << 8 | data[4];
}

inline void encode_dpt13(unsigned char *data, signed int value) {
    data[1] = (value >> 24) & 0xFF;
    data[2] = (value >> 16) & 0xFF;
    data[3] = (value >>  8) & 0xFF;
    data[4] = (value >>  0) & 0xFF;
}

inline void decode_dpt14(const unsigned char *data, float *value) {
    unsigned int rdata = ((data[1]<< 24) | (data[2]<< 16) | (data[3]<< 8) | data[4]);
    memcpy(value, &rdata, 4);
}

inline void encode_dpt14(unsigned char *data, float value) {
    unsigned int rdata;
    memcpy(&rdata, &value, 4);

    data[1] = (rdata >> 24) & 0xFF;
    data[2] = (rdata >> 16) & 0xFF;
    data[3] = (rdata >>  8) & 0xFF;
    data[4] = (rdata >>  0) & 0xFF;
}

inline void decode_dpt225(const unsigned char *data, unsigned short *time_period, unsigned char *percent) {
    *percent = data[2];
    *time_period = data[1] << 8 | data[2];
}

inline void encode_dpt225(unsigned char *data, unsigned short time_period, unsigned char percent) {
    data[1] = (time_period & 0xFF00) >> 8;
    data[2] = time_period & 0xFF;
    data[3] = percent;
}

KnxBus::KnxBus(QObject *parent)
    : QObject{parent}
{
    qDebug() << "KNX integration loaded";
    QObject::connect(this, &KnxBus::knxdChanged, this, &KnxBus::_tryConnect, Qt::QueuedConnection);
}


QString KnxBus::knxd() const {
    return m_knxdUrl;
}

void KnxBus::setKnxd(const QString &newKnxd) {
    if(m_knxdUrl != newKnxd)
    {
        m_knxdUrl = newKnxd;
        emit knxdChanged();
    }
}


QString KnxBus::knxProj() const {
    return m_knxProj;
}

void KnxBus::setKnxProj(const QString &newKnxProj) {
    if(m_knxProj != newKnxProj)
    {
        m_knxProj = newKnxProj;
        emit knxProjChanged();
        _parseKnxProj();
    }
}

void KnxBus::_parseKnxProj() {
#ifdef DEBUG
    qDebug() << "KNX Load" << m_knxProj;
#endif
    QDomDocument doc;
    unzFile zip = unzOpen64(m_knxProj.toStdString().c_str());
    if (zip == NULL) {
        qWarning() << "Can't open " << m_knxProj;
        return;
    }

    if (unzGoToFirstFile (zip) != UNZ_OK) {
        qWarning() << "Can't open " << m_knxProj;
        return;
    }

    do {
        char *fn;
        char *buffer;
        unz_file_info info;
        unzGetCurrentFileInfo (zip, &info, NULL, 0, NULL, 0, NULL, 0);

        if (info.external_fa & (1 << 4))
            continue;

        if (!(fn = (char *) malloc (info.size_filename + 1)))
        {
            qWarning() << "Can't alocate";
            return;
        }

        unzGetCurrentFileInfo (zip, &info, fn, info.size_filename + 1, NULL, 0, NULL, 0);
        QString filename(fn);
        free(fn);

        if(!filename.endsWith("/0.xml"))
            continue;

        if (unzOpenCurrentFile (zip) != UNZ_OK) {
            qWarning() << "Can't open" << filename;
            return;
        }

        buffer = (char *) malloc(info.uncompressed_size + 1);
        if(!buffer)
        {
            qWarning() << "Can't alocate";
            return;
        }

        unzReadCurrentFile(zip, buffer, info.uncompressed_size + 1);

        QByteArray data(buffer, info.uncompressed_size + 1);
        doc.setContent(data);
        free(buffer);
        unzCloseCurrentFile (zip);
        break;
    } while (unzGoToNextFile (zip) == UNZ_OK);

    // Extract the root markup
    QDomElement knx = doc.documentElement();
    QDomElement project = knx.firstChild().toElement();
    while(project.isElement() && project.tagName() != "Project")
    {
        project = project.nextSibling().toElement();
    }

    QDomElement installations = project.firstChild().toElement();
    while(installations.isElement() && installations.tagName() != "Installations")
    {
        installations = installations.nextSibling().toElement();
    }

    QDomElement installation = installations.firstChild().toElement();
    while(installation.isElement() && installation.tagName() != "Installation")
    {
        installation = installation.nextSibling().toElement();
    }

    QDomElement groupAddresses = installation.firstChild().toElement();
    while(groupAddresses.isElement() && groupAddresses.tagName() != "GroupAddresses")
    {
        groupAddresses = groupAddresses.nextSibling().toElement();
    }

    QDomElement groupRanges = groupAddresses.firstChild().toElement();
    while(groupRanges.isElement() && groupRanges.tagName() != "GroupRanges")
    {
        groupRanges = groupRanges.nextSibling().toElement();
    }

    /* Now loop on each GroupAddress */
    QDomElement groupRange = groupRanges.firstChild().toElement();
    while(groupRange.isElement())
    {
        if(groupRange.tagName() == "GroupRange")
        {
            QString level1 = groupRange.attribute("Name");
            QDomElement groupRange2 = groupRange.firstChild().toElement();
            while(groupRange2.isElement())
            {
                if(groupRange2.tagName() == "GroupRange")
                {
                    QString level2 = groupRange2.attribute("Name");
                    QDomElement groupAddress = groupRange2.firstChild().toElement();
                    while(groupAddress.isElement())
                    {
                        if(groupAddress.tagName() == "GroupAddress")
                        {
                            QString id = level1.trimmed() + "." + level2.trimmed()  + "." + groupAddress.attribute("Name").trimmed();
                            quint16 gad = groupAddress.attribute("Address").toInt();
                            QString dptstr = groupAddress.attribute("DatapointType");
                            if(dptstr.isEmpty())
                            {
                                qWarning() << "WARNING: DPT not set for " << id;
                            }
                            else
                            {
                                quint16 dpt = _datapointTypeToDpt(dptstr);
                                KnxObject *obj = new KnxObject(id, gad, dpt, this);
                                m_objects[gad] = obj;
                                QObject::connect(obj, &KnxObject::askRead, this, &KnxBus::_askRead);
                                QObject::connect(obj, &KnxObject::askWrite, this, &KnxBus::_askWrite);
                            }
                        }
                        groupAddress = groupAddress.nextSibling().toElement();
                    }
                }
                groupRange2 = groupRange2.nextSibling().toElement();
            }
        }
        groupRange = groupRange.nextSibling().toElement();
    }
}

quint16 KnxBus::_datapointTypeToDpt(const QString &str) const
{
    QStringList ar(str.split("-"));
    if(ar.size() == 2)
    {
        return ar[1].toInt() << 8;
    }
    if(ar.size() == 3)
    {
        return ar[1].toInt() << 8 | ar[2].toInt();
    }

    qWarning() << "Unknown DPT " << str;
    return 0;
}

void KnxBus::_tryConnect() {
#ifdef DEBUG
    qInfo() << "Connect to KNXD " << m_knxdUrl.toStdString().c_str();
#endif
    m_knxd = EIBSocketURL(m_knxdUrl.toStdString().c_str());
    if (EIBOpen_GroupSocket (m_knxd, 0) == -1)
    {
        qWarning() << "Error opening knxd socket (" << m_knxdUrl << ")";
        exit(1);
    }
    m_knxdSocket = EIB_Poll_FD(m_knxd);
    m_knxdClient = new QSocketNotifier(m_knxdSocket, QSocketNotifier::Read);
    QObject::connect(m_knxdClient, &QSocketNotifier::activated, this, &KnxBus::_onKnxdReadyRead, Qt::QueuedConnection);
}

void KnxBus::_onKnxdReadyRead()
{
    unsigned char buffer[1025];  //data buffer of 1K
    eibaddr_t dest;
    eibaddr_t src;
    int len = EIBGetGroup_Src(m_knxd, sizeof(buffer), buffer, &src, &dest);
    if(len < 0)
    {
        qWarning() << "Read EIBGetGroup_Src failed (" << len << ") try reconnection";
        EIBClose_sync(m_knxd);
        m_knxd = EIBSocketURL(m_knxdUrl.toStdString().c_str());
        if (EIBOpen_GroupSocket (m_knxd, 0) == -1)
        {
            qWarning() << "Error opening knxd socket (" << m_knxdUrl << ")";
            exit(1);
        }
        m_knxdSocket = EIB_Poll_FD(m_knxd);
        if(m_knxdClient) m_knxdClient->deleteLater();
        m_knxdClient = new QSocketNotifier(m_knxdSocket, QSocketNotifier::Read);
        QObject::connect(m_knxdClient, &QSocketNotifier::activated, this, &KnxBus::_onKnxdReadyRead);
        return;
    }
    if(len < 2)
    {
        qWarning() << "Read EIBGetGroup_Src Invalid packet";
        return;
    }
    if(m_objects.contains(dest))
    {
        m_objects[dest]->reciveFrame(buffer, len);
    }
    else
    {
        if(!m_notmanaged.contains(dest))
        {
            qDebug() << "TODO: Unknown KNX Object " << gadToStr(dest);
            m_notmanaged.append(dest);
        }
    }
}


void KnxBus::_askRead(quint16 gad) {
    QByteArray frame;
    frame.append(KNX_READ >> 2);
    frame.append((KNX_READ & 0x3) << 6);
    if(EIBSendGroup(m_knxd, gad, frame.size(), (const uint8_t *)frame.data()) == -1)
    {
        qWarning() << "EIBSendGroup error";
    }
}


void KnxBus::_askWrite(quint16 gad, quint16 dpt, QVariant value) {
    if(!value.isValid())
    {
        _askRead(gad);
        return;
    }
    QByteArray frame;
    frame.push_back(static_cast<char>(KNX_WRITE >> 2));
    frame.push_back(static_cast<char>((KNX_WRITE & 0x3) << 6));

    switch((dpt >> 8) & 0xFF)
    {
    case 1:
    {
        bool val = (value.toInt() == 1);
        encode_dpt1(reinterpret_cast<unsigned char*>(frame.data() + 1), val);
        break;
    }
    case 5:
    {
        frame.push_back(static_cast<char>(0));
        unsigned char val = 0;
        int raw = value.toUInt();
        switch(dpt & 0xFF)
        {
        case 1:
            val = static_cast<unsigned char>((raw * 255) / 100);
            break;
        case 3:
            val = static_cast<unsigned char>((raw * 255) / 360);
            break;
        default:
            val = static_cast<unsigned char>(raw);
            break;
        }
        encode_dpt5(reinterpret_cast<unsigned char*>(frame.data() + 1), val);
        break;
    }
    case 9:
    {
        frame.push_back(static_cast<char>(0));
        frame.push_back(static_cast<char>(0));
        float val = value.toFloat();
        encode_dpt9(reinterpret_cast<unsigned char*>(frame.data() + 1), val);
        break;
    }
    default:
    {
        qDebug() << "TODO: NEED TO WRITE OBJECT " << gadToStr(gad) << " (" << dptToStr(dpt) << ")  -> " << value;
    }
    }

    if(EIBSendGroup(m_knxd, gad, frame.size(), (const uint8_t *)frame.data()) == -1)
    {
        qWarning() << "EIBSendGroup error";
    }
}
