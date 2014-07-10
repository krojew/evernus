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
#pragma once

#include <vector>

#include <QAbstractTableModel>
#include <QDate>

#include "WalletJournalEntryRepository.h"
#include "Character.h"

namespace Evernus
{
    class EveDataProvider;

    class WalletJournalModel
        : public QAbstractTableModel
    {
    public:
        typedef WalletJournalEntryRepository::EntryType EntryType;

        WalletJournalModel(const WalletJournalEntryRepository &journalRepo,
                           const EveDataProvider &dataProvider,
                           QObject *parent = nullptr);
        virtual ~WalletJournalModel() = default;

        virtual Qt::ItemFlags flags(const QModelIndex &index) const override;
        virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
        virtual int columnCount(const QModelIndex &parent = QModelIndex{}) const override;
        virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
        virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
        virtual int rowCount(const QModelIndex &parent = QModelIndex{}) const override;

        void setFilter(Character::IdType id, const QDate &from, const QDate &till, EntryType type);

        void reset();

    private:
        static const auto ignoredColumn = 0;
        static const auto timestampColumn = 1;
        static const auto amountColumn = 6;
        static const auto idColumn = 9;

        const WalletJournalEntryRepository &mJournalRepository;
        const EveDataProvider &mDataProvider;

        Character::IdType mCharacterId = Character::invalidId;
        QDate mFrom, mTill;
        EntryType mType = EntryType::All;

        std::vector<QVariantList> mData;

        QStringList mColumns;
    };
}
