#pragma once

#include "Repository.h"

namespace Evernus
{
    class CharacterRepository
        : public Repository
    {
    public:
        using Repository::Repository;
        virtual ~CharacterRepository() = default;

        void create() const;
    };
}
