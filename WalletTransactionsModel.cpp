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
#include <QColor>
#include <QFont>

#include "EveDataProvider.h"

#include "WalletTransactionsModel.h"

namespace Evernus
{
    WalletTransactionsModel
    ::WalletTransactionsModel(const WalletTransactionRepository &transactionsRepo, const EveDataProvider &dataProvider, QObject *parent)
        : QAbstractTableModel{parent}
        , mTransactionsRepository{transactionsRepo}
        , mDataProvider{dataProvider}
    {
        mColumns
            << tr("Ignored")
            << tr("Date")
            << tr("Type")
            << tr("Quantity")
            << tr("Item")
            << tr("Price")
            << tr("Client")
            << tr("Location");
    }

    Qt::ItemFlags WalletTransactionsModel::flags(const QModelIndex &index) const
    {
        auto flags = QAbstractTableModel::flags(index);
        if (index.isValid() && index.column() == ignoredColumn)
            flags |= Qt::ItemIsUserCheckable;

        return flags;
    }

    QVariant WalletTransactionsModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
            return mColumns[section];

        return QVariant{};
    }

    int WalletTransactionsModel::columnCount(const QModelIndex &parent) const
    {
        if (!parent.isValid())
            return mColumns.count();

        return 0;
    }

    QVariant WalletTransactionsModel::data(const QModelIndex &index, int role) const
    {
        if (!index.isValid())
             return QVariant{};

        const auto row = index.row();
        const auto column = index.column();

        switch (role) {
        case Qt::UserRole:
            return mData[row][column];
        case Qt::DisplayRole:
            switch (column) {
            case ignoredColumn:
                return QVariant{};
            case timestampColumn:
                return QLocale{}.toString(mData[row][timestampColumn].toDateTime().toLocalTime());
            case typeColumn:
                return (static_cast<WalletTransaction::Type>(mData[row][typeColumn].toInt()) == WalletTransaction::Type::Buy) ?
                       ("Buy") :
                       ("Sell");
            case quantityColumn:
                return QLocale{}.toString(mData[row][quantityColumn].toUInt());
            case priceColumn:
                return QLocale{}.toCurrencyString(mData[row][priceColumn].toDouble(), "ISK");
            }

            return mData[row][column];
        case Qt::ForegroundRole:
            if (column == priceColumn)
            {
                return (static_cast<WalletTransaction::Type>(mData[row][typeColumn].toInt()) == WalletTransaction::Type::Buy) ?
                       (QColor{Qt::darkRed}) :
                       (QColor{Qt::darkGreen});
            }
            break;
        case Qt::CheckStateRole:
            if (column == ignoredColumn)
                return (mData[row][ignoredColumn].toBool()) ? (Qt::Checked) : (Qt::Unchecked);
            break;
        case Qt::FontRole:
            if (mData[row][ignoredColumn].toBool())
            {
                QFont font;
                font.setStrikeOut(true);

                return font;
            }
        }

        return QVariant{};
    }

    bool WalletTransactionsModel::setData(const QModelIndex &index, const QVariant &value, int role)
    {
        if (role == Qt::CheckStateRole && index.isValid() && index.column() == ignoredColumn)
        {
            auto &data = mData[index.row()];

            const auto ignored = value.toInt() == Qt::Checked;
            data[ignoredColumn] = ignored;

            mTransactionsRepository.setIgnored(data[idColumn].value<WalletTransaction::IdType>(), ignored);

            emit dataChanged(index, index, QVector<int>{} << Qt::CheckStateRole << Qt::FontRole);
            return true;
        }

        return false;
    }

    int WalletTransactionsModel::rowCount(const QModelIndex &parent) const
    {
        if (!parent.isValid())
            return static_cast<int>(mData.size());

        return 0;
    }

    void WalletTransactionsModel::setFilter(Character::IdType id, const QDate &from, const QDate &till, EntryType type, EveType::IdType typeId)
    {
        mCharacterId = id;
        mFrom = from;
        mTill = till;
        mType = type;
        mTypeId = typeId;

        reset();
    }

    void WalletTransactionsModel::reset()
    {
        beginResetModel();

        mData.clear();
        if (mCharacterId != Character::invalidId)
        {
            const auto entries = mTransactionsRepository.fetchForCharacterInRange(mCharacterId, QDateTime{mFrom}.toUTC(), QDateTime{mTill}.toUTC(), mType, mTypeId);
            mData.reserve(entries.size());

            for (const auto &entry : entries)
            {
                mData.emplace_back();
                auto &data = mData.back();

                data
                    << entry.isIgnored()
                    << entry.getTimestamp()
                    << static_cast<int>(entry.getType())
                    << entry.getQuantity()
                    << mDataProvider.getTypeName(entry.getTypeId())
                    << entry.getPrice()
                    << entry.getClientName()
                    << mDataProvider.getLocationName(entry.getLocationId())
                    << entry.getId();
            }
        }

        endResetModel();
    }

    void WalletTransactionsModel::clear()
    {
        beginResetModel();
        mData.clear();
        endResetModel();
    }
}
