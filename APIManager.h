#pragma once

#include <functional>
#include <vector>

#include "APIResponseCache.h"
#include "APIInterface.h"
#include "Character.h"

namespace Evernus
{
    class Character;

    class APIManager
    {
    public:
        typedef std::vector<Character::IdType> CharacterList;

        APIManager() = default;
        virtual ~APIManager() = default;

        void fetchCharacterList(const Key &key, const std::function<void (const CharacterList &)> &callback);

    private:
        static const QString eveTimeFormat;

        APIResponseCache mCache;
        APIInterface mInterface;

        static QString queryPath(const QString &path, const QString &xml);
    };
}
