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
        : public QObject
    {
        Q_OBJECT

    public:
        template<class T>
        using Callback = std::function<void (const T &, bool)>;

        typedef std::vector<Character::IdType> CharacterList;

        APIManager();
        virtual ~APIManager() = default;

        void fetchCharacterList(const Key &key, const Callback<CharacterList> &callback);

    signals:
        void error(const QString &info);

    private:
        static const QString eveTimeFormat;

        APIResponseCache mCache;
        APIInterface mInterface;

        static QString queryPath(const QString &path, const QString &xml);
    };
}
