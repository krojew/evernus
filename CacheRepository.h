#pragma once

#include "Repository.h"

namespace Evernus
{
    template<class T>
    class CacheRepository
        : public Repository<T>
    {
    public:
        using Repository<T>::Repository;
        virtual ~CacheRepository() = default;

        void clearOldData() const;
    };
}

#include "CacheRepository.cpp"
