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
    }

    bool CitadelLocationModel::canFetchMore(const QModelIndex &parent) const
    {
        if (!parent.isValid())
            return mRegions.empty();

        const auto node = static_cast<const LocationNode *>(parent.internalPointer());
        switch (node->mType) {
        case LocationNode::Type::Region:
            return mConstellations.find(node->mId) == std::end(mConstellations);
        case LocationNode::Type::Constellation:
            return mSolarSystems.find(node->mId) == std::end(mSolarSystems);
        case LocationNode::Type::SolarSystem:
            return mCitadels.find(node->mId) == std::end(mCitadels);
        default:
            return false;
        }
    }

    int CitadelLocationModel::columnCount(const QModelIndex &parent) const
    {
        Q_UNUSED(parent);
        return 1;
    }

    QVariant CitadelLocationModel::data(const QModelIndex &index, int role) const
    {
        if (!index.isValid())
            return {};

        if (role == Qt::DisplayRole)
            return static_cast<const LocationNode *>(index.internalPointer())->mName;

        return {};
    }

    void CitadelLocationModel::fetchMore(const QModelIndex &parent)
    {
        if (!parent.isValid())
        {
            const auto &regions = mDataProvider.getRegions();
            mRegions.reserve(regions.size());

            for (const auto &region : regions)
                mRegions.emplace_back(LocationNode{region.first, nullptr, mRegions.size(), region.second, LocationNode::Type::Region});

            return;
        }

        const auto node = static_cast<LocationNode *>(parent.internalPointer());
        switch (node->mType) {
        case LocationNode::Type::Region:
            {
                const auto &constellations = mDataProvider.getConstellations(node->mId);
                auto &target = mConstellations[node->mId];

                target.reserve(constellations.size());

                for (const auto &constellation : constellations)
                    target.emplace_back(LocationNode{constellation.first, node, target.size(), constellation.second, LocationNode::Type::Constellation});

                break;
            }
        case LocationNode::Type::Constellation:
            {
                const auto &systems = mDataProvider.getSolarSystemsForConstellation(node->mId);
                auto &target = mSolarSystems[node->mId];

                target.reserve(systems.size());

                for (const auto &system : systems)
                    target.emplace_back(LocationNode{system.first, node, target.size(), system.second, LocationNode::Type::SolarSystem});

                break;
            }
        case LocationNode::Type::SolarSystem:
            {
                const auto &citadels = mDataProvider.getCitadelsForSolarSystem(node->mId);
                auto &target = mCitadels[node->mId];

                target.reserve(citadels.size());

                for (const auto &citadel : citadels)
                    target.emplace_back(LocationNode{citadel->getId(), node, target.size(), citadel->getName(), LocationNode::Type::Citadel});

                break;
            }
        default:
            break;
        }
    }

    bool CitadelLocationModel::hasChildren(const QModelIndex &parent) const
    {
        if (!parent.isValid())
            return true;

        const auto node = static_cast<LocationNode *>(parent.internalPointer());
        return node->mType == LocationNode::Type::Region ||
               node->mType == LocationNode::Type::Constellation ||
               node->mType == LocationNode::Type::SolarSystem;
    }

    QModelIndex CitadelLocationModel::index(int row, int column, const QModelIndex &parent) const
    {
        if (column != 0)
            return QModelIndex{};

        if (!parent.isValid())
        {
            // https://bugreports.qt-project.org/browse/QTBUG-40624
            if (mRegions.empty())
                return {};

            return createIndex(row, column, &mRegions[row]);
        }

        const auto node = static_cast<const LocationNode *>(parent.internalPointer());
        switch (node->mType) {
        case LocationNode::Type::Region:
            return createIndex(row, column, &mConstellations[node->mId][row]);
        case LocationNode::Type::Constellation:
            return createIndex(row, column, &mSolarSystems[node->mId][row]);
        case LocationNode::Type::SolarSystem:
            return createIndex(row, column, &mCitadels[node->mId][row]);
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
        endResetModel();
    }
}
