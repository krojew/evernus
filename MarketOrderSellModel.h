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

#include "MarketOrderTreeModel.h"
#include "Repository.h"

namespace Evernus
{
    class MarketOrderProvider;
    class CharacterRepository;
    class CacheTimerProvider;
    class ItemCostProvider;

    class MarketOrderSellModel
        : public MarketOrderTreeModel
    {
        Q_OBJECT

    public:
        MarketOrderSellModel(const MarketOrderProvider &orderProvider,
                             const EveDataProvider &dataProvider,
                             const ItemCostProvider &itemCostProvider,
                             const CacheTimerProvider &cacheTimerProvider,
                             const CharacterRepository &characterRepository,
                             bool corp,
                             QObject *parent = nullptr);
        virtual ~MarketOrderSellModel() = default;

        virtual int columnCount(const QModelIndex &parent = QModelIndex{}) const override;
        virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
        virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

        virtual OrderInfo getOrderInfo(const QModelIndex &index) const override;
        virtual WalletTransactionsModel::EntryType getOrderTypeFilter(const QModelIndex &index) const override;

        virtual bool shouldShowPriceInfo(const QModelIndex &index) const override;

        virtual int getVolumeColumn() const override;

        virtual Type getType() const override;

    private:
        enum
        {
            nameColumn,
            groupColumn,
            statusColumn,
            customCostColumn,
            priceColumn,
            priceStatusColumn,
            priceDifferenceColumn,
            volumeColumn,
            totalColumn,
            deltaColumn,
            marginColumn,
            profitColumn,
            totalProfitColumn,
            profitPerItemColumn,
            etaColumn,
            timeLeftColumn,
            orderAgeColumn,
            firstSeenColumn,
            stationColumn,
            ownerColumn,

            numColumns
        };

        const MarketOrderProvider &mOrderProvider;
        const ItemCostProvider &mItemCostProvider;
        const CacheTimerProvider &mCacheTimerProvider;
        const CharacterRepository &mCharacterRepository;

        bool mCorp = false;

        Repository<Character>::EntityPtr mCharacter;

        mutable std::unordered_map<Character::IdType, QString> mCharacterNames;

        virtual OrderList getOrders() const override;
        virtual void handleNewCharacter() override;

        QString getCharacterName(Character::IdType id) const;
    };
}
