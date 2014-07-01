#pragma once

#include "EveType.h"

namespace Evernus
{
    class NameProvider
    {
    public:
        NameProvider() = default;
        virtual ~NameProvider() = default;

        virtual QString getName(EveType::IdType id) const = 0;
    };
}
