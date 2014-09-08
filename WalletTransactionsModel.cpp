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

#include "CharacterRepository.h"
#include "EveDataProvider.h"
#include "TextUtils.h"

#include "WalletTransactionsModel.h"

namespace Evernus
{
    WalletTransactionsModel::WalletTransactionsModel(const WalletTransactionRepository &transactionsRepo,
                                                     const CharacterRepository &characterRepository,
                                                     const EveDataProvider &dataProvider,
                                                     bool corp,
                                                     QObject *parent)
        : QAbstractTableModel(parent)
        , mTransactionsRepository(transactionsRepo)
        , mCharacterRepository(characterRepository)
        , mDataProvider(dataProvider)
        , mCorp(corp)
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
            if (column == typeIdColumn)
                return mDataProvider.getTypeName(mData[row][typeIdColumn].value<EveType::IdType>());

            return mData[row][column];
        case Qt::DisplayRole:
            switch (column) {
            case ignoredColumn:
                return QVariant{};
            case timestampColumn:
                return TextUtils::dateTimeToString(mData[row][timestampColumn].toDateTime().toLocalTime(), QLocale{});
            case typeColumn:
                return (getType(row) == WalletTransaction::Type::Buy) ?
                       (tr("Buy")) :
                       (tr("Sell"));
            case quantityColumn:
                return QLocale{}.toString(mData[row][quantityColumn].toUInt());
            case typeIdColumn:
                return mDataProvider.getTypeName(mData[row][typeIdColumn].value<EveType::IdType>());
            case priceColumn:
                {
                    auto price = mData[row][priceColumn].toDouble();
                    if (static_cast<WalletTransaction::Type>(mData[row][typeColumn].toInt()) == WalletTransaction::Type::Buy)
                        price = -price;

                    return QLocale{}.toCurrencyString(price, "ISK");
                }
            }

            return mData[row][column];
        case Qt::ForegroundRole:
            if (column == priceColumn)
            {
                return (getType(row) == WalletTransaction::Type::Buy) ?
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

    EveType::IdType WalletTransactionsModel::getTypeId(int row) const
    {
        return mData[row][typeIdColumn].value<EveType::IdType>();
    }

    uint WalletTransactionsModel::getQuantity(int row) const
    {
        return mData[row][quantityColumn].toUInt();
    }

    double WalletTransactionsModel::getPrice(int row) const
    {
        return mData[row][priceColumn].toDouble();
    }

    WalletTransaction::Type WalletTransactionsModel::getType(int row) const
    {
        return static_cast<WalletTransaction::Type>(mData[row][typeColumn].toInt());
    }

    quint64 WalletTransactionsModel::getTotalQuantity() const noexcept
    {
        return mTotalQuantity;
    }

    double WalletTransactionsModel::getTotalSize() const noexcept
    {
        return mTotalSize;
    }

    double WalletTransactionsModel::getTotalIncome() const noexcept
    {
        return mTotalIncome;
    }

    double WalletTransactionsModel::getTotalCost() const noexcept
    {
        return mTotalCost;
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

        mTotalQuantity = 0;
        mTotalSize = 0.;
        mTotalIncome = 0.;
        mTotalCost = 0.;

        mData.clear();
        if (mCharacterId != Character::invalidId)
        {
            try
            {
                const auto entries = (mCorp) ?
                                     (mTransactionsRepository.fetchForCorporationInRange(mCharacterRepository.getCorporationId(mCharacterId),
                                                                                         QDateTime{mFrom}.toUTC(),
                                                                                         QDateTime{mTill}.addDays(1).toUTC(),
                                                                                         mType,
                                                                                         mTypeId)) :
                                     (mTransactionsRepository.fetchForCharacterInRange(mCharacterId,
                                                                                       QDateTime{mFrom}.toUTC(),
                                                                                       QDateTime{mTill}.addDays(1).toUTC(),
                                                                                       mType,
                                                                                       mTypeId));
                mData.reserve(entries.size());

                for (const auto &entry : entries)
                {
                    mData.emplace_back();
                    auto &data = mData.back();

                    data
                        << entry->isIgnored()
                        << entry->getTimestamp()
                        << static_cast<int>(entry->getType())
                        << entry->getQuantity()
                        << entry->getTypeId()
                        << entry->getPrice()
                        << entry->getClientName()
                        << mDataProvider.getLocationName(entry->getLocationId())
                        << entry->getId();

                    mTotalQuantity += entry->getQuantity();
                    mTotalSize += mDataProvider.getTypeVolume(entry->getTypeId());

                    if (entry->getType() == WalletTransaction::Type::Buy)
                        mTotalCost += entry->getPrice();
                    else
                        mTotalIncome += entry->getPrice();
                }
            }
            catch (const CharacterRepository::NotFoundException &)
            {
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
