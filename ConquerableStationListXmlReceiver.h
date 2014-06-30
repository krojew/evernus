#pragma once

#include "ConquerableStationList.h"
#include "APIXmlReceiver.h"

namespace Evernus
{
    template<>
    void APIXmlReceiver<ConquerableStation>::attribute(const QXmlName &name, const QStringRef &value);
}
