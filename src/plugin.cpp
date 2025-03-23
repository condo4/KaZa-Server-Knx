#include "plugin.h"

#include "knxbus.h"

#include <qqml.h>

void KnxPlugin::registerTypes(const char *uri)
{
    // @uri xdgutils
    qmlRegisterType<KnxBus>(uri, 1, 0, "KnxBus");
}
