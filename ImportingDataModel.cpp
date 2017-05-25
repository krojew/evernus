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
#include <type_traits>

#include <QCoreApplication>
#include <QLocale>

#include <boost/range/adaptor/filtered.hpp>
#include <boost/scope_exit.hpp>

#include "EveDataProvider.h"
#include "ExternalOrder.h"
#include "MathUtils.h"

#include "ImportingDataModel.h"

namespace Evernus
{
    ImportingDataModel::ImportingDataModel(const EveDataProvider &dataProvider, QObject *parent)
        : QAbstractTableModel{parent}
        , mDataProvider{dataProvider}
    {
    }

    int ImportingDataModel::columnCount(const QModelIndex &parent) const
    {
        Q_UNUSED(parent);
        return numColumns;
    }

    QVariant ImportingDataModel::data(const QModelIndex &index, int role) const
    {
        if (!index.isValid())
            return QVariant{};

        const auto column = index.column();
        const auto &data = mData[index.row()];

        switch (role) {
        case Qt::DisplayRole:
            {
                QLocale locale;

                switch (column) {
                case nameColumn:
                    return mDataProvider.getTypeName(data.mId);
                case avgVolumeColumn:
                    return locale.toString(data.mAvgVolume);
                case dstVolume:
                    return locale.toString(data.mDstVolume);
                case relativeDstVolume:
                    return QStringLiteral("%1%2").arg(locale.toString(data.mDstVolume * 100 / data.mAvgVolume, 'f', 2)).arg(locale.percent());
                }
            }
            break;
        case Qt::UserRole:
            switch (column) {
            case nameColumn:
                return mDataProvider.getTypeName(data.mId);
            case avgVolumeColumn:
                return data.mAvgVolume;
            case dstVolume:
                return data.mDstVolume;
            case relativeDstVolume:
                return data.mDstVolume * 100 / data.mAvgVolume;
            }
        }

        return QVariant{};
    }

    QVariant ImportingDataModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        {
            switch (section) {
            case nameColumn:
                return tr("Name");
            case avgVolumeColumn:
                return tr("Avg. dst. volume");
            case dstVolume:
                return tr("Dst. remaining volume");
            case relativeDstVolume:
                return tr("Relative dst. remaining volume");
            }
        }

        return QVariant{};
    }

    int ImportingDataModel::rowCount(const QModelIndex &parent) const
    {
        return static_cast<int>(mData.size());
    }

    void ImportingDataModel::setCharacter(const std::shared_ptr<Character> &character)
    {
        beginResetModel();
        mCharacter = character;
        endResetModel();
    }

    void ImportingDataModel::discardBogusOrders(bool flag) noexcept
    {
        mDiscardBogusOrders = flag;
    }

    void ImportingDataModel::setBogusOrderThreshold(double value) noexcept
    {
        mBogusOrderThreshold = value;
    }

    EveType::IdType ImportingDataModel::getTypeId(const QModelIndex &index) const
    {
        if (!index.isValid())
            return EveType::invalidId;

        return mData[index.row()].mId;
    }

    void ImportingDataModel::setOrderData(const std::vector<ExternalOrder> &orders,
                                          const HistoryRegionMap &history,
                                          quint64 srcStation,
                                          quint64 dstStation,
                                          PriceType srcType,
                                          PriceType dstType,
                                          int aggrDays)
    {
        beginResetModel();

        BOOST_SCOPE_EXIT(this_) {
            this_->endResetModel();
        } BOOST_SCOPE_EXIT_END

        mData.clear();

        const auto dstHistory = history.find(mDataProvider.getStationRegionId(dstStation));
        if (dstHistory == std::end(history))
            return;

        struct TypeData
        {
            quint64 mVolume = 0;
            double mSellPrice = 0.;
        };

        TypeMap<TypeData> typeMap;
        TypeMap<quint64> sellVolumes;
        TypeMap<std::vector<std::reference_wrapper<const ExternalOrder>>> sellOrders;

        const auto dstOrderFilter = [=](const auto &order) {
            return order.getStationId() == dstStation;
        };

        for (const auto &order : orders | boost::adaptors::filtered(dstOrderFilter))
        {
            const auto typeId = order.getTypeId();
            if (order.getType() == ExternalOrder::Type::Sell)
            {
                sellOrders[typeId].emplace_back(std::cref(order));
                sellVolumes[typeId] += order.getVolumeRemaining();

                QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
            }
        }

        for (const auto &order : orders)
        {
            const auto typeId = order.getTypeId();
            const auto type = typeMap.find(typeId);
            if (type != std::end(typeMap))
                continue;

            auto &data = typeMap[typeId];
            auto avgPrice = 0.;

            const auto dstTypeHistory = dstHistory->second.find(typeId);
            if (dstTypeHistory != std::end(dstHistory->second) && !dstTypeHistory->second.empty())
            {
//                for (const auto &timePoint : dstTypeHistory->second)
//                {
//                    if (timePoint.first < historyLimit)
//                        break;
//
//                    data.mVolume += timePoint.second.mVolume;
//                    avgPrice += timePoint.second.mAvgPrice;
//                }
//
//                avgPrice /= aggrDays;
            }

            data.mSellPrice = MathUtils::calcPercentile(sellOrders[typeId],
                                                        sellVolumes[typeId] * 0.05,
                                                        avgPrice,
                                                        mDiscardBogusOrders,
                                                        mBogusOrderThreshold);

            QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        }

        mData.reserve(typeMap.size());

        for (const auto &type : typeMap)
        {
            mData.emplace_back();

            auto &data = mData.back();
            data.mId = type.first;
            data.mAvgVolume = static_cast<double>(type.second.mVolume) / aggrDays;
            data.mDstVolume = sellVolumes[data.mId];
        }
    }
}
