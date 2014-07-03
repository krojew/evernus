#pragma once

#include "EveType.h"

namespace Evernus
{
    class NameProvider
    {
    public:
        NameProvider() = default;
        virtual ~NameProvider() = default;

        virtual QString getTypeName(EveType::IdType id) const = 0;
        virtual QString getLocationName(quint64 id) const = 0;
    };
}
