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

#include <memory>
#include <vector>

#include <QAbstractTableModel>

#include "ModelWithTypes.h"
#include "Character.h"
#include "PriceType.h"

namespace Evernus
{
    class EveDataProvider;
    class ExternalOrder;

    class OreReprocessingArbitrageModel
        : public QAbstractTableModel
        , public ModelWithTypes
    {
        Q_OBJECT

    public:
        explicit OreReprocessingArbitrageModel(const EveDataProvider &dataProvider,
                                               QObject *parent = nullptr);
        OreReprocessingArbitrageModel(const OreReprocessingArbitrageModel &) = default;
        OreReprocessingArbitrageModel(OreReprocessingArbitrageModel &&) = default;
        virtual ~OreReprocessingArbitrageModel() = default;

        virtual int columnCount(const QModelIndex &parent = QModelIndex{}) const override;
        virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
        virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
        virtual int rowCount(const QModelIndex &parent = QModelIndex{}) const override;

        virtual EveType::IdType getTypeId(const QModelIndex &index) const override;

        void setCharacter(std::shared_ptr<Character> character);
        void setOrderData(const std::vector<ExternalOrder> &orders,
                          PriceType srcPriceType,
                          PriceType dstPriceType,
                          bool useStationTax,
                          bool ignoreMinVolume);

        void reset();

        OreReprocessingArbitrageModel &operator =(const OreReprocessingArbitrageModel &) = default;
        OreReprocessingArbitrageModel &operator =(OreReprocessingArbitrageModel &&) = default;

    private:
        enum
        {
            numColumns
        };

        const EveDataProvider &mDataProvider;

        std::shared_ptr<Character> mCharacter;
    };
}
