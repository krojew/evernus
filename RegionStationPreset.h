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

#include <unordered_set>

#include <boost/optional.hpp>

#include <QString>

#include "Entity.h"

namespace Evernus
{
    class RegionStationPreset final
        : public Entity<QString>
    {
    public:
        using StationId = boost::optional<quint64>;
        using RegionSet = std::unordered_set<uint>;

        using Entity::Entity;
        RegionStationPreset() = default;
        RegionStationPreset(const RegionStationPreset &) = default;
        RegionStationPreset(RegionStationPreset &&) = default;
        virtual ~RegionStationPreset() = default;

        StationId getStationId() const;
        void setStationId(StationId station);

        RegionSet getRegions() const &;
        RegionSet &&getRegions() && noexcept;
        void setRegions(const RegionSet &regions);
        void setRegions(RegionSet &&regions) noexcept;

        RegionStationPreset &operator =(const RegionStationPreset &) = default;
        RegionStationPreset &operator =(RegionStationPreset &&) = default;

    private:
        StationId mStationId;
        RegionSet mRegions;
    };
}
