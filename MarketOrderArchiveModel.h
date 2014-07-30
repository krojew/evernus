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

#include "MarketOrderTreeModel.h"

namespace Evernus
{
    class MarketOrderProvider;
    class CacheTimerProvider;
    class ItemCostProvider;

    class MarketOrderArchiveModel
        : public MarketOrderTreeModel
    {
        Q_OBJECT

    public:
        MarketOrderArchiveModel(const MarketOrderProvider &orderProvider,
                                const EveDataProvider &dataProvider,
                                const ItemCostProvider &itemCostProvider,
                                QObject *parent = nullptr);
        virtual ~MarketOrderArchiveModel() = default;

        virtual int columnCount(const QModelIndex &parent = QModelIndex{}) const override;
        virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
        virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

        virtual OrderInfo getOrderInfo(const QModelIndex &index) const override;
        virtual WalletTransactionsModel::EntryType getOrderTypeFilter(const QModelIndex &index) const override;

        virtual bool shouldShowPriceInfo(const QModelIndex &index) const override;

        virtual int getVolumeColumn() const override;

        void setCharacterAndRange(Character::IdType id, const QDateTime &from, const QDateTime &to);

    private:
        static const auto lastSeenColumn = 0;
        static const auto typeColumn = 1;
        static const auto nameColumn = 2;
        static const auto groupColumn = 3;
        static const auto statusColumn = 4;
        static const auto customCostColumn = 5;
        static const auto priceColumn = 6;
        static const auto volumeColumn = 7;
        static const auto profitColumn = 8;
        static const auto stationColumn = 9;

        const MarketOrderProvider &mOrderProvider;
        const ItemCostProvider &mItemCostProvider;

        QDateTime mFrom, mTo;

        virtual OrderList getOrders() const override;
    };
}
