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
#include "WalletJournalEntry.h"

namespace Evernus
{
    Character::IdType WalletJournalEntry::getCharacterId() const noexcept
    {
        return mCharacterId;
    }

    void WalletJournalEntry::setCharacterId(Character::IdType id) noexcept
    {
        mCharacterId = id;
    }

    QDateTime WalletJournalEntry::getTimestamp() const
    {
        return mTimestamp;
    }

    void WalletJournalEntry::setTimestamp(const QDateTime &dt)
    {
        mTimestamp = dt;
    }

    uint WalletJournalEntry::getRefTypeId() const noexcept
    {
        return mRefTypeId;
    }

    void WalletJournalEntry::setRefTypeId(uint id) noexcept
    {
        mRefTypeId = id;
    }

    QString WalletJournalEntry::getOwnerName1() const &
    {
        return mOwnerName1;
    }

    QString &&WalletJournalEntry::getOwnerName1() && noexcept
    {
        return std::move(mOwnerName1);
    }

    void WalletJournalEntry::setOwnerName1(const QString &name)
    {
        mOwnerName1 = name;
    }

    void WalletJournalEntry::setOwnerName1(QString &&name)
    {
        mOwnerName1 = std::move(name);
    }

    quint64 WalletJournalEntry::getOwnerId1() const noexcept
    {
        return mOwnerId1;
    }

    void WalletJournalEntry::setOwnerId1(quint64 id) noexcept
    {
        mOwnerId1 = id;
    }

    QString WalletJournalEntry::getOwnerName2() const &
    {
        return mOwnerName2;
    }

    QString &&WalletJournalEntry::getOwnerName2() && noexcept
    {
        return std::move(mOwnerName2);
    }

    void WalletJournalEntry::setOwnerName2(const QString &name)
    {
        mOwnerName2 = name;
    }

    void WalletJournalEntry::setOwnerName2(QString &&name)
    {
        mOwnerName2 = std::move(name);
    }

    quint64 WalletJournalEntry::getOwnerId2() const noexcept
    {
        return mOwnerId2;
    }

    void WalletJournalEntry::setOwnerId2(quint64 id) noexcept
    {
        mOwnerId2 = id;
    }

    WalletJournalEntry::ArgType WalletJournalEntry::getArgName() const &
    {
        return mArgName;
    }

    WalletJournalEntry::ArgType &&WalletJournalEntry::getArgName() && noexcept
    {
        return std::move(mArgName);
    }

    void WalletJournalEntry::setArgName(const ArgType &name)
    {
        mArgName = name;
    }

    void WalletJournalEntry::setArgName(ArgType &&name)
    {
        mArgName = std::move(name);
    }

    quint64 WalletJournalEntry::getArgId() const noexcept
    {
        return mArgId;
    }

    void WalletJournalEntry::setArgId(quint64 id) noexcept
    {
        mArgId = id;
    }

    double WalletJournalEntry::getAmount() const noexcept
    {
        return mAmount;
    }

    void WalletJournalEntry::setAmount(double value) noexcept
    {
        mAmount = value;
    }

    double WalletJournalEntry::getBalance() const noexcept
    {
        return mBalance;
    }

    void WalletJournalEntry::setBalance(double value) noexcept
    {
        mBalance = value;
    }

    WalletJournalEntry::ReasonType WalletJournalEntry::getReason() const &
    {
        return mReason;
    }

    WalletJournalEntry::ReasonType &&WalletJournalEntry::getReason() && noexcept
    {
        return std::move(mReason);
    }

    void WalletJournalEntry::setReason(const ReasonType &reason)
    {
        mReason = reason;
    }

    void WalletJournalEntry::setReason(ReasonType &&reason)
    {
        mReason = std::move(reason);
    }

    WalletJournalEntry::TaxReceiverType WalletJournalEntry::getTaxReceiverId() const noexcept
    {
        return mTaxReceiverId;
    }

    void WalletJournalEntry::setTaxReceiverId(TaxReceiverType id) noexcept
    {
        mTaxReceiverId = id;
    }

    WalletJournalEntry::TaxAmountType WalletJournalEntry::getTaxAmount() const noexcept
    {
        return mTaxAmount;
    }

    void WalletJournalEntry::setTaxAmount(TaxAmountType amount) noexcept
    {
        mTaxAmount = amount;
    }

    uint WalletJournalEntry::getCorporationId() const noexcept
    {
        return mCorporationId;
    }

    void WalletJournalEntry::setCorporationId(uint id) noexcept
    {
        mCorporationId = id;
    }

    bool WalletJournalEntry::isIgnored() const noexcept
    {
        return mIgnored;
    }

    void WalletJournalEntry::setIgnored(bool flag) noexcept
    {
        mIgnored = flag;
    }

    bool operator <(const WalletJournalEntry &a, const WalletJournalEntry &b)
    {
        return a.getId() < b.getId();
    }
}
