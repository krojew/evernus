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
#include <QLocale>

#include "ItemCostRepository.h"
#include "EveDataProvider.h"

#include "ItemCostModel.h"

namespace Evernus
{
    ItemCostModel::ItemCostModel(const ItemCostRepository &itemRepo,
                                 const EveDataProvider &dataProvider,
                                 QObject *parent)
        : QAbstractTableModel{parent}
        , mItemRepo{itemRepo}
        , mDataProvider{dataProvider}
    {
    }

    QVariant ItemCostModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        {
            switch (section) {
            case 0:
                return tr("Type");
            case 1:
                return tr("Cost");
            }
        }

        return QVariant{};
    }

    int ItemCostModel::columnCount(const QModelIndex &parent) const
    {
        return 2;
    }

    QVariant ItemCostModel::data(const QModelIndex &index, int role) const
    {
        if (!index.isValid())
            return QVariant{};

        if (role == Qt::DisplayRole)
        {
            const auto &cost = mData[index.row()];
            switch (index.column()) {
            case 0:
                return mDataProvider.getTypeName(cost.getTypeId());
            case 1:
                return QLocale{}.toCurrencyString(cost.getCost(), "ISK");
            }
        }
        else if (role == Qt::UserRole)
        {
            const auto &cost = mData[index.row()];
            switch (index.column()) {
            case 0:
                return mDataProvider.getTypeName(cost.getTypeId());
            case 1:
                return cost.getCost();
            }
        }

        return QVariant{};
    }

    bool ItemCostModel::removeRows(int row, int count, const QModelIndex &parent)
    {
        return false;
    }

    int ItemCostModel::rowCount(const QModelIndex &parent) const
    {
        return static_cast<int>(mData.size());
    }

    void ItemCostModel::setCharacter(Character::IdType id)
    {
        mCharacterId = id;
        reset();
    }

    void ItemCostModel::reset()
    {
        beginResetModel();

        if (mCharacterId == Character::invalidId)
            mData.clear();
        else
            mData = mItemRepo.fetchForCharacter(mCharacterId);

        endResetModel();
    }

    ItemCost::IdType ItemCostModel::getId(int row) const
    {
        return mData[row].getId();
    }
}
