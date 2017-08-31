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

#include <boost/optional.hpp>

#include <QDateTime>

#include "Character.h"

namespace Evernus
{
    class WalletJournalEntry
        : public Entity<quint64>
    {
    public:
        using TaxReceiverType = boost::optional<quint64>;
        using ExtraInfoIdType = boost::optional<quint64>;
        using TaxAmountType = boost::optional<double>;

        using Entity::Entity;

        WalletJournalEntry() = default;
        WalletJournalEntry(const WalletJournalEntry &) = default;
        WalletJournalEntry(WalletJournalEntry &&) = default;
        virtual ~WalletJournalEntry() = default;

        Character::IdType getCharacterId() const noexcept;
        void setCharacterId(Character::IdType id) noexcept;

        QDateTime getTimestamp() const;
        void setTimestamp(const QDateTime &dt);

        QString getRefType() const &;
        QString &&getRefType() && noexcept;
        void setRefType(const QString &type);
        void setRefType(QString &&type) noexcept;

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

        ExtraInfoIdType getExtraInfoId() const noexcept;
        void setExtraInfoId(ExtraInfoIdType id) noexcept;

        QString getExtraInfoType() const &;
        QString &&getExtraInfoType() && noexcept;
        void setExtraInfoType(const QString &type);
        void setExtraInfoType(QString &&type) noexcept;

        double getAmount() const noexcept;
        void setAmount(double value) noexcept;

        double getBalance() const noexcept;
        void setBalance(double value) noexcept;

        QString getReason() const &;
        QString &&getReason() && noexcept;
        void setReason(const QString &reason);
        void setReason(QString &&reason);

        TaxReceiverType getTaxReceiverId() const noexcept;
        void setTaxReceiverId(TaxReceiverType id) noexcept;

        TaxAmountType getTaxAmount() const noexcept;
        void setTaxAmount(TaxAmountType amount) noexcept;

        quint64 getCorporationId() const noexcept;
        void setCorporationId(quint64 id) noexcept;

        bool isIgnored() const noexcept;
        void setIgnored(bool flag) noexcept;

        WalletJournalEntry &operator =(const WalletJournalEntry &) = default;
        WalletJournalEntry &operator =(WalletJournalEntry &&) = default;

    private:
        Character::IdType mCharacterId = Character::invalidId;
        QDateTime mTimestamp;
        QString mRefType = 0;
        QString mOwnerName1;
        quint64 mOwnerId1 = 0;
        QString mOwnerName2;
        quint64 mOwnerId2 = 0;
        ExtraInfoIdType mExtraInfoId;
        QString mExtraInfoType;
        double mAmount = 0.;
        double mBalance = 0.;
        QString mReason;
        TaxReceiverType mTaxReceiverId = static_cast<quint64>(0u);
        TaxAmountType mTaxAmount = 0.;
        quint64 mCorporationId = 0;
        bool mIgnored = false;
    };

    bool operator <(const WalletJournalEntry &a, const WalletJournalEntry &b);
}
