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

    QString WalletJournalEntry::getRefType() const &
    {
        return mRefType;
    }

    QString &&WalletJournalEntry::getRefType() && noexcept
    {
        return std::move(mRefType);
    }

    void WalletJournalEntry::setRefType(const QString &type)
    {
        mRefType = type;
    }

    void WalletJournalEntry::setRefType(QString &&type) noexcept
    {
        mRefType = std::move(type);
    }

    WalletJournalEntry::PartyIdType WalletJournalEntry::getFirstPartyId() const noexcept
    {
        return mFirstPartyId;
    }

    void WalletJournalEntry::setFirstPartyId(PartyIdType id) noexcept
    {
        mFirstPartyId = std::move(id);
    }

    WalletJournalEntry::PartyIdType WalletJournalEntry::getSecondPartyId() const noexcept
    {
        return mSecondPartyId;
    }

    void WalletJournalEntry::setSecondPartyId(PartyIdType id) noexcept
    {
        mSecondPartyId = std::move(id);
    }

    QString WalletJournalEntry::getFirstPartyType() const &
    {
        return mFirstPartyType;
    }

    QString &&WalletJournalEntry::getFirstPartyType() && noexcept
    {
        return std::move(mFirstPartyType);
    }

    void WalletJournalEntry::setFirstPartyType(const QString &type)
    {
        mFirstPartyType = type;
    }

    void WalletJournalEntry::setFirstPartyType(QString &&type) noexcept
    {
        mFirstPartyType = std::move(type);
    }

    QString WalletJournalEntry::getSecondPartyType() const &
    {
        return mSecondPartyType;
    }

    QString &&WalletJournalEntry::getSecondPartyType() && noexcept
    {
        return std::move(mSecondPartyType);
    }

    void WalletJournalEntry::setSecondPartyType(const QString &type)
    {
        mSecondPartyType = type;
    }

    void WalletJournalEntry::setSecondPartyType(QString &&type) noexcept
    {
        mSecondPartyType = std::move(type);
    }

    WalletJournalEntry::ExtraInfoIdType WalletJournalEntry::getExtraInfoId() const noexcept
    {
        return mExtraInfoId;
    }

    void WalletJournalEntry::setExtraInfoId(ExtraInfoIdType id) noexcept
    {
        mExtraInfoId = id;
    }

    QString WalletJournalEntry::getExtraInfoType() const &
    {
        return mExtraInfoType;
    }

    QString &&WalletJournalEntry::getExtraInfoType() && noexcept
    {
        return std::move(mExtraInfoType);
    }

    void WalletJournalEntry::setExtraInfoType(const QString &type)
    {
        mExtraInfoType = type;
    }

    void WalletJournalEntry::setExtraInfoType(QString &&type) noexcept
    {
        mExtraInfoType = std::move(type);
    }

    WalletJournalEntry::ISKType WalletJournalEntry::getAmount() const noexcept
    {
        return mAmount;
    }

    void WalletJournalEntry::setAmount(ISKType value) noexcept
    {
        mAmount = std::move(value);
    }

    WalletJournalEntry::ISKType WalletJournalEntry::getBalance() const noexcept
    {
        return mBalance;
    }

    void WalletJournalEntry::setBalance(ISKType value) noexcept
    {
        mBalance = std::move(value);
    }

    QString WalletJournalEntry::getReason() const &
    {
        return mReason;
    }

    QString &&WalletJournalEntry::getReason() && noexcept
    {
        return std::move(mReason);
    }

    void WalletJournalEntry::setReason(const QString &reason)
    {
        mReason = reason;
    }

    void WalletJournalEntry::setReason(QString &&reason)
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

    WalletJournalEntry::ISKType WalletJournalEntry::getTaxAmount() const noexcept
    {
        return mTaxAmount;
    }

    void WalletJournalEntry::setTaxAmount(ISKType amount) noexcept
    {
        mTaxAmount = amount;
    }

    quint64 WalletJournalEntry::getCorporationId() const noexcept
    {
        return mCorporationId;
    }

    void WalletJournalEntry::setCorporationId(quint64 id) noexcept
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
