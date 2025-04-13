#ifndef KNXOBJECT_H
#define KNXOBJECT_H

#include <kazaobject.h>
#include <QVariant>

class KnxObject : public KaZaObject
{
    Q_OBJECT

public:
    explicit KnxObject(const QString &name, quint16 gad, quint16 dpt, QObject *parent = nullptr);

    quint16 gad() const;
    quint16 dpt() const;

    QVariant value() const override;
    void setValue(QVariant newValue) override;
    void changeValue(QVariant, bool confirm = false) override;

    void reciveFrame(unsigned char *buffer, int len);

    QVariant rawid() const override;

private:
    quint16 m_gad;
    quint16 m_dpt;
    QVariant m_value;
    bool m_localData {false};


signals:
    void askRead(quint16 gad) const;
    void askWrite(quint16 gad, quint16 dpt, QVariant value) const;

};

static inline QString addrToStr(uint16_t addr)
{
    return QString("%1.%2.%3").arg((addr >> 11) & 0x1f).arg((addr >> 8) & 0x07).arg((addr) & 0xff);
}

static inline QString gadToStr(uint16_t addr)
{
    return QString("%1/%2/%3").arg((addr >> 11) & 0x1f).arg((addr >> 8) & 0x07).arg((addr) & 0xff);
}

static inline QString dptToStr(uint16_t d)
{
    return QString("%1.%2").arg((d >> 8) & 0xff).arg(d & 0xff);
}

static inline QString frameToStr(unsigned char *buffer, int len)
{
    QString ret = "[";
    for(int i = 0; i <= len; i++)
    {
        ret += QStringLiteral("%1").arg(buffer[i], 2, 16, QLatin1Char('0')) + ":";
    }
    ret.chop(1);
    ret += "]";

    return ret;
}


#endif // KNXOBJECT_H
