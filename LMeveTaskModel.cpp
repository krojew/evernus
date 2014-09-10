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

#include "LMeveTaskModel.h"

namespace Evernus
{
    LMeveTaskModel::LMeveTaskModel(const EveDataProvider &dataProvider, QObject *parent)
        : QAbstractTableModel{parent}
        , mDataProvider{dataProvider}
    {
    }

    int LMeveTaskModel::columnCount(const QModelIndex &parent) const
    {
        Q_UNUSED(parent);
        return numColumns;
    }

    QVariant LMeveTaskModel::data(const QModelIndex &index, int role) const
    {
        if (!index.isValid())
            return QVariant{};

        return QVariant{};
    }

    QVariant LMeveTaskModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        return QVariant{};
    }

    int LMeveTaskModel::rowCount(const QModelIndex &parent) const
    {
        return (parent.isValid()) ? (0) : (static_cast<int>(mData.size()));
    }
}
