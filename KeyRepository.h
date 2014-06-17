#pragma once

#include "Repository.h"

namespace Evernus
{
    class KeyRepository
        : public Repository
    {
    public:
        using Repository::Repository;
        virtual ~KeyRepository() = default;

        void create() const;

        QString getTableName() const;
    };
}
