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

#include "WalletTransactionRepository.h"
#include "Character.h"

namespace Evernus
{
    class CharacterRepository;
    class EveDataProvider;

    class WalletTransactionsModel
        : public QAbstractTableModel
    {
        Q_OBJECT

    public:
        typedef WalletTransactionRepository::EntryType EntryType;

        WalletTransactionsModel(const WalletTransactionRepository &transactionsRepo,
                                const CharacterRepository &characterRepository,
                                const EveDataProvider &dataProvider,
                                bool corp,
                                QObject *parent = nullptr);
        virtual ~WalletTransactionsModel() = default;

        virtual Qt::ItemFlags flags(const QModelIndex &index) const override;
        virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
        virtual int columnCount(const QModelIndex &parent = QModelIndex{}) const override;
        virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
        virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
        virtual int rowCount(const QModelIndex &parent = QModelIndex{}) const override;

        EveType::IdType getTypeId(int row) const;
        uint getQuantity(int row) const;
        double getPrice(int row) const;

        void setFilter(Character::IdType id, const QDate &from, const QDate &till, EntryType type, EveType::IdType typeId = EveType::invalidId);

        void reset();
        void clear();

    private:
        static const auto ignoredColumn = 0;
        static const auto timestampColumn = 1;
        static const auto typeColumn = 2;
        static const auto quantityColumn = 3;
        static const auto typeIdColumn = 4;
        static const auto priceColumn = 5;
        static const auto idColumn = 8;

        const WalletTransactionRepository &mTransactionsRepository;
        const CharacterRepository &mCharacterRepository;
        const EveDataProvider &mDataProvider;

        Character::IdType mCharacterId = Character::invalidId;
        QDate mFrom, mTill;
        EntryType mType = EntryType::All;
        EveType::IdType mTypeId = EveType::invalidId;

        std::vector<QVariantList> mData;

        QStringList mColumns;

        bool mCorp = false;
    };
}
