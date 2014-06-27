#pragma once

#include <boost/optional.hpp>

#include <QString>

#include "MarketGroup.h"
#include "Entity.h"

namespace Evernus
{
    class EveType
        : public Entity<uint>
    {
    public:
        typedef boost::optional<QString> DescriptionType;
        typedef boost::optional<uint> RaceIdType;
        typedef boost::optional<MarketGroup::IdType> MarketGroupIdType;

        using Entity::Entity;
        virtual ~EveType() = default;

        uint getGroupId() const noexcept;
        void setGroupId(uint id);

        QString getName() const &;
        QString &&getName() && noexcept;
        void setName(const QString &name);
        void setName(QString &&name);

        DescriptionType getDescription() const;
        void setDescription(const DescriptionType &desc);

        double getMass() const noexcept;
        void setMass(double value) noexcept;

        double getVolume() const noexcept;
        void setVolume(double value) noexcept;

        double getCapacity() const noexcept;
        void setCapacity(double value) noexcept;

        int getPortionSize() const noexcept;
        void setPortionSize(int value) noexcept;

        RaceIdType getRaceId() const;
        void setRaceId(const RaceIdType &id);

        double getBasePrice() const noexcept;
        void setBasePrice(double value) noexcept;

        bool isPublished() const noexcept;
        void setPublished(bool flag) noexcept;

        MarketGroupIdType getMarketGroupId() const;
        void setMarketGroupId(const MarketGroupIdType &id);

        double getChanceOfDuplicating() const noexcept;
        void setChanceOfDuplicating(double value) noexcept;

    private:
        uint mGroupId;
        QString mName;
        DescriptionType mDescription;
        double mMass;
        double mVolume;
        double mCapacity;
        int mPortionSize;
        RaceIdType mRaceId;
        double mBasePrice;
        bool mPublished;
        MarketGroupIdType mMarketGroup;
        double mChanceOfDuplicating;
    };
}
