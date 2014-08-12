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

#include <vector>
#include <memory>

#include "Character.h"

class QDateTime;

namespace Evernus
{
    class MarketOrder;

    class MarketOrderProvider
    {
    public:
        MarketOrderProvider() = default;
        virtual ~MarketOrderProvider() = default;

        virtual std::vector<std::shared_ptr<MarketOrder>> getSellOrders(Character::IdType characterId) const = 0;
        virtual std::vector<std::shared_ptr<MarketOrder>> getBuyOrders(Character::IdType characterId) const = 0;
        virtual std::vector<std::shared_ptr<MarketOrder>> getArchivedOrders(Character::IdType characterId, const QDateTime &from, const QDateTime &to) const = 0;

        virtual std::vector<std::shared_ptr<MarketOrder>> getSellOrdersForCorporation(uint corporationId) const = 0;
        virtual std::vector<std::shared_ptr<MarketOrder>> getBuyOrdersForCorporation(uint corporationId) const = 0;
        virtual std::vector<std::shared_ptr<MarketOrder>> getArchivedOrdersForCorporation(uint corporationId, const QDateTime &from, const QDateTime &to) const = 0;
    };
}
