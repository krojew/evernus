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
#include "WalletJournalModel.h"

namespace Evernus
{
    WalletJournalModel::WalletJournalModel(QObject *parent)
        : QAbstractTableModel{parent}
    {
    }

    Qt::ItemFlags WalletJournalModel::flags(const QModelIndex &index) const
    {
        return QAbstractTableModel::flags(index);
    }

    QVariant WalletJournalModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        return QVariant{};
    }

    int WalletJournalModel::columnCount(const QModelIndex &parent) const
    {
        return 0;
    }

    QVariant WalletJournalModel::data(const QModelIndex &index, int role) const
    {
        return QVariant{};
    }

    bool WalletJournalModel::setData(const QModelIndex &index, const QVariant &value, int role)
    {
        return false;
    }

    int WalletJournalModel::rowCount(const QModelIndex &parent) const
    {
        return 0;
    }
}
