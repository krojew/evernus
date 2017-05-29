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

#include <QHash>

#include <boost/functional/hash.hpp>

#include "ExternalOrderRepository.h"
#include "MarketGroupRepository.h"
#include "MetaGroupRepository.h"
#include "EveTypeRepository.h"
#include "Citadel.h"
#include "RefType.h"

#include "EveDataProvider.h"

class QDir;

namespace Evernus
{
    class ConquerableStationRepository;
    class MarketOrderRepository;
    class RefTypeRepository;
    class APIManager;

    class CachingEveDataProvider
        : public EveDataProvider
    {
        Q_OBJECT

    public:
        static const QString systemDistanceCacheFileName;

        CachingEveDataProvider(const EveTypeRepository &eveTypeRepository,
                               const MetaGroupRepository &metaGroupRepository,
                               const ExternalOrderRepository &externalOrderRepository,
                               const MarketOrderRepository &marketOrderRepository,
                               const MarketOrderRepository &corpMarketOrderRepository,
                               const ConquerableStationRepository &conquerableStationRepository,
                               const MarketGroupRepository &marketGroupRepository,
                               const RefTypeRepository &refTypeRepository,
                               const CitadelRepository &citadelRepository,
                               const APIManager &apiManager,
                               const QSqlDatabase &eveDb,
                               QObject *parent = nullptr);
        virtual ~CachingEveDataProvider();

        virtual QString getTypeName(EveType::IdType id) const override;
        virtual QString getTypeMarketGroupParentName(EveType::IdType id) const override;
        virtual QString getTypeMarketGroupName(EveType::IdType id) const override;
        virtual MarketGroup::IdType getTypeMarketGroupParentId(EveType::IdType id) const override;
        virtual const std::unordered_map<EveType::IdType, QString> &getAllTradeableTypeNames() const override;
        virtual QString getTypeMetaGroupName(EveType::IdType id) const override;
        virtual QString getGenericName(quint64 id) const override;

        virtual double getTypeVolume(EveType::IdType id) const override;
        virtual std::shared_ptr<ExternalOrder> getTypeStationSellPrice(EveType::IdType id, quint64 stationId) const override;
        virtual std::shared_ptr<ExternalOrder> getTypeRegionSellPrice(EveType::IdType id, uint regionId) const override;
        virtual std::shared_ptr<ExternalOrder> getTypeBuyPrice(EveType::IdType id, quint64 stationId, int range = -1) const override;

        virtual void updateExternalOrders(const std::vector<ExternalOrder> &orders) override;
        virtual void clearExternalOrders() override;
        virtual void clearExternalOrdersForType(EveType::IdType id) override;

        virtual QString getLocationName(quint64 id) const override;
        virtual QString getRegionName(uint id) const override;
        virtual QString getSolarSystemName(uint id) const override;

        virtual QString getRefTypeName(uint id) const override;

        virtual const std::vector<MapLocation> &getRegions() const override;
        virtual const std::vector<MapLocation> &getConstellations(uint regionId) const override;
        virtual const std::vector<MapLocation> &getSolarSystemsForConstellation(uint constellationId) const override;
        virtual const std::vector<MapLocation> &getSolarSystemsForRegion(uint regionId) const override;
        virtual const std::vector<Station> &getStations(uint solarSystemId) const override;

        virtual double getSolarSystemSecurityStatus(uint solarSystemId) const override;

        virtual uint getStationRegionId(quint64 stationId) const override;
        virtual uint getStationSolarSystemId(quint64 stationId) const override;

        virtual const CitadelRepository::EntityList &getCitadelsForRegion(uint regionId) const override;

        virtual const ReprocessingMap &getOreReprocessingInfo() const override;

        void precacheJumpMap();
        void precacheRefTypes();

        void clearExternalOrderCaches();
        void clearStationCache();
        void clearCitadelCache();

        void handleNewPreferences();

        std::shared_ptr<ExternalOrder> getTypeSellPrice(EveType::IdType id, quint64 stationId, bool dontThrow) const;

        static QDir getCacheDir();

    private:
        typedef std::pair<EveType::IdType, quint64> TypeLocationPair;
        typedef std::pair<EveType::IdType, uint> TypeRegionPair;

        static const QString nameCacheFileName;

        const EveTypeRepository &mEveTypeRepository;
        const MetaGroupRepository &mMetaGroupRepository;
        const ExternalOrderRepository &mExternalOrderRepository;
        const MarketOrderRepository &mMarketOrderRepository, &mCorpMarketOrderRepository;
        const ConquerableStationRepository &mConquerableStationRepository;
        const MarketGroupRepository &mMarketGroupRepository;
        const RefTypeRepository &mRefTypeRepository;
        const CitadelRepository &mCitadelRepository;

        const APIManager &mAPIManager;

        QSqlDatabase mEveDb;

        mutable std::unordered_map<EveType::IdType, QString> mTypeNameCache;
        mutable std::unordered_map<EveType::IdType, MetaGroupRepository::EntityPtr> mTypeMetaGroupCache;
        mutable std::unordered_map<EveType::IdType, EveTypeRepository::EntityPtr> mTypeCache;

        mutable std::unordered_map<TypeLocationPair, ExternalOrderRepository::EntityPtr, boost::hash<TypeLocationPair>>
        mStationSellPrices;
        mutable std::unordered_map<TypeLocationPair, ExternalOrderRepository::EntityPtr, boost::hash<TypeLocationPair>>
        mRegionSellPrices;
        mutable std::unordered_map<TypeLocationPair, ExternalOrderRepository::EntityPtr, boost::hash<TypeLocationPair>>
        mBuyPrices;

        mutable std::unordered_map<TypeRegionPair, ExternalOrderRepository::EntityList, boost::hash<TypeRegionPair>>
        mTypeRegionOrderCache;

        mutable std::unordered_map<quint64, QString> mLocationNameCache;

        mutable std::unordered_map<EveType::IdType, MarketGroupRepository::EntityPtr> mTypeMarketGroupParentCache;
        mutable std::unordered_map<EveType::IdType, MarketGroupRepository::EntityPtr> mTypeMarketGroupCache;

        std::unordered_map<RefType::IdType, QString> mRefTypeNames;

        mutable QHash<quint64, QString> mGenericNameCache;
        mutable std::unordered_set<quint64> mPendingNameRequests;

        std::unordered_map<uint, std::unordered_multimap<uint, uint>> mSystemJumpMap;

        mutable std::unordered_map<uint, uint> mSolarSystemRegionCache;
        mutable std::unordered_map<quint64, uint> mLocationSolarSystemCache;

        mutable std::unordered_map<uint, double> mSecurityStatuses;

        mutable std::vector<MapLocation> mRegionCache;
        mutable std::unordered_map<uint, std::vector<MapLocation>> mConstellationCache, mConstellationSolarSystemCache, mRegionSolarSystemCache;
        mutable std::unordered_map<uint, std::vector<Station>> mStationCache;
        mutable std::unordered_map<Citadel::IdType, CitadelRepository::EntityPtr> mCitadelCache;
        mutable std::unordered_map<uint, CitadelRepository::EntityList> mRegionCitadelCache;

        mutable std::unordered_map<uint, QString> mRegionNameCache;
        mutable std::unordered_map<uint, QString> mSolarSystemNameCache;

        mutable std::unordered_map<quint64, uint> mStationRegionCache;

        bool mUsePackagedVolume = false;

        mutable QHash<QPair<uint, uint>, uint> mSystemDistances;

        mutable ReprocessingMap mOreReprocessingInfo;

        EveTypeRepository::EntityPtr getEveType(EveType::IdType id) const;

        MarketGroupRepository::EntityPtr getMarketGroupParent(MarketGroup::IdType id) const;
        MarketGroupRepository::EntityPtr getMarketGroup(MarketGroup::IdType id) const;

        uint getSolarSystemRegionId(uint stationId) const;

        const ExternalOrderRepository::EntityList &getExternalOrders(EveType::IdType typeId, uint regionId) const;

        uint getDistance(uint startSystem, uint endSystem) const;

        QString getCitadelName(Citadel::IdType id) const;
        uint getCitadelRegionId(Citadel::IdType id) const;
        uint getCitadelSolarSystemId(Citadel::IdType id) const;
        const Citadel &getCitadel(Citadel::IdType id) const;

        static double getPackagedVolume(const EveType &type);
    };
}
