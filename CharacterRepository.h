#pragma once

#include "Repository.h"

namespace Evernus
{
    class KeyRepository;

    class CharacterRepository
        : public Repository
    {
    public:
        using Repository::Repository;
        virtual ~CharacterRepository() = default;

        void create(const KeyRepository &keyRepository) const;
    };
}
