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
#include <unordered_map>

#include <QLocale>
#include <QDate>

#ifdef Q_CC_MSVC
#   pragma warning(push)
#   pragma warning(disable : 4244)
#endif

#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/median.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/scope_exit.hpp>

#ifdef Q_CC_MSVC
#   pragma warning(pop)
#endif

#include "MarketOrderRepository.h"
#include "CharacterRepository.h"
#include "EveDataProvider.h"
#include "MarketOrder.h"
#include "TextUtils.h"

#include "MarketOrderPerformanceModel.h"

using namespace boost::accumulators;

namespace Evernus
{
    MarketOrderPerformanceModel::MarketOrderPerformanceModel(const MarketOrderRepository &marketOrderRepository,
                                                             const MarketOrderRepository &corpMarketOrderRepository,
                                                             const CharacterRepository &characterRepository,
                                                             const EveDataProvider &dataProvider,
                                                             QObject *parent)
        : QAbstractTableModel{parent}
        , mMarketOrderRepository{marketOrderRepository}
        , mCorpMarketOrderRepository{corpMarketOrderRepository}
        , mCharacterRepository{characterRepository}
        , mDataProvider{dataProvider}
    {
    }

    int MarketOrderPerformanceModel::columnCount(const QModelIndex &parent) const
    {
        Q_UNUSED(parent);
        return numColumns;
    }

    QVariant MarketOrderPerformanceModel::data(const QModelIndex &index, int role) const
    {
        if (!index.isValid())
            return {};

        const auto column = index.column();
        const auto &data = mData[index.row()];

        switch (role) {
        case Qt::DisplayRole:
            {
                QLocale locale;

                switch (column) {
                case nameColumn:
                    return mDataProvider.getTypeName(data.mId);
                case volumeColumn:
                    return locale.toString(data.mVolume);
                case timeColumn:
                    return TextUtils::secondsToString(data.mTurnover);
                }
            }
            break;
        case Qt::UserRole:
            switch (column) {
            case nameColumn:
                return mDataProvider.getTypeName(data.mId);
            case volumeColumn:
                return data.mVolume;
            case timeColumn:
                return static_cast<qlonglong>(data.mTurnover.count());
            }
        }

        return {};
    }

    QVariant MarketOrderPerformanceModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        {
            switch (section) {
            case nameColumn:
                return tr("Name");
            case volumeColumn:
                return tr("Volume");
            case timeColumn:
                return tr("Median turnover time");
            }
        }

        return {};
    }

    int MarketOrderPerformanceModel::rowCount(const QModelIndex &parent) const
    {
        Q_UNUSED(parent);
        return static_cast<int>(mData.size());
    }

    void MarketOrderPerformanceModel::reset(const QDate &from,
                                            const QDate &to,
                                            bool combineCharacters,
                                            bool combineCorp,
                                            Character::IdType characterId)
    {
        beginResetModel();

        BOOST_SCOPE_EXIT(this_) {
            this_->endResetModel();
        } BOOST_SCOPE_EXIT_END

        mData.clear();

        struct OrderData
        {
            quint64 mVolume = 0;
            accumulator_set<int, stats<tag::median>> mMedianTuronover;
        };

        std::unordered_map<EveType::IdType, OrderData> orderMap;

        const auto insertOrders = [&](auto &orders) {
            for (auto &order : orders)
            {
                Q_ASSERT(order);

                auto &data = orderMap[order->getTypeId()];
                data.mVolume += order->getVolumeEntered();
                data.mMedianTuronover(order->getFirstSeen().secsTo(order->getLastSeen()));
            }
        };

        auto orders = (combineCharacters) ?
                      (mMarketOrderRepository.fetchFulfilled(from, to)) :
                      (mMarketOrderRepository.fetchFulfilledForCharacter(from, to, characterId));

        insertOrders(orders);

        if (combineCorp)
        {
            orders = (combineCharacters) ?
                     (mCorpMarketOrderRepository.fetchFulfilled(from, to)) :
                     (mCorpMarketOrderRepository.fetchFulfilledForCorporation(from, to, mCharacterRepository.getCorporationId(characterId)));

            insertOrders(orders);
        }

        mData.reserve(orderMap.size());

        for (const auto &order : orderMap)
        {
            mData.emplace_back();

            auto &data = mData.back();
            data.mId = order.first;
            data.mVolume = order.second.mVolume;
            data.mTurnover = std::chrono::minutes{static_cast<int>(median(order.second.mMedianTuronover)) * 60};
        }
    }
}
