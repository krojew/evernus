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
#include <boost/scope_exit.hpp>

#include "EveDataProvider.h"

#include "CitadelLocationModel.h"

namespace Evernus
{
    CitadelLocationModel::LocationNode::LocationNode(quint64 id, LocationNode *parent, size_t row, const QString &name, Type type)
        : mId{id}
        , mParent{parent}
        , mRow{row}
        , mName{name}
        , mType{type}
    {
    }

    CitadelLocationModel::CitadelLocationModel(const EveDataProvider &dataProvider, QObject *parent)
        : QAbstractItemModel{parent}
        , mDataProvider{dataProvider}
    {
        blockSignals(true);

        fillStaticData();
        refresh();

        blockSignals(false);
    }

    int CitadelLocationModel::columnCount(const QModelIndex &parent) const
    {
        Q_UNUSED(parent);
        return 1;
    }

    QVariant CitadelLocationModel::data(const QModelIndex &index, int role) const
    {
        if (Q_UNLIKELY(!index.isValid()))
            return {};

        const auto item = static_cast<const LocationNode *>(index.internalPointer());

        switch (role) {
        case Qt::DisplayRole:
            return item->mName;
//        case Qt::CheckStateRole:
        }

        return {};
    }

    bool CitadelLocationModel::hasChildren(const QModelIndex &parent) const
    {
        if (!parent.isValid())
            return true;

        const auto node = static_cast<LocationNode *>(parent.internalPointer());
        return node->mType == LocationNode::Type::Region ||
               node->mType == LocationNode::Type::Constellation ||
               (node->mType == LocationNode::Type::SolarSystem && mCitadels.find(node->mId) != std::end(mCitadels));
    }

    QModelIndex CitadelLocationModel::index(int row, int column, const QModelIndex &parent) const
    {
        if (column != 0)
            return {};

        if (!parent.isValid())
        {
            // https://bugreports.qt-project.org/browse/QTBUG-40624
            if (mRegions.empty())
                return {};

            return createIndex(row, column, mRegions[row].get());
        }

        const auto node = static_cast<const LocationNode *>(parent.internalPointer());
        switch (node->mType) {
        case LocationNode::Type::Region:
            return createIndex(row, column, mConstellations[node->mId][row].get());
        case LocationNode::Type::Constellation:
            return createIndex(row, column, mSolarSystems[node->mId][row].get());
        case LocationNode::Type::SolarSystem:
            return createIndex(row, column, mCitadels[node->mId][row].get());
        default:
            return {};
        }
    }

    QModelIndex CitadelLocationModel::parent(const QModelIndex &index) const
    {
        if (!index.isValid())
            return {};

        const auto node = static_cast<const LocationNode *>(index.internalPointer());
        switch (node->mType) {
        case LocationNode::Type::Region:
            return {};
        case LocationNode::Type::Constellation:
        case LocationNode::Type::SolarSystem:
        case LocationNode::Type::Citadel:
            return createIndex(static_cast<int>(node->mParent->mRow), 0, node->mParent);
        }

        return {};
    }

    int CitadelLocationModel::rowCount(const QModelIndex &parent) const
    {
        if (!parent.isValid())
            return static_cast<int>(mRegions.size());

        const auto node = static_cast<const LocationNode *>(parent.internalPointer());
        switch (node->mType) {
        case LocationNode::Type::Region:
            return static_cast<int>(mConstellations[node->mId].size());
        case LocationNode::Type::Constellation:
            return static_cast<int>(mSolarSystems[node->mId].size());
        case LocationNode::Type::SolarSystem:
            return static_cast<int>(mCitadels[node->mId].size());
        default:
            return 0;
        }
    }

    void CitadelLocationModel::refresh()
    {
        beginResetModel();

        BOOST_SCOPE_EXIT(this_) {
            this_->endResetModel();
        } BOOST_SCOPE_EXIT_END

        mCitadels.clear();

        const auto &citadels = mDataProvider.getCitadels();
        for (const auto &citadel : citadels)
        {
            const auto solarSystem = mSolarSystemMap.find(citadel->getSolarSystemId());
            if (Q_LIKELY(solarSystem != std::end(mSolarSystemMap)))
            {
                auto &target = mCitadels[citadel->getSolarSystemId()];
                target.emplace_back(std::make_unique<LocationNode>(citadel->getId(), solarSystem->second, target.size(), citadel->getName(), LocationNode::Type::Citadel));
            }
        }
    }

    void CitadelLocationModel::fillStaticData()
    {
        beginResetModel();

        BOOST_SCOPE_EXIT(this_) {
            this_->endResetModel();
        } BOOST_SCOPE_EXIT_END

        const auto &regions = mDataProvider.getRegions();
        mRegions.reserve(regions.size());

        LocationCache regionMap, constellationMap;

        for (const auto &region : regions)
        {
            mRegions.emplace_back(std::make_unique<LocationNode>(region.first, nullptr, mRegions.size(), region.second, LocationNode::Type::Region));
            regionMap[mRegions.back()->mId] = mRegions.back().get();
        }

        const auto &constellations = mDataProvider.getConstellations();
        mConstellations.reserve(constellations.size());

        for (const auto &constellation : constellations)
        {
            const auto region = regionMap.find(constellation.mParent);
            if (Q_LIKELY(region != std::end(regionMap)))
            {
                auto &target = mConstellations[constellation.mParent];
                target.emplace_back(std::make_unique<LocationNode>(constellation.mId, region->second, target.size(), constellation.mName, LocationNode::Type::Constellation));
                constellationMap[target.back()->mId] = target.back().get();
            }
        }

        const auto &solarSystems = mDataProvider.getSolarSystems();
        mSolarSystems.reserve(solarSystems.size());

        for (const auto &solarSystem : solarSystems)
        {
            const auto constellation = constellationMap.find(solarSystem.mParent);
            if (Q_LIKELY(constellation != std::end(constellationMap)))
            {
                auto &target = mSolarSystems[solarSystem.mParent];
                target.emplace_back(std::make_unique<LocationNode>(solarSystem.mId, constellation->second, target.size(), solarSystem.mName, LocationNode::Type::SolarSystem));
                mSolarSystemMap[target.back()->mId] = target.back().get();
            }
        }
    }
}
