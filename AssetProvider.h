#pragma once

#include "Character.h"

namespace Evernus
{
    class AssetList;

    class AssetProvider
    {
    public:
        AssetProvider() = default;
        virtual ~AssetProvider() = default;

        virtual const AssetList &fetchForCharacter(Character::IdType id) const = 0;
    };
}
