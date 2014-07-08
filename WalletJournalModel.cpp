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
#include "WalletJournalEntryRepository.h"

#include "WalletJournalModel.h"

namespace Evernus
{
    WalletJournalModel::WalletJournalModel(const WalletJournalEntryRepository &journalRepo, QObject *parent)
        : QAbstractTableModel{parent}
        , mJournalRepository{journalRepo}
    {
    }

    Qt::ItemFlags WalletJournalModel::flags(const QModelIndex &index) const
    {
        return QAbstractTableModel::flags(index);
    }

    QVariant WalletJournalModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        {
            switch (section) {
            case 0:
                return tr("Ignored");
            case 1:
                return tr("Date");
            case 2:
                return tr("Type");
            case 3:
                return tr("First party");
            case 4:
                return tr("Second party");
            case 5:
                return tr("Additional data");
            case 6:
                return tr("Amount");
            case 7:
                return tr("Balance after");
            case 8:
                return tr("Reason");
            }
        }

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

    void WalletJournalModel::setCharacter(Character::IdType id)
    {
        mCharacterId = id;
        reset();
    }

    void WalletJournalModel::reset()
    {
        beginResetModel();

        if (mCharacterId == Character::invalidId)
            mData.clear();
        else
            mData = mJournalRepository.fetchForCharacterInRange(mCharacterId, QDateTime{mFrom}, QDateTime{mTill});

        endResetModel();
    }
}
