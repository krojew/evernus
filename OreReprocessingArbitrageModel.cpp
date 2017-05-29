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

#include "OreReprocessingArbitrageModel.h"

namespace Evernus
{
    OreReprocessingArbitrageModel::OreReprocessingArbitrageModel(const EveDataProvider &dataProvider, QObject *parent)
        : QAbstractTableModel{parent}
        , ModelWithTypes{}
        , mDataProvider{dataProvider}
    {
    }

    int OreReprocessingArbitrageModel::columnCount(const QModelIndex &parent) const
    {
        Q_UNUSED(parent);
        return numColumns;
    }

    QVariant OreReprocessingArbitrageModel::data(const QModelIndex &index, int role) const
    {
        return {};
    }

    QVariant OreReprocessingArbitrageModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        return {};
    }

    int OreReprocessingArbitrageModel::rowCount(const QModelIndex &parent) const
    {
        return 0;
    }

    EveType::IdType OreReprocessingArbitrageModel::getTypeId(const QModelIndex &index) const
    {
        return EveType::invalidId;
    }

    void OreReprocessingArbitrageModel::setCharacter(std::shared_ptr<Character> character)
    {
        beginResetModel();
        mCharacter = std::move(character);
        endResetModel();
    }

    void OreReprocessingArbitrageModel::setOrderData(const std::vector<ExternalOrder> &orders,
                                                     PriceType srcPriceType,
                                                     PriceType dstPriceType,
                                                     bool useStationTax,
                                                     bool ignoreMinVolume)
    {
        beginResetModel();
        endResetModel();
    }

    void OreReprocessingArbitrageModel::reset()
    {
        beginResetModel();
        endResetModel();
    }
}
