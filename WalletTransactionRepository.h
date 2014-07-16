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

#include "WalletTransaction.h"
#include "Repository.h"

namespace Evernus
{
    class WalletTransactionRepository
        : public Repository<WalletTransaction>
    {
    public:
        enum class EntryType
        {
            All,
            Buy,
            Sell
        };

        using Repository::Repository;
        virtual ~WalletTransactionRepository() = default;

        virtual QString getTableName() const override;
        virtual QString getIdColumn() const override;

        virtual WalletTransaction populate(const QSqlRecord &record) const override;

        void create(const Repository<Character> &characterRepo) const;

        WalletTransaction::IdType getLatestEntryId(Character::IdType characterId) const;

        void setIgnored(WalletTransaction::IdType id, bool ignored) const;
        void deleteOldEntires(const QDateTime &from) const;

        std::vector<WalletTransaction> fetchForCharacterInRange(Character::IdType characterId,
                                                                const QDateTime &from,
                                                                const QDateTime &till,
                                                                EntryType type,
                                                                EveType::IdType typeId = EveType::invalidId) const;

    private:
        virtual QStringList getColumns() const override;
        virtual void bindValues(const WalletTransaction &entity, QSqlQuery &query) const override;
        virtual void bindPositionalValues(const WalletTransaction &entity, QSqlQuery &query) const override;

        virtual size_t getMaxRowsPerInsert() const override;
    };
}
