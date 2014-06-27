#include "EveType.h"

namespace Evernus
{
    uint EveType::getGroupId() const noexcept
    {
        return mGroupId;
    }

    void EveType::setGroupId(uint id)
    {
        mGroupId = id;
    }

    QString EveType::getName() const &
    {
        return mName;
    }

    QString &&EveType::getName() && noexcept
    {
        return std::move(mName);
    }

    void EveType::setName(const QString &name)
    {
        mName = name;
    }

    void EveType::setName(QString &&name)
    {
        mName = std::move(name);
    }

    EveType::DescriptionType EveType::getDescription() const
    {
        return mDescription;
    }

    void EveType::setDescription(const DescriptionType &desc)
    {
        mDescription = desc;
    }

    double EveType::getMass() const noexcept
    {
        return mMass;
    }

    void EveType::setMass(double value) noexcept
    {
        mMass = value;
    }

    double EveType::getVolume() const noexcept
    {
        return mVolume;
    }

    void EveType::setVolume(double value) noexcept
    {
        mVolume = value;
    }

    double EveType::getCapacity() const noexcept
    {
        return mCapacity;
    }

    void EveType::setCapacity(double value) noexcept
    {
        mCapacity = value;
    }

    int EveType::getPortionSize() const noexcept
    {
        return mPortionSize;
    }

    void EveType::setPortionSize(int value) noexcept
    {
        mPortionSize = value;
    }

    EveType::RaceIdType EveType::getRaceId() const
    {
        return mRaceId;
    }

    void EveType::setRaceId(const RaceIdType &id)
    {
        mRaceId = id;
    }

    double EveType::getBasePrice() const noexcept
    {
        return mBasePrice;
    }

    void EveType::setBasePrice(double value) noexcept
    {
        mBasePrice = value;
    }

    bool EveType::isPublished() const noexcept
    {
        return mPublished;
    }

    void EveType::setPublished(bool flag) noexcept
    {
        mPublished = flag;
    }

    EveType::MarketGroupIdType EveType::getMarketGroupId() const
    {
        return mMarketGroup;
    }

    void EveType::setMarketGroupId(const MarketGroupIdType &id)
    {
        mMarketGroup = id;
    }

    double EveType::getChanceOfDuplicating() const noexcept
    {
        return mChanceOfDuplicating;
    }

    void EveType::setChanceOfDuplicating(double value) noexcept
    {
        mChanceOfDuplicating = value;
    }
}
