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
        return mData.mCharacterId;
    }

    void WalletJournalEntry::setCharacterId(Character::IdType id) noexcept
    {
        mData.mCharacterId = id;
    }

    QDateTime WalletJournalEntry::getTimestamp() const
    {
        return mData.mTimestamp;
    }

    void WalletJournalEntry::setTimestamp(const QDateTime &dt)
    {
        mData.mTimestamp = dt;
    }

    uint WalletJournalEntry::getRefTypeId() const noexcept
    {
        return mData.mRefTypeId;
    }

    void WalletJournalEntry::setRefTypeId(uint id) noexcept
    {
        mData.mRefTypeId = id;
    }

    QString WalletJournalEntry::getOwnerName1() const &
    {
        return mData.mOwnerName1;
    }

    QString &&WalletJournalEntry::getOwnerName1() && noexcept
    {
        return std::move(mData.mOwnerName1);
    }

    void WalletJournalEntry::setOwnerName1(const QString &name)
    {
        mData.mOwnerName1 = name;
    }

    void WalletJournalEntry::setOwnerName1(QString &&name)
    {
        mData.mOwnerName1 = std::move(name);
    }

    quint64 WalletJournalEntry::getOwnerId1() const noexcept
    {
        return mData.mOwnerId1;
    }

    void WalletJournalEntry::setOwnerId1(quint64 id) noexcept
    {
        mData.mOwnerId1 = id;
    }

    QString WalletJournalEntry::getOwnerName2() const &
    {
        return mData.mOwnerName2;
    }

    QString &&WalletJournalEntry::getOwnerName2() && noexcept
    {
        return std::move(mData.mOwnerName2);
    }

    void WalletJournalEntry::setOwnerName2(const QString &name)
    {
        mData.mOwnerName2 = name;
    }

    void WalletJournalEntry::setOwnerName2(QString &&name)
    {
        mData.mOwnerName2 = std::move(name);
    }

    quint64 WalletJournalEntry::getOwnerId2() const noexcept
    {
        return mData.mOwnerId2;
    }

    void WalletJournalEntry::setOwnerId2(quint64 id) noexcept
    {
        mData.mOwnerId2 = id;
    }

    WalletJournalEntry::ArgType WalletJournalEntry::getArgName() const &
    {
        return mData.mArgName;
    }

    WalletJournalEntry::ArgType &&WalletJournalEntry::getArgName() && noexcept
    {
        return std::move(mData.mArgName);
    }

    void WalletJournalEntry::setArgName(const ArgType &name)
    {
        mData.mArgName = name;
    }

    void WalletJournalEntry::setArgName(ArgType &&name)
    {
        mData.mArgName = std::move(name);
    }

    quint64 WalletJournalEntry::getArgId() const noexcept
    {
        return mData.mArgId;
    }

    void WalletJournalEntry::setArgId(quint64 id) noexcept
    {
        mData.mArgId = id;
    }

    double WalletJournalEntry::getAmount() const noexcept
    {
        return mData.mAmount;
    }

    void WalletJournalEntry::setAmount(double value) noexcept
    {
        mData.mAmount = value;
    }

    double WalletJournalEntry::getBalance() const noexcept
    {
        return mData.mBalance;
    }

    void WalletJournalEntry::setBalance(double value) noexcept
    {
        mData.mBalance = value;
    }

    WalletJournalEntry::ReasonType WalletJournalEntry::getReason() const &
    {
        return mData.mReason;
    }

    WalletJournalEntry::ReasonType &&WalletJournalEntry::getReason() && noexcept
    {
        return std::move(mData.mReason);
    }

    void WalletJournalEntry::setReason(const ReasonType &reason)
    {
        mData.mReason = reason;
    }

    void WalletJournalEntry::setReason(ReasonType &&reason)
    {
        mData.mReason = std::move(reason);
    }

    WalletJournalEntry::TaxReceiverType WalletJournalEntry::getTaxReceiverId() const noexcept
    {
        return mData.mTaxReceiverId;
    }

    void WalletJournalEntry::setTaxReceiverId(TaxReceiverType id) noexcept
    {
        mData.mTaxReceiverId = id;
    }

    WalletJournalEntry::TaxAmountType WalletJournalEntry::getTaxAmount() const noexcept
    {
        return mData.mTaxAmount;
    }

    void WalletJournalEntry::setTaxAmount(TaxAmountType amount) noexcept
    {
        mData.mTaxAmount = amount;
    }
}
