#pragma once

#include "CachedConquerableStationList.h"
#include "Entity.h"

namespace Evernus
{
    class CachedConquerableStation
        : public Entity<uint>
    {
    public:
        using Entity::Entity;
        virtual ~CachedConquerableStation() = default;

        CachedConquerableStationList::IdType getListId() const noexcept;
        void setListId(CachedConquerableStationList::IdType id) noexcept;

        QString getName() const &;
        QString &&getName() && noexcept;
        void setName(const QString &name);
        void setName(QString &&name);

    private:
        CachedConquerableStationList::IdType mListId = CachedConquerableStationList::invalidId;
        QString mName;
    };
}
