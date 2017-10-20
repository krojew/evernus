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
#include <boost/scope_exit.hpp>

#include <QLocale>
#include <QDate>

#include "EveDataProvider.h"
#include "MiningLedger.h"

#include "MiningLedgerModel.h"

namespace Evernus
{
    MiningLedgerModel::MiningLedgerModel(const EveDataProvider &dataProvider,
                                         const MiningLedgerRepository &ledgerRepo,
                                         QObject *parent)
        : QAbstractTableModel{parent}
        , mDataProvider{dataProvider}
        , mLedgerRepo{ledgerRepo}
    {
    }

    int MiningLedgerModel::columnCount(const QModelIndex &parent) const
    {
        Q_UNUSED(parent);
        return numColumns;
    }

    QVariant MiningLedgerModel::data(const QModelIndex &index, int role) const
    {
        if (Q_UNLIKELY(!index.isValid()))
            return {};

        if (role == Qt::DisplayRole)
        {
            const auto &data = mData[index.row()];
            Q_ASSERT(data);

            switch (index.column()) {
            case nameColumn:
                return mDataProvider.getTypeName(data->getTypeId());
            case dateColumn:
                return data->getDate();
            case quantityColumn:
                return QLocale{}.toString(data->getQuantity());
            case solarSystemColumn:
                return mDataProvider.getSolarSystemName(data->getSolarSystemId());
            }
        }
        else if (role == Qt::UserRole)
        {
            const auto &data = mData[index.row()];
            Q_ASSERT(data);

            switch (index.column()) {
            case nameColumn:
                return mDataProvider.getTypeName(data->getTypeId());
            case dateColumn:
                return data->getDate();
            case quantityColumn:
                return data->getQuantity();
            case solarSystemColumn:
                return mDataProvider.getSolarSystemName(data->getSolarSystemId());
            }
        }

        return {};
    }

    QVariant MiningLedgerModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
        {
            switch (section) {
            case nameColumn:
                return tr("Name");
            case dateColumn:
                return tr("Date");
            case quantityColumn:
                return tr("Quantity");
            case solarSystemColumn:
                return tr("Solar system");
            }
        }

        return {};
    }

    int MiningLedgerModel::rowCount(const QModelIndex &parent) const
    {
        return (parent.isValid()) ? (0) : (static_cast<int>(mData.size()));
    }

    void MiningLedgerModel::refresh(Character::IdType charId, const QDate &from, const QDate &to)
    {
        beginResetModel();

        BOOST_SCOPE_EXIT(this_) {
            this_->endResetModel();
        } BOOST_SCOPE_EXIT_END

        mData = mLedgerRepo.fetchForCharacter(charId, from, to);
    }

    EveType::IdType MiningLedgerModel::getTypeId(const QModelIndex &index) const
    {
        if (Q_LIKELY(index.isValid()))
        {
            Q_ASSERT(mData.size() > index.row());
            return mData[index.row()]->getTypeId();
        }

        return EveType::invalidId;
    }
}
