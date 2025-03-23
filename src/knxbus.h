#ifndef KNXBUS_H
#define KNXBUS_H

#include <QObject>
#include <QQmlEngine>
#include <QTimer>
#include <cstdbool>
#include <cstring>


struct _EIBConnection;
class QSocketNotifier;

class KnxObject;

#define KNX_READ            (0x00)
#define KNX_RESPONSE        (0x01)
#define KNX_WRITE           (0x02)

class KnxBus : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString knxd READ knxd WRITE setKnxd NOTIFY knxdChanged FINAL)
    Q_PROPERTY(QString knxProj READ knxProj WRITE setKnxProj NOTIFY knxProjChanged FINAL)

public:
    explicit KnxBus(QObject *parent = nullptr);

    QString knxd() const;
    void setKnxd(const QString &newKnxd);

    QString knxProj() const;
    void setKnxProj(const QString &newKnxProj);


signals:
    void knxdChanged();
    void knxProjChanged();

private:
    QString m_knxdUrl;
    _EIBConnection *m_knxd {nullptr};
    int m_knxdSocket {0};
    QSocketNotifier *m_knxdClient {nullptr};
    QMap<quint16, KnxObject*> m_objects;
    QMap<quint16, quint16> m_notInitialized;
    QString m_knxProj;
    QList<uint16_t> m_notmanaged;
    QTimer m_initializer;

    void _parseKnxProj();
    quint16 _datapointTypeToDpt(const QString &str) const;

private slots:
    void _tryConnect();
    void _onKnxdReadyRead();
    void _askRead(quint16 gad);
    void _askWrite(quint16 gad, quint16 dpt, QVariant value);
    void _initialize();
};



#endif // KNXBUS_H
