#pragma once

#include <QString>

#include "Entity.h"

namespace Evernus
{
    class Character
        : public Entity<quint64>
    {
    public:
        using Entity::Entity;
        virtual ~Character() = default;
    };
}
