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
#include "IndustryManufacturingSetup.h"
#include "EveDataProvider.h"

#include "IndustryManufacturingSetupModel.h"

namespace Evernus
{
    IndustryManufacturingSetupModel::IndustryManufacturingSetupModel(IndustryManufacturingSetup &setup,
                                                                     const EveDataProvider &dataProvider,
                                                                     QObject *parent)
        : QAbstractItemModel{parent}
        , mSetup{setup}
        , mDataProvider{dataProvider}
    {
    }

    int IndustryManufacturingSetupModel::columnCount(const QModelIndex &parent) const
    {
        Q_UNUSED(parent);
        return 1;
    }

    QVariant IndustryManufacturingSetupModel::data(const QModelIndex &index, int role) const
    {
        if (Q_UNLIKELY(!index.isValid()))
            return {};

        return {};
    }

    QModelIndex IndustryManufacturingSetupModel::index(int row, int column, const QModelIndex &parent) const
    {
        return {};
    }

    QModelIndex IndustryManufacturingSetupModel::parent(const QModelIndex &index) const
    {
        return {};
    }

    QHash<int, QByteArray> IndustryManufacturingSetupModel::roleNames() const
    {
        return {
            { NameRole, QByteArrayLiteral("name") }
        };
    }

    int IndustryManufacturingSetupModel::rowCount(const QModelIndex &parent) const
    {
        return 0;
    }

    void IndustryManufacturingSetupModel::refreshData()
    {
        beginResetModel();
        endResetModel();
    }
}
