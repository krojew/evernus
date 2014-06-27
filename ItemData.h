#pragma once

#include <boost/optional.hpp>

#include "EveType.h"

namespace Evernus
{
    struct ItemData
    {
        typedef boost::optional<quint64> LocationIdType;
        typedef EveType::IdType TypeIdType;
        typedef quint64 IdType;

        TypeIdType mTypeId = 0;
        LocationIdType mLocationId;
        uint mQuantity = 0;
    };
}
