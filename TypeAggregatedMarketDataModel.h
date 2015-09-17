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
#include <memory>
#include <vector>
#include <map>

#include <QAbstractTableModel>

#include "MarketHistoryEntry.h"
#include "Character.h"
#include "EveType.h"

class QColor;

namespace Evernus
{
    class EveDataProvider;
    class ExternalOrder;

    class TypeAggregatedMarketDataModel
        : public QAbstractTableModel
    {
        Q_OBJECT

    public:
        template<class T>
        using TypeMap = std::unordered_map<EveType::IdType, T>;
        using HistoryMap = TypeMap<std::map<QDate, MarketHistoryEntry>>;

        explicit TypeAggregatedMarketDataModel(const EveDataProvider &dataProvider, QObject *parent = nullptr);
        virtual ~TypeAggregatedMarketDataModel() = default;

        virtual int columnCount(const QModelIndex &parent = QModelIndex{}) const override;
        virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
        virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
        virtual int rowCount(const QModelIndex &parent = QModelIndex{}) const override;

        void setOrderData(const std::vector<ExternalOrder> &orders, const HistoryMap &history, uint region, uint solarSystem = 0);
        void setCharacter(const std::shared_ptr<Character> &character);
        void discardBogusOrders(bool flag);
        void setBogusOrderThreshold(double value);

        EveType::IdType getTypeId(const QModelIndex &index) const;

        static int getScoreColumn() noexcept;
        static int getVolumeColumn() noexcept;
        static int getMarginColumn() noexcept;
        static int getBuyPriceColumn() noexcept;
        static int getSellPriceColumn() noexcept;

    private:
        enum
        {
            nameColumn,
            scoreColumn,
            buyPriceColumn,
            sellPriceColumn,
            differenceColumn,
            volumeColumn,
            marginColumn,

            numColumns
        };

        struct TypeData
        {
            EveType::IdType mId = EveType::invalidId;
            double mBuyPrice = 0.;
            double mSellPrice = 0.;
            uint mVolume = 0;
        };

        const EveDataProvider &mDataProvider;

        std::vector<TypeData> mData;

        std::shared_ptr<Character> mCharacter;

        bool mDiscardBogusOrders = true;
        double mBogusOrderThreshold = 0.9;

        double getMargin(const TypeData &data) const;
    };
}
