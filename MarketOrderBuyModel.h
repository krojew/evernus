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
#include "CharacterRepository.h"

namespace Evernus
{
    class CharacterRepository;
    class MarketOrderProvider;
    class CacheTimerProvider;

    class MarketOrderBuyModel
        : public MarketOrderTreeModel
    {
        Q_OBJECT

    public:
        MarketOrderBuyModel(const MarketOrderProvider &orderProvider,
                            const EveDataProvider &dataProvider,
                            const CacheTimerProvider &cacheTimerProvider,
                            const CharacterRepository &characterRepository,
                            bool corp,
                            QObject *parent = nullptr);
        virtual ~MarketOrderBuyModel() = default;

        virtual int columnCount(const QModelIndex &parent = QModelIndex{}) const override;
        virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
        virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

        virtual OrderInfo getOrderInfo(const QModelIndex &index) const override;
        virtual WalletTransactionsModel::EntryType getOrderTypeFilter(const QModelIndex &index) const override;

        virtual bool shouldShowPriceInfo(const QModelIndex &index) const override;

        virtual int getVolumeColumn() const override;

        virtual Type getType() const override;

    private:
        static const auto nameColumn = 0;
        static const auto groupColumn = 1;
        static const auto statusColumn = 2;
        static const auto priceColumn = 3;
        static const auto priceStatusColumn = 4;
        static const auto volumeColumn = 5;
        static const auto totalColumn = 6;
        static const auto deltaColumn = 7;
        static const auto marginColumn = 8;
        static const auto newMarginColumn = 9;
        static const auto rangeColumn = 10;
        static const auto minQuantityColumn = 11;
        static const auto etaColumn = 12;
        static const auto timeLeftColumn = 13;
        static const auto orderAgeColumn = 14;
        static const auto firstSeenColumn = 15;
        static const auto stationColumn = 16;
        static const auto ownerColumn = 17;

        static const auto maxColumn = ownerColumn;

        const MarketOrderProvider &mOrderProvider;
        const CacheTimerProvider &mCacheTimerProvider;
        const CharacterRepository &mCharacterRepository;

        Repository<Character>::EntityPtr mCharacter;

        bool mCorp = false;

        mutable std::unordered_map<Character::IdType, QString> mCharacterNames;

        virtual OrderList getOrders() const override;
        virtual void handleNewCharacter() override;

        double getMargin(const MarketOrder &order) const;
        double getNewMargin(const MarketOrder &order) const;
        QString getCharacterName(Character::IdType id) const;

        static QColor getMarginColor(double margin);
    };
}
