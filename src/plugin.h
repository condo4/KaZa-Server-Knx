#ifndef KNX_PLUGIN_H
#define KNX_PLUGIN_H

#include <QQmlExtensionPlugin>

class KnxPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

public:
    void registerTypes(const char *uri) override;
};

#endif // KNX_PLUGIN_H
