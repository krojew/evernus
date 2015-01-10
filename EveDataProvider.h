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

#include <unordered_map>
#include <vector>
#include <memory>

#include <QDateTime>
#include <QObject>

#include "MarketGroup.h"
#include "MetaGroup.h"
#include "EveType.h"

namespace Evernus
{
    class ExternalOrder;

    class EveDataProvider
        : public QObject
    {
        Q_OBJECT

    public:
        typedef std::pair<uint, QString> MapLocation;
        typedef std::pair<quint64, QString> Station;

        using QObject::QObject;
        virtual ~EveDataProvider() = default;

        virtual QString getTypeName(EveType::IdType id) const = 0;
        virtual QString getTypeMarketGroupParentName(EveType::IdType id) const = 0;
        virtual QString getTypeMarketGroupName(EveType::IdType id) const = 0;
        virtual MarketGroup::IdType getTypeMarketGroupParentId(EveType::IdType id) const = 0;
        virtual const std::unordered_map<EveType::IdType, QString> &getAllTradeableTypeNames() const = 0;
        virtual QString getTypeMetaGroupName(EveType::IdType id) const = 0;
        virtual QString getGenericName(quint64 id) const = 0;

        virtual double getTypeVolume(EveType::IdType id) const = 0;
        virtual std::shared_ptr<ExternalOrder> getTypeSellPrice(EveType::IdType id, quint64 stationId) const = 0;
        virtual std::shared_ptr<ExternalOrder> getTypeBuyPrice(EveType::IdType id, quint64 stationId, int range = -1) const = 0;

        virtual void updateExternalOrders(const std::vector<ExternalOrder> &orders) = 0;
        virtual void clearExternalOrders() = 0;
        virtual void clearExternalOrdersForType(EveType::IdType id) = 0;

        virtual QString getLocationName(quint64 id) const = 0;
        virtual QString getRegionName(uint id) const = 0;
        virtual QString getSolarSystemName(uint id) const = 0;

        virtual QString getRefTypeName(uint id) const = 0;

        virtual const std::vector<MapLocation> &getRegions() const = 0;
        virtual const std::vector<MapLocation> &getConstellations(uint regionId) const = 0;
        virtual const std::vector<MapLocation> &getSolarSystemsForConstellation(uint constellationId) const = 0;
        virtual const std::vector<MapLocation> &getSolarSystemsForRegion(uint regionId) const = 0;
        virtual const std::vector<Station> &getStations(uint solarSystemId) const = 0;

        virtual double getSolarSystemSecurityStatus(uint solarSystemId) const = 0;

        virtual uint getStationRegionId(quint64 stationId) const = 0;
        virtual uint getStationSolarSystemId(quint64 stationId) const = 0;

    signals:
        void namesChanged() const;
    };
}
