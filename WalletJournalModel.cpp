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
#include <QRegularExpression>
#include <QLocale>
#include <QColor>
#include <QFont>

#include "EveDataProvider.h"

#include "WalletJournalModel.h"

namespace Evernus
{
    WalletJournalModel
    ::WalletJournalModel(const WalletJournalEntryRepository &journalRepo, const EveDataProvider &dataProvider, QObject *parent)
        : QAbstractTableModel{parent}
        , mJournalRepository{journalRepo}
        , mDataProvider{dataProvider}
    {
        mColumns
            << tr("Ignored")
            << tr("Date")
            << tr("Type")
            << tr("First party")
            << tr("Second party")
            << tr("Additional data")
            << tr("Amount")
            << tr("Balance after")
            << tr("Reason");
    }

    Qt::ItemFlags WalletJournalModel::flags(const QModelIndex &index) const
    {
        auto flags = QAbstractTableModel::flags(index);
        if (index.isValid() && index.column() == ignoredColumn)
            flags |= Qt::ItemIsUserCheckable;

        return flags;
    }

    QVariant WalletJournalModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
            return mColumns[section];

        return QVariant{};
    }

    int WalletJournalModel::columnCount(const QModelIndex &parent) const
    {
        if (!parent.isValid())
            return mColumns.count();

        return 0;
    }

    QVariant WalletJournalModel::data(const QModelIndex &index, int role) const
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
            case amountColumn:
            case balanceColumn:
                return QLocale{}.toCurrencyString(mData[row][column].toDouble(), "ISK");
            }

            return mData[row][column];
        case Qt::ForegroundRole:
            if (column == amountColumn)
                return (mData[row][amountColumn].toDouble() < 0.) ? (QColor{Qt::darkRed}) : (QColor{Qt::darkGreen});
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

    bool WalletJournalModel::setData(const QModelIndex &index, const QVariant &value, int role)
    {
        if (role == Qt::CheckStateRole && index.isValid() && index.column() == ignoredColumn)
        {
            auto &data = mData[index.row()];

            const auto ignored = value.toInt() == Qt::Checked;
            data[ignoredColumn] = ignored;

            mJournalRepository.setIgnored(data[idColumn].value<WalletJournalEntry::IdType>(), ignored);

            emit dataChanged(index, index, QVector<int>{} << Qt::CheckStateRole << Qt::FontRole);
            return true;
        }

        return false;
    }

    int WalletJournalModel::rowCount(const QModelIndex &parent) const
    {
        if (!parent.isValid())
            return static_cast<int>(mData.size());

        return 0;
    }

    void WalletJournalModel::setFilter(Character::IdType id, const QDate &from, const QDate &till, EntryType type)
    {
        mCharacterId = id;
        mFrom = from;
        mTill = till;
        mType = type;

        reset();
    }

    void WalletJournalModel::reset()
    {
        beginResetModel();

        mData.clear();
        if (mCharacterId != Character::invalidId)
        {
            const auto entries
                = mJournalRepository.fetchForCharacterInRange(mCharacterId, QDateTime{mFrom}.toUTC(), QDateTime{mTill}.addDays(1).toUTC(), mType);
            mData.reserve(entries.size());

            QRegularExpression re{"^DESC: "};

            for (const auto &entry : entries)
            {
                const auto argName = entry->getArgName();
                auto reason = entry->getReason();

                mData.emplace_back();
                auto &data = mData.back();

                data
                    << entry->isIgnored()
                    << entry->getTimestamp()
                    << mDataProvider.getRefTypeName(entry->getRefTypeId())
                    << entry->getOwnerName1()
                    << entry->getOwnerName2()
                    << ((argName) ? (*argName) : (QVariant{}))
                    << entry->getAmount()
                    << entry->getBalance()
                    << ((reason) ? (reason->remove(re)) : (QVariant{}))
                    << entry->getId();
            }
        }

        endResetModel();
    }
}
