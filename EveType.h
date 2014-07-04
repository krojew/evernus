/**
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
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
        uint mGroupId = 0;
        QString mName;
        DescriptionType mDescription;
        double mMass = 0.;
        double mVolume = 0.;
        double mCapacity = 0.;
        int mPortionSize = 0;
        RaceIdType mRaceId;
        double mBasePrice = 0.;
        bool mPublished = true;
        MarketGroupIdType mMarketGroup;
        double mChanceOfDuplicating = 0.;
    };
}
