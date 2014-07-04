#pragma once

#include "EveType.h"

namespace Evernus
{
    class ItemPrice;

    class EveDataProvider
    {
    public:
        EveDataProvider() = default;
        virtual ~EveDataProvider() = default;

        virtual QString getTypeName(EveType::IdType id) const = 0;
        virtual double getTypeVolume(EveType::IdType id) const = 0;
        virtual ItemPrice getTypeSellPrice(EveType::IdType id, quint64 stationId) const = 0;

        virtual QString getLocationName(quint64 id) const = 0;
    };
}
