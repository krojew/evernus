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
#include <QDate>

#include "MarketHistoryEntry.h"
#include "Character.h"
#include "PriceType.h"
#include "EveType.h"

namespace Evernus
{
    class EveDataProvider;
    class ExternalOrder;

    class ImportingDataModel
        : public QAbstractTableModel
    {
    public:
        template<class T>
        using TypeMap = std::unordered_map<EveType::IdType, T>;
        template<class T>
        using RegionMap = std::unordered_map<uint, T>;
        using HistoryTypeMap = TypeMap<std::map<QDate, MarketHistoryEntry>>;
        using HistoryRegionMap = RegionMap<HistoryTypeMap>;

        explicit ImportingDataModel(const EveDataProvider &dataProvider, QObject *parent = nullptr);
        ImportingDataModel(const ImportingDataModel &) = default;
        ImportingDataModel(ImportingDataModel &&) = default;
        virtual ~ImportingDataModel() = default;

        virtual int columnCount(const QModelIndex &parent = QModelIndex{}) const override;
        virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
        virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
        virtual int rowCount(const QModelIndex &parent = QModelIndex{}) const override;

        void setCharacter(const std::shared_ptr<Character> &character);
        void discardBogusOrders(bool flag) noexcept;
        void setBogusOrderThreshold(double value) noexcept;

        EveType::IdType getTypeId(const QModelIndex &index) const;

        void setOrderData(const std::vector<ExternalOrder> &orders,
                          const HistoryRegionMap &history,
                          quint64 srcStation,
                          quint64 dstStation,
                          PriceType srcType,
                          PriceType dstType,
                          int aggrDays);

        ImportingDataModel &operator =(const ImportingDataModel &) = default;
        ImportingDataModel &operator =(ImportingDataModel &&) = default;

    private:
        enum
        {
            nameColumn,
            avgVolumeColumn,
            dstVolume,
            relativeDstVolume,

            numColumns
        };

        struct TypeData
        {
            EveType::IdType mId = EveType::invalidId;
            double mAvgVolume = 0.;
            quint64 mDstVolume = 0;
        };

        const EveDataProvider &mDataProvider;

        std::shared_ptr<Character> mCharacter;

        std::vector<TypeData> mData;

        bool mDiscardBogusOrders = true;
        double mBogusOrderThreshold = 0.9;
    };
}
