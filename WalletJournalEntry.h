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

#include "WalletJournalData.h"
#include "Entity.h"

namespace Evernus
{
    class WalletJournalEntry
        : public Entity<quint64>
    {
    public:
        typedef WalletJournalData::ArgType ArgType;
        typedef WalletJournalData::ReasonType ReasonType;
        typedef WalletJournalData::TaxReceiverType TaxReceiverType;
        typedef WalletJournalData::TaxAmountType TaxAmountType;

        using Entity::Entity;
        virtual ~WalletJournalEntry() = default;

        Character::IdType getCharacterId() const noexcept;
        void setCharacterId(Character::IdType id) noexcept;

        QDateTime getTimestamp() const;
        void setTimestamp(const QDateTime &dt);

        uint getRefTypeId() const noexcept;
        void setRefTypeId(uint id) noexcept;

        QString getOwnerName1() const &;
        QString &&getOwnerName1() && noexcept;
        void setOwnerName1(const QString &name);
        void setOwnerName1(QString &&name);

        quint64 getOwnerId1() const noexcept;
        void setOwnerId1(quint64 id) noexcept;

        QString getOwnerName2() const &;
        QString &&getOwnerName2() && noexcept;
        void setOwnerName2(const QString &name);
        void setOwnerName2(QString &&name);

        quint64 getOwnerId2() const noexcept;
        void setOwnerId2(quint64 id) noexcept;

        ArgType getArgName() const &;
        ArgType &&getArgName() && noexcept;
        void setArgName(const ArgType &name);
        void setArgName(ArgType &&name);

        quint64 getArgId() const noexcept;
        void setArgId(quint64 id) noexcept;

        double getAmount() const noexcept;
        void setAmount(double value) noexcept;

        double getBalance() const noexcept;
        void setBalance(double value) noexcept;

        ReasonType getReason() const &;
        ReasonType &&getReason() && noexcept;
        void setReason(const ReasonType &reason);
        void setReason(ReasonType &&reason);

        TaxReceiverType getTaxReceiverId() const noexcept;
        void setTaxReceiverId(TaxReceiverType id) noexcept;

        TaxAmountType getTaxAmount() const noexcept;
        void setTaxAmount(TaxAmountType amount) noexcept;

        bool isIgnored() const noexcept;
        void setIgnored(bool flag) noexcept;

    private:
        WalletJournalData mData;
        bool mIgnored = false;
    };
}
