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
#include <vector>

#include <boost/functional/hash.hpp>

#include <QObject>

#include "Character.h"
#include "EveType.h"

namespace Evernus
{
    class ExternalOrder;

    class ExternalOrderImporter
        : public QObject
    {
        Q_OBJECT

    public:
        using TypeLocationPair = std::pair<EveType::IdType, quint64>;
        using TypeLocationPairs = std::unordered_set<TypeLocationPair, boost::hash<TypeLocationPair>>;

        using QObject::QObject;
        virtual ~ExternalOrderImporter() = default;

        virtual void fetchExternalOrders(Character::IdType id, const TypeLocationPairs &target) const = 0;

    signals:
        void externalOrdersChanged(const std::vector<ExternalOrder> &orders) const;
        void error(const QString &info) const;
        void genericError(const QString &info) const;
        void statusChanged(const QString &status) const;
    };
}
