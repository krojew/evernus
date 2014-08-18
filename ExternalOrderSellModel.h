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
#include <memory>

#include "ExternalOrderModel.h"
#include "ExternalOrder.h"
#include "MarketOrder.h"
#include "Character.h"

namespace Evernus
{
    class ExternalOrderRepository;
    class CharacterRepository;
    class MarketOrderProvider;
    class EveDataProvider;

    class ExternalOrderSellModel
        : public ExternalOrderModel
    {
        Q_OBJECT

    public:
        ExternalOrderSellModel(const EveDataProvider &dataProvider,
                               const ExternalOrderRepository &orderRepo,
                               const CharacterRepository &characterRepo,
                               const MarketOrderProvider &orderProvider,
                               const MarketOrderProvider &corpOrderProvider,
                               QObject *parent = nullptr);
        virtual ~ExternalOrderSellModel() = default;

        virtual int columnCount(const QModelIndex &parent = QModelIndex{}) const override;
        virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
        virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
        virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex{}) const override;
        virtual QModelIndex parent(const QModelIndex &index) const override;
        virtual int rowCount(const QModelIndex &parent = QModelIndex{}) const override;

        virtual int getPriceColumn() const override;
        virtual Qt::SortOrder getPriceSortOrder() const override;
        virtual int getVolumeColumn() const override;

        void setCharacter(Character::IdType id);
        void setType(EveType::IdType id);
        void setStationId(uint id);

        void reset();

    private:
        static const auto stationColumn = 0;
        static const auto deviationColumn = 1;
        static const auto priceColumn = 2;
        static const auto volumeColumn = 3;
        static const auto totalProfitColumn = 4;
        static const auto totalSizeColumn = 5;
        static const auto issuedColumn = 6;
        static const auto durationColumn = 7;
        static const auto updatedColumn = 8;

        const EveDataProvider &mDataProvider;
        const ExternalOrderRepository &mOrderRepo;
        const CharacterRepository &mCharacterRepo;
        const MarketOrderProvider &mOrderProvider;
        const MarketOrderProvider &mCorpOrderProvider;

        EveType::IdType mTypeId = EveType::invalidId;
        uint mStationId = 0;

        std::vector<std::shared_ptr<ExternalOrder>> mOrders;

        std::unordered_set<MarketOrder::IdType> mOwnOrders;
    };
}