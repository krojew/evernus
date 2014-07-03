#include "ConquerableStationListXmlReceiver.h"

namespace Evernus
{
    template<>
    void APIXmlReceiver<ConquerableStation>::attribute(const QXmlName &name, const QStringRef &value)
    {
        const auto localName = name.localName(mNamePool);
        if (localName == "stationID")
            mCurrentElement->setId(convert<uint>(value.toString()));
        else if (localName == "stationName")
            mCurrentElement->setName(value.toString());
    }
}
