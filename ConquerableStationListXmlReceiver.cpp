#include "ConquerableStationListXmlReceiver.h"

namespace Evernus
{
    template<>
    void APIXmlReceiver<ConquerableStation>::attribute(const QXmlName &name, const QStringRef &value)
    {
        const auto localName = name.localName(mNamePool);
        if (localName == "stationID")
            mCurrentElement->mId = convert<uint>(value.toString());
        else if (localName == "stationName")
            mCurrentElement->mName = value.toString();
    }
}
