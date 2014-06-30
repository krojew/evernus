#include "CachedConquerableStation.h"

namespace Evernus
{
    CachedConquerableStationList::IdType CachedConquerableStation::getListId() const noexcept
    {
        return mListId;
    }

    void CachedConquerableStation::setListId(CachedConquerableStationList::IdType id) noexcept
    {
        mListId = id;
    }

    QString CachedConquerableStation::getName() const &
    {
        return mName;
    }

    QString &&CachedConquerableStation::getName() && noexcept
    {
        return std::move(mName);
    }

    void CachedConquerableStation::setName(const QString &name)
    {
        mName = name;
    }

    void CachedConquerableStation::setName(QString &&name)
    {
        mName = std::move(name);
    }
}
