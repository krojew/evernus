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
#include "MarketOrderRepository.h"

#include "MarketOrderSellModel.h"

namespace Evernus
{
    MarketOrderSellModel::MarketOrderSellModel(const MarketOrderRepository &orderRepo, QObject *parent)
        : MarketOrderModel{parent}
        , mOrderRepo{orderRepo}
    {
    }

    int MarketOrderSellModel::columnCount(const QModelIndex &parent) const
    {
        return 15;
    }

    QVariant MarketOrderSellModel::data(const QModelIndex &index, int role) const
    {
        return QVariant{};
    }

    QVariant MarketOrderSellModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        {
            switch (section) {
            case 0:
                return tr("Name");
            case 1:
                return tr("Group");
            case 2:
                return tr("Status");
            case 3:
                return tr("Custom cost");
            case 4:
                return tr("Price");
            case 5:
                return tr("Price status");
            case 6:
                return tr("Volume");
            case 7:
                return tr("Total");
            case 8:
                return tr("Delta");
            case 9:
                return tr("Profit");
            case 10:
                return tr("Total profit");
            case 11:
                return tr("Profit per item");
            case 12:
                return tr("Time left");
            case 13:
                return tr("Order age");
            case 14:
                return tr("Station");
            }
        }

        return QVariant{};
    }

    QModelIndex MarketOrderSellModel::index(int row, int column, const QModelIndex &parent) const
    {
        return QModelIndex{};
    }

    QModelIndex MarketOrderSellModel::parent(const QModelIndex &index) const
    {
        return QModelIndex{};
    }

    int MarketOrderSellModel::rowCount(const QModelIndex &parent) const
    {
        return 0;
    }

    uint MarketOrderSellModel::getOrderCount() const
    {
        return 0;
    }

    quint64 MarketOrderSellModel::getVolumeRemaining() const
    {
        return 0;
    }

    quint64 MarketOrderSellModel::getVolumeEntered() const
    {
        return 0;
    }

    double MarketOrderSellModel::getTotalISK() const
    {
        return 0.;
    }

    double MarketOrderSellModel::getTotalSize() const
    {
        return 0.;
    }

    void MarketOrderSellModel::setCharacter(Character::IdType id)
    {
        mCharacterId = id;
        reset();
    }

    void MarketOrderSellModel::reset()
    {

    }
}
