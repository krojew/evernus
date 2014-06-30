#pragma once

#include <vector>

#include <QString>

namespace Evernus
{
    struct ConquerableStation
    {
        uint mId;
        QString mName;
    };

    typedef std::vector<ConquerableStation> ConquerableStationList;
}
